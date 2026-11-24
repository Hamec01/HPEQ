# Control-flow tests use a native fake CMake and fake bundles in an isolated out folder.
# No VST compilation, elevation or access to the real installed VST3 directory.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$work=Join-Path $repo ('out/build-scripts-smoke/'+[Guid]::NewGuid().ToString('N'))
$bin=Join-Path $work 'bin';[void][IO.Directory]::CreateDirectory($bin)
$products=@('HamloComp','HamloUdarnik','HamloRedWall','HamloMag','HamloVerb','HamloEcho','HamloZastava','HamloShirota','HamloFader','HamloRevolution')
$script:checks=0
function Assert($Condition,[string]$Message) {
    if (!$Condition) {throw "FAIL: $Message"};$script:checks++
}
$mock=@'
using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
class FakeCmake {
 static string Value(string[] a,string key) { int i=Array.IndexOf(a,key);return i>=0&&i+1<a.Length?a[i+1]:""; }
 static int Main(string[] a) {
  File.AppendAllText(Environment.GetEnvironmentVariable("HAMLO_TEST_TRACE"),string.Join("\t",a.Select(x=>Convert.ToBase64String(Encoding.UTF8.GetBytes(x))))+"\n");
  if(a.Contains("--help")){Console.WriteLine("Visual Studio 18 2026");return 0;}
  string target=Value(a,"--target"), fail=Environment.GetEnvironmentVariable("HAMLO_TEST_FAIL");
  if(!string.IsNullOrEmpty(fail)&&(target==fail||(a.Contains("-S")&&fail=="configure")))return 7;
  if(a.Contains("-S")) {
   if(Environment.GetEnvironmentVariable("HAMLO_TEST_WARN")=="1")Console.Error.WriteLine("fake cmake configure warning");
   string source=Value(a,"-S"),build=Value(a,"-B");Directory.CreateDirectory(build);
   string project=Regex.Match(File.ReadAllText(Path.Combine(source,"CMakeLists.txt")),@"project\s*\(\s*(\w+)",RegexOptions.IgnoreCase).Groups[1].Value;
   File.WriteAllText(Path.Combine(build,"CMakeCache.txt"),"CMAKE_HOME_DIRECTORY:INTERNAL="+source+"\nCMAKE_CACHEFILE_DIR:INTERNAL="+build+"\nCMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026\nCMAKE_GENERATOR_PLATFORM:INTERNAL=x64\nCMAKE_PROJECT_NAME:STATIC="+project+"\n");
   if(project=="HPEQ"){string juce=Path.Combine(build,"_deps","juce-src");Directory.CreateDirectory(juce);File.WriteAllText(Path.Combine(juce,"CMakeLists.txt"),"project(JUCE)");}
  }
  if(target.EndsWith("_VST3")) {
   string name=target.Substring(0,target.Length-5),product=name=="HPEQ"?"HamloRack":name,build=Value(a,"--build");
   string dir=Path.Combine(build,name+"_artefacts","Release","VST3",product+".vst3","Contents","x86_64-win");Directory.CreateDirectory(dir);File.WriteAllText(Path.Combine(dir,product+".vst3"),"FAKE "+product);
  }
  return 0;
 }
}
'@
$cs=Join-Path $bin 'Fake.cs';[IO.File]::WriteAllText($cs,$mock)
$compiler=Join-Path $env:SystemRoot 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
& $compiler /nologo "/out:$(Join-Path $bin 'cmake.exe')" $cs
if ($LASTEXITCODE) {throw 'Could not compile native mock.'}
$oldPath=$env:PATH;$oldPause=$env:HAMLO_NO_PAUSE;$oldJuce=$env:HPEQ_JUCE_DIR
$env:PATH=$bin+';'+$env:PATH;$env:HAMLO_NO_PAUSE='1';$env:HPEQ_JUCE_DIR=''
function Fixture([string]$Name,[bool]$WithJuce=$true) {
    $root=Join-Path $work ($Name+' repo ! & (space)')
    [void][IO.Directory]::CreateDirectory((Join-Path $root 'scripts'))
    foreach($file in @('build_vst.bat','build_hamlo_plugins.bat')) {Copy-Item -LiteralPath (Join-Path $repo $file) -Destination $root}
    Copy-Item -LiteralPath (Join-Path $repo 'scripts/build_hamlo.ps1') -Destination (Join-Path $root 'scripts')
    # If these tests run elevated, installation must still be a local stub.
    [IO.File]::WriteAllText((Join-Path $root 'scripts/install_hamlo_plugins.ps1'), 'param([string]$SourceRoot,[string[]]$PluginNames); [IO.File]::WriteAllText((Join-Path (Split-Path -Parent $PSScriptRoot) "selected-install.txt"),($PluginNames -join ","))')
    [IO.File]::WriteAllText((Join-Path $root 'CMakeLists.txt'),'project(HPEQ)')
    foreach($product in $products) {
        $path=Join-Path $root "Plugins/$product";[void][IO.Directory]::CreateDirectory($path)
        Copy-Item -LiteralPath (Join-Path $repo "Plugins/$product/CMakeLists.txt") -Destination $path
    }
    if($WithJuce){$path=Join-Path $root 'build/_deps/juce-src';[void][IO.Directory]::CreateDirectory($path);[IO.File]::WriteAllText((Join-Path $path 'CMakeLists.txt'),'project(JUCE)')}
    return $root
}
function Run-Bat([string]$Root,[string]$Batch,[string]$Arguments) {
    $env:HAMLO_TEST_TRACE=Join-Path $Root 'trace.txt';[IO.File]::WriteAllText($env:HAMLO_TEST_TRACE,'')
    $previousPreference=$ErrorActionPreference
    try { $ErrorActionPreference='Continue';$text=& $env:ComSpec /d /v:off /c ('call "'+(Join-Path $Root $Batch)+'" '+$Arguments) 2>&1;$batExit=$LASTEXITCODE }
    finally { $ErrorActionPreference=$previousPreference }
    return [pscustomobject]@{Code=$batExit;Text=($text|Out-String)}
}
function Trace([string]$Root) {
    $calls=@();foreach($line in Get-Content -LiteralPath (Join-Path $Root 'trace.txt')) {
        $calls+=,@($line.Split("`t")|ForEach-Object {[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($_))})
    };return ,$calls
}
function New-Bundle([string]$Root,[string]$Name,[string]$Value) {
    $path=Join-Path $Root "$Name.vst3/Contents/x86_64-win";[void][IO.Directory]::CreateDirectory($path)
    [IO.File]::WriteAllText((Join-Path $path "$Name.vst3"),$Value)
}
try {
    $root=Fixture 'single';$result=Run-Bat $root 'build_hamlo_plugins.bat' 'comp';Assert ($result.Code -eq 0) $result.Text
    $calls=Trace $root;$builds=@($calls|Where-Object {$_ -contains '--build'})
    Assert ($builds.Count -eq 2) 'Single comp builds assets then VST, without rack compilation'
    Assert ($builds[0] -contains 'HamloComp_Assets') 'Assets must precede VST'
    Assert ($builds[1] -contains 'HamloComp_VST3') 'Comp VST target'
    Assert (Test-Path -LiteralPath (Join-Path $root 'batvst/HamloComp.vst3/Contents/x86_64-win/HamloComp.vst3')) 'Copied bundle'
    $configure=@($calls|Where-Object {$_ -contains '-S'})[0]
    Assert ($configure -contains (Join-Path $root 'Plugins/HamloComp')) 'Native argument paths survive spaces, parentheses, ampersand and exclamation'

    $root=Fixture 'all';$result=Run-Bat $root 'build_hamlo_plugins.bat' 'all';Assert ($result.Code -eq 0) $result.Text
    $calls=Trace $root;$builds=@($calls|Where-Object {$_ -contains '--build'})
    Assert ($builds.Count -eq 21) 'All: one rack target and two targets per standalone'
    foreach($call in $builds) {Assert (($call -contains '4') -and ($call -contains '/p:CL_MPCount=2')) 'Bound parallelism on every compile'}
    Assert (@(Get-ChildItem -LiteralPath (Join-Path $root 'batvst') -Directory).Count -eq 11) 'All eleven bundles copied'
    foreach($product in $products) {
        $cmake=Get-Content -LiteralPath (Join-Path $repo "Plugins/$product/CMakeLists.txt") -Raw
        Assert ($cmake -match ('juce_add_binary_data\('+[regex]::Escape($product)+'_Assets')) "$product asset target exists"
        $configured=@($calls|Where-Object {($_ -contains '-S') -and ($_ -contains (Join-Path $root "Plugins/$product"))})[0]
        $flag=@($configured|Where-Object {$_ -like '-D*JUCE_DIR=*'})[0];$variable=($flag -split '=')[0].Substring(2)
        Assert ($cmake -match ('set\('+[regex]::Escape($variable)+'\s')) "$product uses its actual JUCE cache variable"
    }

    $root=Fixture 'rack';$result=Run-Bat $root 'build_vst.bat' '';Assert ($result.Code -eq 0) $result.Text
    Assert (@((Trace $root)|Where-Object {$_ -contains '--build'}).Count -eq 1) 'Rack wrapper builds only rack'
    $root=Fixture 'check';$result=Run-Bat $root 'build_hamlo_plugins.bat' 'check all';Assert ($result.Code -eq 0) $result.Text
    Assert (@((Trace $root)|Where-Object {$_ -contains '--build'}).Count -eq 0) 'Check performs no builds'
    Assert (!(Test-Path -LiteralPath (Join-Path $root 'batvst'))) 'Check copies no old or new binaries'
    $result=Run-Bat $root 'build_hamlo_plugins.bat' 'typo';Assert ($result.Code -ne 0) 'Unknown choice fails'
    Assert ((Get-Item -LiteralPath (Join-Path $root 'trace.txt')).Length -eq 0) 'Unknown choice does not bootstrap the rack'
    $result=Run-Bat $root 'build_hamlo_plugins.bat' 'install comp'
    Assert ($result.Text -match 'Selected: HamloComp\.') 'Install prefix retains requested single plug-in'
    $principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Assert ($result.Code -eq 0) $result.Text
        Assert ([IO.File]::ReadAllText((Join-Path $root 'selected-install.txt')) -eq 'HamloComp') 'Elevated mode requests only the selected plug-in from the install stub'
    } else {
        Assert ($result.Code -ne 0) 'Non-elevated install is rejected'
        Assert ((Get-Item -LiteralPath (Join-Path $root 'trace.txt')).Length -eq 0) 'Non-elevated install stops before compiling'
    }

    $root=Fixture 'bootstrap' $false;$result=Run-Bat $root 'build_hamlo_plugins.bat' '11';Assert ($result.Code -eq 0) $result.Text
    $calls=Trace $root;Assert (@($calls|Where-Object {$_ -contains '-S'}).Count -eq 2) 'Fresh standalone configures rack for JUCE, then standalone'
    Assert (!(@($calls|Where-Object {$_ -contains 'HPEQ_VST3'}).Count)) 'JUCE bootstrap does not compile rack'

    $root=Fixture 'moved';$cache=Join-Path $root 'build/CMakeCache.txt'
    $old="CMAKE_HOME_DIRECTORY:INTERNAL=E:/other/PC`nCMAKE_CACHEFILE_DIR:INTERNAL=E:/other/PC/build`n"
    [IO.File]::WriteAllText($cache,$old);$result=Run-Bat $root 'build_vst.bat' '';Assert ($result.Code -eq 0) $result.Text
    Assert ([IO.File]::ReadAllText($cache) -eq $old) 'Moved cache is preserved'
    Assert (Test-Path -LiteralPath (Join-Path $root 'build-rebuild/HamloRack/CMakeCache.txt')) 'Fresh separate build cache selected'

    $root=Fixture 'failure';$env:HAMLO_TEST_FAIL='HamloComp_Assets';$result=Run-Bat $root 'build_hamlo_plugins.bat' 'comp';$env:HAMLO_TEST_FAIL=''
    Assert ($result.Code -ne 0) 'Compiler failure propagates through BAT'
    Assert (!(Test-Path -LiteralPath (Join-Path $root 'batvst'))) 'Failure copies no stale VST'
    Assert (!(@((Trace $root)|Where-Object {$_ -contains 'HamloComp_VST3'}).Count)) 'Failed asset stage stops VST stage'
    $env:HAMLO_TEST_FAIL='configure';$result=Run-Bat $root 'build_hamlo_plugins.bat' 'comp';$env:HAMLO_TEST_FAIL=''
    Assert ($result.Code -ne 0) 'Configure failure propagates through BAT'
    Assert (!(@((Trace $root)|Where-Object {$_ -contains '--build'}).Count)) 'Configure failure stops compilation'
    $env:HAMLO_TEST_WARN='1';$result=Run-Bat $root 'build_hamlo_plugins.bat' 'comp';$env:HAMLO_TEST_WARN=''
    Assert ($result.Code -eq 0) 'CMake stderr warning with exit zero is not a build failure'

    # Run only installer functions, with fake Common Files under the isolated test directory.
    . (Join-Path $repo 'scripts/install_hamlo_plugins.ps1') -FunctionsOnly
    $source=Join-Path $work 'install-source';$common=Join-Path $work 'fake-common';[void][IO.Directory]::CreateDirectory($common)
    New-Bundle $source 'HamloComp' 'new comp';New-Bundle (Join-Path $common 'VST3') 'HamloComp' 'old comp'
    Install-HamloBundles $source $common @('HamloComp')
    Assert ([IO.File]::ReadAllText((Join-Path $common 'VST3/HamloComp.vst3/Contents/x86_64-win/HamloComp.vst3')) -eq 'new comp') 'Selected standalone replaced'
    Assert (@(Get-ChildItem -LiteralPath $common -Filter HamloComp.vst3 -File -Recurse | Where-Object {$_.FullName -like '*backup*'}).Count -eq 1) 'Old standalone retained in backup'
    New-Bundle $source 'HamloRack' 'new rack';New-Bundle (Join-Path $common 'VST3') 'HamloRack' 'old rack';New-Bundle (Join-Path $common 'VST3') 'HPEQ' 'old HPEQ'
    Install-HamloBundles $source $common @('HamloRack') @('HPEQ')
    Assert (!(Test-Path -LiteralPath (Join-Path $common 'VST3/HPEQ.vst3'))) 'Retired rack duplicate removed'
    Assert ([IO.File]::ReadAllText((Join-Path $common 'VST3/HamloRack.vst3/Contents/x86_64-win/HamloRack.vst3')) -eq 'new rack') 'Current rack installed'
    New-Bundle $source 'HamloComp' 'next comp';New-Bundle $source 'HamloFader' 'new fader'
    $locked=Join-Path $common 'VST3/HamloFader.vst3';[IO.File]::WriteAllText($locked,'locked old fader')
    $stream=[IO.File]::Open($locked,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    $failed=$false
    try {Install-HamloBundles $source $common @('HamloComp','HamloFader')} catch {$failed=$true} finally {$stream.Dispose()}
    Assert $failed 'Locked target rejects installation'
    Assert ([IO.File]::ReadAllText((Join-Path $common 'VST3/HamloComp.vst3/Contents/x86_64-win/HamloComp.vst3')) -eq 'new comp') 'Earlier replacement rolled back'
    Assert ([IO.File]::ReadAllText($locked) -eq 'locked old fader') 'Locked original kept intact'
    $previousCommon=$env:CommonProgramW6432
    try {
        $env:CommonProgramW6432=$common
        & (Join-Path $repo 'scripts/install_rack.ps1') -BundlePath (Join-Path $source 'HamloRack.vst3')
    } finally {$env:CommonProgramW6432=$previousCommon}
    Assert ([IO.File]::ReadAllText((Join-Path $common 'VST3/HamloRack.vst3/Contents/x86_64-win/HamloRack.vst3')) -eq 'new rack') 'Real rack installer wrapper uses the supplied bundle and fake destination'
    # Exercise the exact elevated payload construction without requesting UAC.
    $parseErrors=$null
    $ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $repo 'scripts/install_hamlo_plugins.ps1'),[ref]$null,[ref]$parseErrors)
    $assignment=$ast.Find({param($node) $node -is [Management.Automation.Language.AssignmentStatementAst] -and $node.Left.Extent.Text -eq '$payload'},$true)
    $SourceRoot=$source;$commonFiles=$common;$log=Join-Path $work 'elevated-payload.log';$nameLiteral="@('HamloComp')";$oldLiteral='@()'
    $payloadText=& ([scriptblock]::Create($assignment.Extent.Text+"`n"+'$payload'))
    [void][Management.Automation.Language.Parser]::ParseInput($payloadText,[ref]$null,[ref]$parseErrors)
    Assert ($parseErrors.Count -eq 0) 'Serialized elevated installer parses'
    & ([scriptblock]::Create($payloadText))
    Assert ([IO.File]::ReadAllText((Join-Path $common 'VST3/HamloComp.vst3/Contents/x86_64-win/HamloComp.vst3')) -eq 'next comp') 'Elevated payload installs only into the explicit fake destination'
    Write-Host "PASS: $script:checks build/installer assertions. Evidence: $work"
} finally {
    $env:PATH=$oldPath;$env:HAMLO_NO_PAUSE=$oldPause;$env:HPEQ_JUCE_DIR=$oldJuce
    $env:HAMLO_TEST_FAIL='';$env:HAMLO_TEST_TRACE='';$env:HAMLO_TEST_WARN=''
}
