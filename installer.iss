; Installer Settings for Inno Setup

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{456ABA63-C4BC-4F1F-BF67-7E348FB20ABA}
AppName=RunInLang
AppVerName={cm:AppName} 2.0
AppPublisher=katahiromz
VersionInfoVersion=2.0
VersionInfoTextVersion=2.0
VersionInfoCopyright=Copyright (C) 2023-2025 Katayama Hirofumi MZ.
AppPublisherURL=https://katahiromz.fc2.page/
AppSupportURL=https://katahiromz.fc2.page/runinlang-en/
AppUpdatesURL=https://katahiromz.fc2.page/runinlang-en/
DefaultDirName={pf}\RunInLang
DefaultGroupName={cm:AppName}
OutputDir=.
OutputBaseFilename=runinlang-2.0-setup
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
ShowLanguageDialog=auto

[Languages]
Name: en; MessagesFile: "compiler:Default.isl"
Name: ja; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]                                               
Source: "README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "README_ja.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "build32\Release\RunInLang.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build32\Release\ril32.exe"; DestDir: "{app}"; Flags: ignoreversion 32bit
Source: "build64\Release\ril64.exe"; DestDir: "{app}"; Flags: ignoreversion 64bit; Check: IsWin64

; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\RunInLang"; Filename: "{app}\RunInLang.exe"
Name: "{group}\{cm:ReadMeENG}"; Filename: "{app}\README.txt"
Name: "{group}\{cm:ReadMeJPN}"; Filename: "{app}\README_ja.txt"
Name: "{group}\{cm:License}"; Filename: "{app}\LICENSE.txt"
Name: "{group}\{cm:Uninstall}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\RunInLang"; Filename: "{app}\RunInLang.exe"; Tasks: desktopicon

[CustomMessages]
ja.AppName=言語を指定して実行 (RunInLang)
en.AppName=RunInLang
ja.Authors=片山博文MZ
en.Authors=katahiromz
ja.ReadMeJPN=読んでね (日本語)
en.ReadMeJPN=ReadMe (Japanese)
ja.ReadMeENG=ReadMe (English)
en.ReadMeENG=ReadMe (English)
ja.License=ライセンス
en.License=License
ja.Uninstall=アンインストール
en.Uninstall=Uninstall
