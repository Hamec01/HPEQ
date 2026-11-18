@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "HAMLO_INSTALL_BATCH=%~f0"
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -Command "$ErrorActionPreference = 'Stop'; try { $batch = $env:HAMLO_INSTALL_BATCH; $body = ([IO.File]::ReadAllText($batch) -split '(?m)^# PowerShell\r?$', 2)[1]; & ([scriptblock]::Create($body)) (Join-Path (Split-Path -LiteralPath $batch) 'batvst') } catch { Write-Host ('[ERROR] ' + $_.Exception.Message); exit 1 }"
set "HAMLO_INSTALL_RESULT=%errorlevel%"
echo.
pause
exit /b %HAMLO_INSTALL_RESULT%

# PowerShell
param([string]$SourceRoot)

function Install-HamloBundles {
    param([string]$SourceRoot, [string]$CommonFiles)

    $ErrorActionPreference = 'Stop'
    $plugins = @(
        'HamloComp', 'HamloEcho', 'HamloFader', 'HamloMag', 'HamloRedWall',
        'HamloRevolution', 'HamloShirota', 'HamloUdarnik', 'HamloVerb', 'HamloZastava'
    ) | ForEach-Object { "$_.vst3" }

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

    foreach ($plugin in $plugins) {
        $binary = Join-Path $SourceRoot "$plugin\Contents\x86_64-win\$plugin"
        if (!(Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Missing VST3 binary: $binary" }
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
    try {
        [void][IO.Directory]::CreateDirectory($incoming)
        [void][IO.Directory]::CreateDirectory($backup)
        foreach ($plugin in $plugins) {
            Copy-Item -LiteralPath (Join-Path $SourceRoot $plugin) -Destination $incoming -Recurse -Force
        }

        [void][IO.Directory]::CreateDirectory($destination)
        foreach ($plugin in $plugins) {
            $target = Join-Path $destination $plugin
            $staged = Join-Path $incoming $plugin
            $previous = Join-Path $backup $plugin
            $entry = [pscustomobject]@{ Target = $target; Staged = $staged; Previous = $previous; Saved = $false; Installed = $false }
            $changed.Add($entry)
            if (Test-Path -LiteralPath $target) {
                Move-Bundle $target $previous
                $entry.Saved = $true
            }
            Move-Bundle $staged $target
            $entry.Installed = $true
        }
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
        if (!$keepBackup -and (Test-Path -LiteralPath $work)) {
            $item = Get-Item -LiteralPath $work -Force
            if ($item.FullName -eq $work -and $item.Parent.FullName -eq $common -and !($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction Continue
            }
        }
    }
    Write-Output "Installed all $($plugins.Count) Hamlo VST3 plug-ins in $destination."
}

function Invoke-HamloInstaller {
    param([string]$SourceRoot, [string]$CommonFiles)

    $mutex = [Threading.Mutex]::new($false, 'Global\HamloVst3Installer')
    $locked = $false
    try {
        try { $locked = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $locked = $true }
        if (!$locked) { throw 'A Hamlo VST3 installation is already running.' }
        Install-HamloBundles $SourceRoot $CommonFiles
    } finally {
        if ($locked) { $mutex.ReleaseMutex() }
        $mutex.Dispose()
    }
}

$commonFiles = $env:CommonProgramW6432
if (!$commonFiles) { $commonFiles = [Environment]::GetFolderPath('CommonProgramFiles') }
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Invoke-HamloInstaller $SourceRoot $commonFiles
    return
}

Write-Host 'Confirm the Windows administrator prompt to install the Hamlo VST3 plug-ins.'
$log = [IO.Path]::GetTempFileName()
try {
    $payload = @"
function Install-HamloBundles { ${function:Install-HamloBundles} }
function Invoke-HamloInstaller { ${function:Invoke-HamloInstaller} }
try {
    Invoke-HamloInstaller '$($SourceRoot.Replace("'", "''"))' '$($commonFiles.Replace("'", "''"))' *> '$($log.Replace("'", "''"))'
} catch {
    Add-Content -LiteralPath '$($log.Replace("'", "''"))' -Value ('[ERROR] ' + `$_.Exception.Message)
    exit 1
}
"@
    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($payload))
    $process = Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') -Verb RunAs -WindowStyle Hidden -Wait -PassThru -ArgumentList '-NoProfile', '-NonInteractive', '-EncodedCommand', $encoded
    Get-Content -LiteralPath $log
    exit $process.ExitCode
} finally {
    Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
}
