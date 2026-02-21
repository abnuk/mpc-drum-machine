;
; Beatwerk — Windows Installer (Inno Setup)
;
; Build from repo root:
;   iscc installer\windows\beatwerk.iss /DAppVersion=1.1.0
;
; Or use the helper script:
;   powershell installer\windows\create_installer.ps1 -Build
;

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif

#define AppName      "Beatwerk"
#define AppPublisher "Beatwerk"
#define AppURL       "https://github.com/beatwerk/beatwerk"
#define AppExeName   "Beatwerk.exe"

[Setup]
AppId={{B3A7E2F1-4D5C-4E6F-8A9B-1C2D3E4F5A6B}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\..\installer\output
OutputBaseFilename={#AppName}-{#AppVersion}-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallMode=x64compatible
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExeName}

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone Application"; Types: full custom; Flags: fixed
Name: "vst3"; Description: "VST3 Plugin"; Types: full custom

[Files]
Source: "..\..\build\Beatwerk_artefacts\Release\Standalone\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; Components: standalone
Source: "..\..\build\Beatwerk_artefacts\Release\VST3\Beatwerk.vst3\*"; DestDir: "{commoncf}\VST3\Beatwerk.vst3"; Flags: ignoreversion recursesubdirs; Components: vst3

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Components: standalone
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon; Components: standalone
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Components: standalone

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent; Components: standalone
