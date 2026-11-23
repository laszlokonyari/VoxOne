[Setup]
AppId={{8E4F3A21-7C9B-4D1E-A520-6B8D4F12C901}
AppName=VoxOne
AppVersion=1.0.0
AppPublisher=lacikapeek
AppPublisherURL=https://github.com/lacikapeek/VoxOne
DefaultDirName={commoncf64}\VST3\VoxOne.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=Output
OutputBaseFilename=VoxOne-1.0.0-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=VoxOne v1.0.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\cmake-build-release\VoxOne_artefacts\Release\VST3\VoxOne.vst3\*"; DestDir: "{commoncf64}\VST3\VoxOne.vst3"; Excludes: "*.pdb,*.ilk,*.exp,*.lib"; Flags: ignoreversion recursesubdirs createallsubdirs