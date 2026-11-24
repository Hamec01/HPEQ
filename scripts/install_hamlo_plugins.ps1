param([string]$SourceRoot = '', [string[]]$PluginNames = @('HamloComp','HamloEcho','HamloFader','HamloMag','HamloRedWall','HamloRevolution','HamloShirota','HamloUdarnik','HamloVerb','HamloZastava'), [string[]]$DeprecatedNames = @(), [switch]$FunctionsOnly)

function Install-HamloBundles {
    param([string]$SourceRoot, [string]$CommonFiles, [string[]]$PluginNames, [string[]]$DeprecatedNames = @())

    $ErrorActionPreference = 'Stop'
    $allowed = @('HamloComp','HamloEcho','HamloFader','HamloMag','HamloRedWall','HamloRevolution','HamloShirota','HamloUdarnik','HamloVerb','HamloZastava') + @('HamloRack')
    if (!$PluginNames.Count) { throw 'No plug-ins selected.' }
    foreach ($name in $PluginNames) { if ($allowed -notcontains $name) { throw "Unknown plug-in: $name" } }
    foreach ($name in $DeprecatedNames) { if ($name -ne 'HPEQ') { throw "Unknown deprecated bundle: $name" } }
    $plugins = @($PluginNames | Select-Object -Unique | ForEach-Object { "$_.vst3" })
    $allBundles = @($plugins + @($DeprecatedNames | ForEach-Object { "$_.vst3" }) | Select-Object -Unique)


    function Move-Bundle([string]$From, [string]$To) {
        $item = Get-Item -LiteralPath $From -Force
        if ($item.FullName -ne $From -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Unexpected VST3 path: $From"
        }
        if ($item.PSIsContainer) { [IO.Directory]::Move($From, $To) }
        else { [IO.File]::Move($From, $To) }
    }

    $common = (Get-Item -LiteralPath $CommonFiles).FullName
    $destination = Join-Path $common 'VST3'
    if (Test-Path -LiteralPath $destination) {
        $item = Get-Item -LiteralPath $destination -Force
        if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Expected a regular VST3 directory at $destination."
        }
    }

    foreach ($plugin in $allBundles) {
        $binary = Join-Path $SourceRoot "$plugin\Contents\x86_64-win\$plugin"
        if (($plugins -contains $plugin) -and !(Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Missing VST3 binary: $binary" }
        $old = Get-Item -LiteralPath (Join-Path $destination $plugin) -Force -ErrorAction SilentlyContinue
        if ($old -and ($old.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Existing VST3 is a link: $($old.FullName)"
        }
    }

    # Prepare the complete set next to VST3 so the final moves stay on the same volume.
    $work = Join-Path $common ('Hamlo-VST3-install-' + [Guid]::NewGuid().ToString('N'))
    $incoming = Join-Path $work 'incoming'
    $backup = Join-Path $work 'backup'
    $changed = [Collections.Generic.List[object]]::new()
    $keepBackup = $false
    $succeeded = $false
    try {
        [void][IO.Directory]::CreateDirectory($incoming)
        [void][IO.Directory]::CreateDirectory($backup)
        foreach ($plugin in $plugins) {
            Copy-Item -LiteralPath (Join-Path $SourceRoot $plugin) -Destination $incoming -Recurse -Force
        }

        [void][IO.Directory]::CreateDirectory($destination)
        foreach ($plugin in $allBundles) {
            $target = Join-Path $destination $plugin
            $staged = Join-Path $incoming $plugin
            $previous = Join-Path $backup $plugin
            $entry = [pscustomobject]@{ Target = $target; Staged = $staged; Previous = $previous; Saved = $false; Installed = $false }
            $changed.Add($entry)
            if (Test-Path -LiteralPath $target) {
                Move-Bundle $target $previous
                $entry.Saved = $true
            }
            if ($plugins -contains $plugin) {
                Move-Bundle $staged $target
                $entry.Installed = $true
            }
        }
        $succeeded = $true
    } catch {
        foreach ($entry in $changed) {
            try {
                if ($entry.Installed) { Move-Bundle $entry.Target $entry.Staged }
                if ($entry.Saved) { Move-Bundle $entry.Previous $entry.Target }
            } catch {
                $keepBackup = $true
                Write-Warning "Could not restore $($entry.Target). Recover the previous bundle from $backup."
            }
        }
        throw
    } finally {
        if ($succeeded -and @($changed | Where-Object { $_.Saved }).Count) { $keepBackup = $true }
        if (!$keepBackup -and (Test-Path -LiteralPath $work)) {
            $item = Get-Item -LiteralPath $work -Force
            if ($item.FullName -eq $work -and $item.Parent.FullName -eq $common -and !($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction Continue
            }
        }
    }
    if ($keepBackup) { Write-Output "Previous bundles retained in $backup" }
    Write-Output "Installed $($plugins.Count) Hamlo VST3 plug-ins in $destination."
}

function Invoke-HamloInstaller {
    param([string]$SourceRoot, [string]$CommonFiles, [string[]]$PluginNames, [string[]]$DeprecatedNames = @())

    $mutex = [Threading.Mutex]::new($false, 'Global\HamloVst3Installer')
    $locked = $false
    try {
        try { $locked = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $locked = $true }
        if (!$locked) { throw 'A Hamlo VST3 installation is already running.' }
        Install-HamloBundles $SourceRoot $CommonFiles $PluginNames $DeprecatedNames
    } finally {
        if ($locked) { $mutex.ReleaseMutex() }
        $mutex.Dispose()
    }
}

if ($FunctionsOnly) { return }
if (!$SourceRoot) { $SourceRoot = Join-Path (Split-Path -Parent $PSScriptRoot) 'batvst' }
$commonFiles = $env:CommonProgramW6432
if (!$commonFiles) { $commonFiles = [Environment]::GetFolderPath('CommonProgramFiles') }
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Invoke-HamloInstaller $SourceRoot $commonFiles $PluginNames $DeprecatedNames
    return
}

Write-Host 'Confirm the Windows administrator prompt to install the Hamlo VST3 plug-ins.'
$log = [IO.Path]::GetTempFileName()
try {
    $nameLiteral = '@(' + (($PluginNames | ForEach-Object { "'" + $_.Replace("'", "''") + "'" }) -join ',') + ')'
    $oldLiteral = '@(' + (($DeprecatedNames | ForEach-Object { "'" + $_.Replace("'", "''") + "'" }) -join ',') + ')'
    $payload = @"
function Install-HamloBundles { ${function:Install-HamloBundles} }
function Invoke-HamloInstaller { ${function:Invoke-HamloInstaller} }
try {
    Invoke-HamloInstaller '$($SourceRoot.Replace("'", "''"))' '$($commonFiles.Replace("'", "''"))' $nameLiteral $oldLiteral *> '$($log.Replace("'", "''"))'
} catch {
    Add-Content -LiteralPath '$($log.Replace("'", "''"))' -Value ('[ERROR] ' + `$_.Exception.Message)
    exit 1
}
"@
    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($payload))
    $process = Start-Process -FilePath (Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe') -Verb RunAs -WindowStyle Hidden -Wait -PassThru -ArgumentList '-NoProfile', '-NonInteractive', '-EncodedCommand', $encoded
    Get-Content -LiteralPath $log
    exit $process.ExitCode
} finally {
    Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
}
