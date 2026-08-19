#define MyAppName "ForcedBreak"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Leon Chen"
#define MyAppExeName "ForcedBreak.exe"
#define SourceCodeDir "C:\Users\Leon\Documents\Codes\Company\ForcedBreak"
#define SourceDir "C:\Users\Leon\Documents\Codes\Company\ForcedBreak\ForcedBreak"

[Setup]
; AppId 用来标识同一个程序的不同版本，升级时靠它识别旧版本
; 在 Inno Setup 菜单 Tools > Generate GUID 生成一个，之后版本迭代不要再改
AppId={{C27281B4-213B-4E93-AEC9-6B47E021B909}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir={#SourceCodeDir}\installer
OutputBaseFilename=ForcedBreak_installer_{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "chinese"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "立即运行 {#MyAppName}"; Flags: nowait postinstall skipifsilent