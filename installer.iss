[Setup]
AppName=La Pauta AP550 Pro
AppVersion=1.0.0
AppPublisher=La Pauta
DefaultDirName={autopf}\La Pauta\AP550 Pro
DefaultGroupName=La Pauta
OutputDir=installer_out
OutputBaseFilename=La-Pauta-AP550-Pro-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayIcon={app}\La Pauta AP550 Pro.exe

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full"; Description: "Instalación completa"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3"; Types: full custom
Name: "standalone"; Description: "Aplicación independiente"; Types: full custom

[Files]
Source: "build\LaPautaAP550Pro_artefacts\Release\VST3\La Pauta AP550 Pro.vst3\*"; DestDir: "{commoncf64}\VST3\La Pauta AP550 Pro.vst3"; Flags: recursesubdirs ignoreversion; Components: vst3
Source: "build\LaPautaAP550Pro_artefacts\Release\Standalone\La Pauta AP550 Pro.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{group}\La Pauta AP550 Pro"; Filename: "{app}\La Pauta AP550 Pro.exe"; Components: standalone
Name: "{group}\Desinstalar"; Filename: "{uninstallexe}"
