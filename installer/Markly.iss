; Inno Setup script for Markly. Build it with installer\build-installer.ps1, which
; stages the application with windeployqt first and passes the version in.
;
; This file holds Cyrillic text and must stay UTF-8 *with* a byte order mark:
; without it Inno Setup reads the file as ANSI and the Russian strings turn into
; mojibake.

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\build\deploy"
#endif

#define AppName "Markly"
#define AppExe  "Markly.exe"
#define ProgId  "Markly.Markdown"

[Setup]
; Identifies this application to Windows across versions, so a newer installer
; upgrades the existing install instead of adding a second one. Never change it.
AppId={{8866B81A-EB98-412F-966E-F35CAC230617}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
DefaultDirName={autopf}\{#AppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#AppExe}
SetupIconFile=..\resources\markly.ico
OutputDir=output
OutputBaseFilename=Markly-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Installs for the current user by default and needs no administrator rights.
; The wizard offers "for all users" as a choice, and {autopf} and HKA below
; follow whichever is picked.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; Tells Explorer to refresh its file associations once the install is done.
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
english.FileAssociations=File associations
english.AssociateMarkdown=Open Markdown files (.md, .markdown, .mdown, .mkd) with Markly
english.MarkdownDocument=Markdown Document
english.AppDescription=A simple, fast Markdown editor and reader with live preview.
english.OpenDefaultApps=Choose Markly as the default app for Markdown files (opens Windows Settings)
russian.FileAssociations=Ассоциации файлов
russian.AssociateMarkdown=Открывать файлы Markdown (.md, .markdown, .mdown, .mkd) в Markly
russian.MarkdownDocument=Документ Markdown
russian.AppDescription=Простой и быстрый редактор и просмотрщик Markdown с живым предпросмотром.
russian.OpenDefaultApps=Выбрать Markly приложением по умолчанию для файлов Markdown (откроются параметры Windows)

[Tasks]
Name: "associate"; Description: "{cm:AssociateMarkdown}"; GroupDescription: "{cm:FileAssociations}"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\samples\showcase.md"; DestDir: "{app}\samples"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; The document type: what a Markdown file is called, which icon it wears, and the
; command that opens it.
Root: HKA; Subkey: "Software\Classes\{#ProgId}"; ValueType: string; ValueName: ""; ValueData: "{cm:MarkdownDocument}"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\{#ProgId}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExe},0"; Tasks: associate
Root: HKA; Subkey: "Software\Classes\{#ProgId}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""; Tasks: associate

; Becomes the default only for an extension that has none. One that already has
; an owner keeps it; the user is sent to Windows Settings to change that on
; purpose (see [Run]) rather than having it taken from them here.
Root: HKA; Subkey: "Software\Classes\.md"; ValueType: string; ValueName: ""; ValueData: "{#ProgId}"; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate; Check: HasNoDefaultHandler('.md')
Root: HKA; Subkey: "Software\Classes\.markdown"; ValueType: string; ValueName: ""; ValueData: "{#ProgId}"; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate; Check: HasNoDefaultHandler('.markdown')
Root: HKA; Subkey: "Software\Classes\.mdown"; ValueType: string; ValueName: ""; ValueData: "{#ProgId}"; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate; Check: HasNoDefaultHandler('.mdown')
Root: HKA; Subkey: "Software\Classes\.mkd"; ValueType: string; ValueName: ""; ValueData: "{#ProgId}"; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate; Check: HasNoDefaultHandler('.mkd')

; Puts Markly on each extension's "Open with" list. These stay below the default-
; handler entries on purpose: the uninstaller goes through the entries last to first,
; and the extension key can only be removed once OpenWithProgids is gone from it.
Root: HKA; Subkey: "Software\Classes\.md\OpenWithProgids"; ValueType: string; ValueName: "{#ProgId}"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.markdown\OpenWithProgids"; ValueType: string; ValueName: "{#ProgId}"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.mdown\OpenWithProgids"; ValueType: string; ValueName: "{#ProgId}"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.mkd\OpenWithProgids"; ValueType: string; ValueName: "{#ProgId}"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: associate

; Lists Markly under Settings > Default apps, where Windows 10 and 11 let the
; user pick the default for a file type.
Root: HKA; Subkey: "Software\{#AppName}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#AppName}"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#AppName}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "{cm:AppDescription}"; Tasks: associate
Root: HKA; Subkey: "Software\{#AppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".md"; ValueData: "{#ProgId}"; Tasks: associate
Root: HKA; Subkey: "Software\{#AppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".markdown"; ValueData: "{#ProgId}"; Tasks: associate
Root: HKA; Subkey: "Software\{#AppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mdown"; ValueData: "{#ProgId}"; Tasks: associate
Root: HKA; Subkey: "Software\{#AppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mkd"; ValueData: "{#ProgId}"; Tasks: associate
Root: HKA; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "{#AppName}"; ValueData: "Software\{#AppName}\Capabilities"; Flags: uninsdeletevalue; Tasks: associate

; What the "Open with" dialog shows for the executable itself.
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "{#AppName}"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""; Tasks: associate

[Run]
; Windows 10 and 11 do not let an installer take over a file type: the default is
; protected by a hash only Settings can write. So the closing step offers to open
; Settings, where choosing Markly takes one click.
Filename: "ms-settings:defaultapps"; Description: "{cm:OpenDefaultApps}"; Flags: shellexec postinstall skipifsilent; Tasks: associate
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent unchecked

[Code]
// True when nothing, in any hive, claims `Extension` yet. HKEY_CLASSES_ROOT is
// the merged view of the machine-wide and the per-user registrations.
function HasNoDefaultHandler(const Extension: String): Boolean;
var
  Handler: String;
begin
  Result := not RegQueryStringValue(HKEY_CLASSES_ROOT, Extension, '', Handler)
            or (Handler = '');
end;
