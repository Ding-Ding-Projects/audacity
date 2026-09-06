@echo off
setlocal EnableExtensions
rem ---------------------------------------------------------------------------
rem Material Audacity: configure and build RelWithDebInfo into build\windows.
rem
rem Usage:
rem   build.bat        Configure and build, printing full output.
rem   build.bat /s     Silent mode: only warnings, errors and the summary.
rem ---------------------------------------------------------------------------

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%build\windows"
set "INSTALL_DIR=%ROOT%build.install"
set "QT_DIR=%ROOT%build.tools\Qt\6.10.1\msvc2022_64"

set "SILENT=0"
if /I "%~1"=="/s" set "SILENT=1"

echo === Material Audacity build

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: Visual Studio 2022 build tools were not found. Run download-dependencies.bat first.
  exit /b 1
)
set "VS_INSTALL_DIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL_DIR=%%i"
if not defined VS_INSTALL_DIR (
  echo ERROR: the MSVC x64 toolset was not found. Run download-dependencies.bat first.
  exit /b 1
)
call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
  echo ERROR: could not initialise the MSVC environment.
  exit /b 1
)

if exist "%QT_DIR%\bin\qmake.exe" (
  set "PATH=%QT_DIR%\bin;%PATH%"
  set "CMAKE_PREFIX_PATH=%QT_DIR%"
  echo Using Qt at %QT_DIR%
) else (
  echo Using the Qt found on PATH.
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo.
echo === Configure
if "%SILENT%"=="1" (
  cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja ^
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
        -DMUSE_ENABLE_UNIT_TESTS=OFF > "%BUILD_DIR%\configure.log" 2>&1
  if errorlevel 1 (
    echo ERROR: configure failed. See %BUILD_DIR%\configure.log
    exit /b 1
  )
) else (
  cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja ^
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
        -DMUSE_ENABLE_UNIT_TESTS=OFF
  if errorlevel 1 exit /b 1
)

echo.
echo === Build
if "%SILENT%"=="1" (
  cmake --build "%BUILD_DIR%" > "%BUILD_DIR%\build.log" 2>&1
  if errorlevel 1 (
    echo ERROR: build failed. Last lines of %BUILD_DIR%\build.log:
    powershell -NoProfile -Command "Get-Content '%BUILD_DIR%\build.log' -Tail 40"
    exit /b 1
  )
  findstr /I /C:"warning" /C:"error" "%BUILD_DIR%\build.log"
) else (
  cmake --build "%BUILD_DIR%"
  if errorlevel 1 exit /b 1
)

echo.
echo === Install
cmake --install "%BUILD_DIR%" >nul
if errorlevel 1 (
  echo ERROR: install failed.
  exit /b 1
)

echo.
echo === Installing the verified PDF tool bundle beside Audacity4.exe
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%buildscripts\converter-tools\bootstrap-qpdf.ps1" -DestinationRoot "%INSTALL_DIR%\bin" -CacheRoot "%ROOT%build.tools\downloads"
if errorlevel 1 (
  echo ERROR: the installed PDF tool bundle failed verification.
  exit /b 1
)

echo.
echo === Build finished
echo Build tree:   %BUILD_DIR%
echo Install tree: %INSTALL_DIR%
echo Run: "%INSTALL_DIR%\bin\Audacity4.exe"
exit /b 0
