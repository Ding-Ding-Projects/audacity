@echo off
setlocal EnableExtensions
rem ---------------------------------------------------------------------------
rem Material Audacity: build the unsigned Squirrel.Windows installer locally.
rem
rem Requires a completed build.bat run, which leaves the application in
rem build\install. Output goes to dist\squirrel-windows.
rem
rem Usage:
rem   build-installer.bat        Full output.
rem   build-installer.bat /s     Silent mode: log to file, print the summary.
rem ---------------------------------------------------------------------------

set "ROOT=%~dp0"
set "INSTALL_DIR=%ROOT%build.install"
set "OUT_DIR=%ROOT%dist\squirrel-windows"
set "SCRIPT=%ROOT%buildscripts\ci\windows\package_squirrel.ps1"
set "LOG=%ROOT%build\squirrel-package.log"

set "SILENT=0"
if /I "%~1"=="/s" set "SILENT=1"

echo === Material Audacity installer

if not exist "%INSTALL_DIR%" (
  echo ERROR: %INSTALL_DIR% not found. Run build.bat first.
  exit /b 1
)
if not exist "%SCRIPT%" (
  echo ERROR: %SCRIPT% not found.
  exit /b 1
)

set "PS=powershell"
where pwsh >nul 2>&1
if not errorlevel 1 set "PS=pwsh"

if not exist "%ROOT%build" mkdir "%ROOT%build"

if "%SILENT%"=="1" (
  %PS% -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" -InstallDir "%INSTALL_DIR%" -OutDir "%OUT_DIR%" > "%LOG%" 2>&1
  if errorlevel 1 (
    echo ERROR: packaging failed. Last lines of %LOG%:
    powershell -NoProfile -Command "Get-Content '%LOG%' -Tail 40"
    exit /b 1
  )
) else (
  %PS% -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" -InstallDir "%INSTALL_DIR%" -OutDir "%OUT_DIR%"
  if errorlevel 1 exit /b 1
)

echo.
echo === Installer ready in %OUT_DIR%
dir /b "%OUT_DIR%"
echo.
echo The installer is intentionally unsigned. Verify with:
echo   Get-FileHash "%OUT_DIR%\Setup.exe" -Algorithm SHA256
echo   (Get-AuthenticodeSignature "%OUT_DIR%\Setup.exe").Status
exit /b 0
