; Create installer for VPython using Inno Setup Compiler (www.innosetup.com)
; Assumes Python and numpy already installed.

; Make sure version numbers are correct for VPython and numpy.

[Setup]
AppName=VPython for Python 2.5
AppVerName=VPython 5.00beta
AppPublisherURL=http://vpython.org
DefaultDirName={code:MyConst}

SourceDir=C:\Python25
DisableProgramGroupPage=yes
DirExistsWarning=no
DisableStartupPrompt=yes
OutputBaseFilename=VPython-Win-Py2.5-5.00beta
OutputDir=c:\workspace

[Files]
; Make sure that config-main.def has
; editor-on-startup and autosave set to 1
; and help set for Visual before building package.
Source: "Lib\idlelib\config-main.def"; DestDir: "{app}\Lib\idlelib\"; Flags: uninsneveruninstall

Source: "c:\workspace\vpython-core2\site-packages\visual\cvisual.pyd"; DestDir: "{app}\Lib\site-packages\visual\"; Components: Visual
Source: "c:\workspace\vpython-core2\site-packages\visual\*.py"; DestDir: "{app}\Lib\site-packages\visual\"; Components: Visual
Source: "c:\workspace\vpython-core2\site-packages\visual\*.tga"; DestDir: "{app}\Lib\site-packages\visual\"; Components: Visual

; Execute compilevisual.py from the CVS files to compile the .pyc files:
Source: "Lib\site-packages\visual\*.pyc"; DestDir: "{app}\Lib\site-packages\visual\"; Components: Visual

Source: "c:\workspace\vpython-core2\license.txt"; DestDir: "{app}\Lib\site-packages\visual\"; Components: Visual

; Need to have installed numpy before building Visual, so components available in site-packages:
Source: "Lib\site-packages\numpy*egg-info"; DestDir: "{app}\Lib\site-packages\"; Components: numpy
Source: "Lib\site-packages\numpy\*"; DestDir: "{app}\Lib\site-packages\numpy\"; Components: numpy; Flags: recursesubdirs
;
Source: "c:\workspace\vpython-core2\examples\*.py"; DestDir: "{app}\Lib\site-packages\visual\examples\"; Components: Examples
Source: "c:\workspace\vpython-core2\examples\*.tga"; DestDir: "{app}\Lib\site-packages\visual\examples\"; Components: Examples
;
Source: "c:\workspace\vpython-core2\docs\index.html"; DestDir: "{app}\Lib\site-packages\visual\docs\"; Components: Documentation
Source: "c:\workspace\vpython-core2\docs\visual\*.html"; DestDir: "{app}\Lib\site-packages\visual\docs\visual\"; Components: Documentation
Source: "c:\workspace\vpython-core2\docs\visual\*.gif"; DestDir: "{app}\Lib\site-packages\visual\docs\visual\"; Components: Documentation
Source: "c:\workspace\vpython-core2\docs\visual\*.css"; DestDir: "{app}\Lib\site-packages\visual\docs\visual\"; Components: Documentation
Source: "c:\workspace\vpython-core2\docs\visual\*.txt"; DestDir: "{app}\Lib\site-packages\visual\docs\visual\"; Components: Documentation
Source: "c:\workspace\vpython-core2\docs\visual\images\*.jpg"; DestDir: "{app}\Lib\site-packages\visual\docs\visual\images"; Components: Documentation

[Components]
Name: Visual; Description: "The Visual extension module for Python"; Types: full compact custom; Flags: fixed
Name: Documentation; Description: "Documentation for the Visual extension to Python"; Types: full
Name: Examples; Description: "Example programs"; Types: full
Name: numpy; Description: "numpy 1.2.0 {code:NumpyStatus|C:\Python25}"; Types: full; Check: CheckNumpy( 'C:\Python25' )

[Tasks]
Name: desktopicon; Description: "Create a desktop icon";

[Icons]
Name: "{userdesktop}\IDLE for VPython"; Filename: "{app}\pythonw.exe"; Parameters: "{app}\Lib\idlelib\idle.pyw"; WorkingDir: "{app}\Lib\site-packages\visual\examples"; IconFilename: "{app}\DLLs\py.ico"; Tasks: desktopicon

; This code file contains a ShouldSkipPage function which looks
; for an appropriate version of python.exe,
; and if it is found we skip the "select a directory" page.

[Code]
program Setup;

// Try to discover where Python is actually installed.
function MyConst(Param: String): String;
var Exist1, Exist2: Boolean;
begin
    Exist1 := FileExists( ExpandConstant('{reg:HKLM\Software\Python\PythonCore\2.5\InstallPath,}\python.exe'));
    if Exist1 then
      Result := ExpandConstant('{reg:HKLM\Software\Python\PythonCore\2.5\InstallPath,}')
    else
      begin
      Exist2 := FileExists( ExpandConstant('{reg:HKCU\Software\Python\PythonCore\2.5\InstallPath,}\python.exe'));
      if Exist2 then
        Result := ExpandConstant('{reg:HKCU\Software\Python\PythonCore\2.5\InstallPath,}')
      else
        Result := 'C:\'
      end
end;

function ShouldSkipPage(CurPage: Integer): Boolean;
var Result1, Result2: Boolean;
begin
  case CurPage of
    wpSelectDir:
      begin
      Result1 := FileExists( ExpandConstant('{reg:HKLM\Software\Python\PythonCore\2.5\InstallPath,}\python.exe'));
      Result2 := FileExists( ExpandConstant('{reg:HKCU\Software\Python\PythonCore\2.5\InstallPath,}\python.exe'));
      Result := Result1 or Result2
      if not Result then
         MsgBox('Could not locate where Python 2.5 is installed.' #13 'You will be asked where python.exe is located.', mbInformation, MB_OK);
      end
    else
      Result := False;
  end;
end;

// Need a function to determine if numpy is already installed.
function NumpyAvailable( BasePath: String): Boolean;
begin
  Result := DirExists( BasePath + '\Lib\site-packages\numpy');
end;

// Choose a modifying string for the user-visible numpy component.
function NumpyStatus( Param: String): String;
begin
  if NumpyAvailable( Param) then
    Result := '(found)'
  else
    Result := '(Numpy must be selected)'
end;

// Don't clobber an existing installation of numpy
function CheckNumpy( Param: String): Boolean;
begin
  Result := not NumpyAvailable( Param);
end;

