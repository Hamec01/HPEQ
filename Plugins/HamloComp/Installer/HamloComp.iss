; HamloComp VST3 installer, Windows x64. Build it with Installer\Build-Installer.ps1 (it passes the version from
; CMakeLists.txt and the paths below).
#ifndef MyAppVersion
  #error Build the installer with Installer\Build-Installer.ps1
#endif
#ifndef SourceVst3Dir
#define SourceVst3Dir "..\build\HamloComp_artefacts\Release\VST3\HamloComp.vst3"
#endif
#ifndef OutputDir
#define OutputDir "..\dist"
#endif
#ifndef VCRedistFile
#define VCRedistFile "..\build\prerequisites\vc_redist.x64.exe"
#endif
#ifndef JuceLicense
#define JuceLicense "..\build\JUCE-LICENSE.md"
#endif
#define RuntimeRequiredVersion GetFileVersion(VCRedistFile)
[Setup]
AppId={{0C994CAA-AD7D-4725-B8F1-4FA33F5CE56F}
AppName=HamloComp
AppVersion={#MyAppVersion}
AppVerName=HamloComp {#MyAppVersion}
AppPublisher=HamloProd
AppPublisherURL=https://hamloprod.org
VersionInfoVersion={#MyFileVersion}
VersionInfoCompany=HamloProd
VersionInfoCopyright=Copyright (C) 2026 HamloProd
PrivilegesRequired=admin
OutputBaseFilename=HamloComp_VST3_Setup_{#MyAppVersion}
DefaultDirName={commoncf64}\VST3\HamloComp.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputDir}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
UninstallDisplayIcon={app}\Contents\x86_64-win\HamloComp.vst3
InfoAfterFile=..\docs\release\INSTALL-FINISH.txt
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
[Files]
Source: "{#SourceVst3Dir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\docs\release\README.txt"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "..\docs\release\THIRD-PARTY-NOTICES.txt"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "{#JuceLicense}"; DestDir: "{app}\Documentation\ThirdParty\JUCE"; DestName: "LICENSE.md"; Flags: ignoreversion
Source: "{#VCRedistFile}"; Flags: dontcopy
[Icons]
Name: "{commonprograms}\HamloProd\HamloComp README"; Filename: "{app}\Documentation\README.txt"
Name: "{commonprograms}\HamloProd\Uninstall HamloComp"; Filename: "{uninstallexe}"
[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ExitCode: Integer;
  RuntimeVersion: String;
  Installed: Cardinal;
  InstalledVersion, RequiredVersion: Int64;
begin
  Result := '';
  if RegQueryDWordValue(HKLM64, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed', Installed) and (Installed = 1) then
    if RegQueryStringValue(HKLM64, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Version', RuntimeVersion) then
      if StrToVersion(Copy(RuntimeVersion, 2, Length(RuntimeVersion)), InstalledVersion) and StrToVersion('{#RuntimeRequiredVersion}', RequiredVersion) then
        if ComparePackedVersion(InstalledVersion, RequiredVersion) >= 0 then Exit;
  ExtractTemporaryFile('vc_redist.x64.exe');
  if not Exec(ExpandConstant('{tmp}\vc_redist.x64.exe'), '/install /quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, ExitCode) then
  begin
    Result := 'Could not start Microsoft Visual C++ Runtime setup.';
    Exit;
  end;
  if ExitCode = 3010 then NeedsRestart := True
  else if (ExitCode <> 0) and (ExitCode <> 1638) then
    Result := Format('Microsoft Visual C++ Runtime setup failed (code %d).', [ExitCode]);
end;
