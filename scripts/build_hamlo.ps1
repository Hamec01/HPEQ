param([string]$Choice = '', [string]$Action = '', [switch]$RackOnly)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\','/')
$output = Join-Path $repo 'batvst'
$generator = 'Visual Studio 18 2026'
$plugins = [ordered]@{
    HamloComp='HAMLOCOMP_JUCE_DIR'; HamloUdarnik='HAMLOUDARNIK_JUCE_DIR'
    HamloRedWall='HAMLOREDWALL_JUCE_DIR'; HamloMag='HAMLOMAG_JUCE_DIR'
    HamloVerb='HAMLO_VERB_JUCE_DIR'; HamloEcho='HAMLO_ECHO_JUCE_DIR'
    HamloZastava='HAMLO_ZASTAVA_JUCE_DIR'; HamloShirota='HAMLO_SHIROTA_JUCE_DIR'
    HamloFader='HAMLO_FADER_JUCE_DIR'; HamloRevolution='HAMLO_REVOLUTION_JUCE_DIR'
}
function Read-Cache([string]$Directory, [string]$Key) {
    $file = Join-Path $Directory 'CMakeCache.txt'
    if (Test-Path -LiteralPath $file) {
        $match = Select-String -LiteralPath $file -Pattern ('^' + [regex]::Escape($Key) + ':[^=]+=(.*)$')
        if ($match) { return $match[0].Matches[0].Groups[1].Value }
    }
    return ''
}
function Same-Path([string]$Left, [string]$Right) {
    return $Left -and $Right -and [IO.Path]::GetFullPath($Left).TrimEnd('\','/') -eq [IO.Path]::GetFullPath($Right).TrimEnd('\','/')
}
function Usable-Cache([string]$Directory, [string]$Source) {
    if (!(Test-Path -LiteralPath (Join-Path $Directory 'CMakeCache.txt'))) {
        return !(Test-Path -LiteralPath (Join-Path $Directory 'CMakeFiles'))
    }
    return (Same-Path (Read-Cache $Directory 'CMAKE_HOME_DIRECTORY') $Source) -and
        (Same-Path (Read-Cache $Directory 'CMAKE_CACHEFILE_DIR') $Directory) -and
        (Read-Cache $Directory 'CMAKE_GENERATOR') -eq $generator -and
        (Read-Cache $Directory 'CMAKE_GENERATOR_PLATFORM') -eq 'x64'
}
function Build-Directory([string]$Name, [string]$Source) {
    $preferred = Join-Path $Source 'build'
    if (Usable-Cache $preferred $Source) { return $preferred }
    # Never delete or rewrite a cache belonging to another PC/source/generator.
    for ($index=0; $index -lt 100; $index++) {
        $suffix = if ($index -eq 0) { $Name } else { "$Name-$index" }
        $candidate = Join-Path $repo "build-rebuild/$suffix"
        if (Usable-Cache $candidate $Source) {
            Write-Host "Old/moved cache kept at $preferred. Using $candidate."
            return $candidate
        }
    }
    throw "No usable build directory for $Name."
}
function Find-Juce {
    $candidates = @($env:HPEQ_JUCE_DIR, (Read-Cache $script:rackBuild 'HPEQ_JUCE_DIR'),
        (Join-Path $script:rackBuild '_deps/juce-src'), (Join-Path $repo 'build/_deps/juce-src'))
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'CMakeLists.txt'))) {
            return [IO.Path]::GetFullPath($candidate).TrimEnd('\','/')
        }
    }
    return ''
}
function Run-Cmake([string[]]$Arguments) {
    Write-Host ('cmake ' + (($Arguments | ForEach-Object { '"' + $_ + '"' }) -join ' '))
    if (!$script:checkOnly) {
        # Windows PowerShell may expose native stderr as error records. CMake
        # warnings are not failures: its exit code decides whether to continue.
        $previousPreference = $ErrorActionPreference
        try { $ErrorActionPreference = 'Continue'; & $script:cmake @Arguments; $cmakeExit = $LASTEXITCODE }
        finally { $ErrorActionPreference = $previousPreference }
        if ($cmakeExit -ne 0) { throw "CMake failed (exit $cmakeExit)." }
    }
}
function Configure-Rack {
    $arguments = @('-S', $repo, '-B', $script:rackBuild, '-G', $generator, '-A', 'x64')
    $cachedJuce = Read-Cache $script:rackBuild 'HPEQ_JUCE_DIR'
    if ($env:HPEQ_JUCE_DIR) {
        if (!(Test-Path -LiteralPath (Join-Path $env:HPEQ_JUCE_DIR 'CMakeLists.txt'))) { throw 'HPEQ_JUCE_DIR does not point to JUCE.' }
        $arguments += "-DHPEQ_JUCE_DIR=$env:HPEQ_JUCE_DIR"
    } elseif ($cachedJuce -and !(Test-Path -LiteralPath (Join-Path $cachedJuce 'CMakeLists.txt'))) {
        $arguments += '-DHPEQ_JUCE_DIR='
    } elseif (!(Test-Path -LiteralPath (Join-Path $script:rackBuild 'CMakeCache.txt'))) {
        $available = Find-Juce
        if ($available) { $arguments += "-DHPEQ_JUCE_DIR=$available" }
    }
    $hpdg = Read-Cache $script:rackBuild 'HPEQ_HPDG_SOURCE_DIR'
    if ($env:HPEQ_HPDG_SOURCE_DIR) { $arguments += "-DHPEQ_HPDG_SOURCE_DIR=$env:HPEQ_HPDG_SOURCE_DIR" }
    elseif ($hpdg -and !(Test-Path -LiteralPath (Join-Path $hpdg 'Audio/DrumCompressorBlock.cpp'))) { $arguments += '-DHPEQ_HPDG_SOURCE_DIR=' }
    Run-Cmake $arguments
}
function Compile-Target([string]$Directory, [string]$Target) {
    Run-Cmake @('--build', $Directory, '--config', 'Release', '--parallel', '4', '--target', $Target, '--', '/p:CL_MPCount=2')
}
function Copy-Bundle([string]$Directory, [string]$Target, [string]$Product) {
    $bundle = Join-Path $Directory "${Target}_artefacts/Release/VST3/$Product.vst3"
    if (!$script:checkOnly) {
        $binary = Join-Path $bundle "Contents/x86_64-win/$Product.vst3"
        if (!(Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Expected VST3 binary missing: $binary" }
        [void][IO.Directory]::CreateDirectory($output)
        Copy-Item -LiteralPath $bundle -Destination $output -Recurse -Force
    }
    Write-Host "VST3: $(Join-Path $output "$Product.vst3")"
}
try {
    if ($args.Count) { throw 'Unexpected extra arguments.' }
    if (!$Choice -and !$RackOnly) {
        Write-Host 'Hamlo VST3 build - Release'
        Write-Host '0 All | 1 Rack | 2 Comp | 3 Udarnik | 4 RedWall | 5 Mag | 6 Verb'
        Write-Host '7 Echo | 8 Zastava | 9 Shirota | 10 Fader | 11 Revolution'
        Write-Host 'I Build and install all | Q Quit'
        $Choice = Read-Host 'Choice (default 0)'
        if ($Choice -eq 'q') { exit 0 }
        if ($Choice -eq 'i') { $Choice='install'; $Action='all' }
    }
    $install = $Choice -eq 'install' -or $Action -eq 'install'
    $checkOnly = $Choice -eq 'check' -or $Action -eq 'check'
    if ($install -and $checkOnly) { throw 'Choose install or check, not both.' }
    $name = if ($Choice -eq 'install' -or $Choice -eq 'check') { $Action } else { $Choice }
    if ($name -eq 'install' -or $name -eq 'check') { $name='' }
    if (!$name) { $name = if ($RackOnly) { 'rack' } else { 'all' } }
    $aliases = @{ '0'='all'; '1'='HPEQ'; rack='HPEQ'; hamlorack='HPEQ'; hpeq='HPEQ'; all='all' }
    $number=2
    foreach ($product in $plugins.Keys) {
        $aliases[[string]$number]=$product; $number++
        $aliases[$product]=$product; $aliases[$product.Substring(5)]=$product
    }
    $aliases['rev']='HamloRevolution'
    if (!$aliases.ContainsKey($name)) { throw "Unknown plug-in: $name" }
    $selected = $aliases[$name]
    if ($RackOnly -and $selected -ne 'HPEQ') { throw 'build_vst.bat builds the rack only.' }
    if ($Action -and $Choice -notin @('install','check') -and $Action -notin @('install','check')) { throw "Unknown action: $Action" }
    Write-Host "Selected: $selected. Installation requested: $install."
    if ($install) {
        $principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
        if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Build-and-install needs an administrator terminal. Close the DAW before installing.' }
    }
    $cmake = (Get-Command cmake -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $helpText = & $cmake --help | Out-String
    if ($LASTEXITCODE -ne 0) { throw 'Could not run CMake.' }
    if ($helpText -notmatch [regex]::Escape($generator)) { throw "CMake must support $generator. Update CMake and install the Visual Studio C++ tools." }
    $rackBuild = Build-Directory 'HamloRack' $repo
    $targets = if ($selected -eq 'all') { @('HPEQ') + @($plugins.Keys) } else { @($selected) }
    foreach ($target in $targets) {
        if ($target -eq 'HPEQ') {
            Configure-Rack
            Compile-Target $rackBuild 'HPEQ_VST3'
            Copy-Bundle $rackBuild 'HPEQ' 'HamloRack'
        } else {
            $source = Join-Path $repo "Plugins/$target"
            if (!(Test-Path -LiteralPath (Join-Path $source 'CMakeLists.txt'))) { throw "Missing source: $source" }
            $juce = Find-Juce
            if (!$juce) {
                Write-Host 'Preparing shared JUCE with rack configure (no rack VST compilation).'
                Configure-Rack
                $juce = Find-Juce
                if (!$juce -and !$checkOnly) { throw 'Rack configure did not provide JUCE.' }
                if (!$juce) { $juce = Join-Path $rackBuild '_deps/juce-src' }
            }
            $build = Build-Directory $target $source
            Run-Cmake @('-S', $source, '-B', $build, '-G', $generator, '-A', 'x64', "-D$($plugins[$target])=$juce")
            # Complete generated headers first; some pinned Shared targets omit this dependency.
            Compile-Target $build "${target}_Assets"
            Compile-Target $build "${target}_VST3"
            Copy-Bundle $build $target $target
        }
    }
    # Build the requested complete set before installing anything.
    if ($install -and !$checkOnly) {
        $standalone = @($targets | Where-Object { $_ -ne 'HPEQ' })
        if ($standalone.Count) {
            & (Join-Path $PSScriptRoot 'install_hamlo_plugins.ps1') -SourceRoot $output -PluginNames $standalone
            if ($LASTEXITCODE -ne 0) { throw 'Standalone installation failed.' }
        }
        if ($targets -contains 'HPEQ') {
            & (Join-Path $PSScriptRoot 'install_rack.ps1') -BundlePath (Join-Path $output 'HamloRack.vst3')
            if ($LASTEXITCODE -ne 0) { throw 'Rack installation failed.' }
        }
    }
    if ($checkOnly) { Write-Host 'CHECK PASSED - commands only; no compilation or installation was performed.' }
    else { Write-Host "BUILD SUCCEEDED - $output" }
    exit 0
} catch {
    Write-Host ("[FAILED] " + $_.Exception.Message)
    exit 1
}
