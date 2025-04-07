@echo off
echo mtasa-blue-1.5.9-GTA-Province-0.1.5
echo by: e1ectr0venik
echo.

rem Update CEF eventually
utils\premake5.exe install_cef

rem Update Unifont
utils\premake5.exe install_unifont

rem Generate solutions
utils\premake5.exe vs2019

rem Create a shortcut to the solution
set SCRIPTFILE="%TEMP%\CreateMyShortcut.vbs"
(
  echo Set oWS = WScript.CreateObject^("WScript.Shell"^)
  echo sLinkFile = oWS.ExpandEnvironmentStrings^("MTASA.sln - Shortcut.lnk"^)
  echo Set oLink = oWS.CreateShortcut^(sLinkFile^)
  echo oLink.TargetPath = oWS.ExpandEnvironmentStrings^("%~dp0\Build\MTASA.sln"^)
  echo oLink.Save
) 1>%SCRIPTFILE%
cscript //nologo %SCRIPTFILE%
del /f /q %SCRIPTFILE%

if %0 == "%~0" pause