@echo off
echo mtasa-blue-1.5.9-GTA-Province-0.1.5
echo by: e1ectr0venik
echo.

rem Install data files
utils\premake5 install_data

rem Optionally install resources
echo.
set ANSWER=n
set /P ANSWER="Install resources? [y/N] "
if /I "%ANSWER%"=="y" (
    utils\premake5 install_resources
)

if %0 == "%~0" pause
