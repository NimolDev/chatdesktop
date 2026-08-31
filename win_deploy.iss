[Setup]
AppName=ChatApp
AppVersion=1.0
DefaultDirName={autopf}\ChatApp
DefaultGroupName=ChatApp
OutputBaseFilename=ChatAppSetup
Compression=lzma
SolidCompression=yes

[Files]
Source: "release\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\ChatApp"; Filename: "{app}\ChatApp.exe"
Name: "{autodesktop}\ChatApp"; Filename: "{app}\ChatApp.exe"

[Run]
Filename: "{app}\ChatApp.exe"; Description: "Launch ChatApp"; Flags: nowait postinstall skipifsilent