@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem ---------------------------------------------------------------------------
rem Material Audacity: install and verify the Windows build dependencies.
rem
rem Installs Qt 6.10.1 with aqtinstall, checks that the Visual Studio 2022 C++
rem build tools are present, and fetches ninja, nuget and the pinned
rem squirrel.windows package used by the installer.
rem
rem Usage: download-dependencies.bat
rem ---------------------------------------------------------------------------

set "ROOT=%~dp0"
set "TOOLS=%ROOT%build.tools"
set "QT_ROOT=%TOOLS%\Qt"
set "QT_VERSION=6.10.1"
set "QT_ARCH=win64_msvc2022_64"
set "QT_MODULES=qt5compat qtnetworkauth qtshadertools qtwebsockets qtgraphs qtquick3d"

echo === Material Audacity dependency setup
echo Root: %ROOT%

if not exist "%TOOLS%" mkdir "%TOOLS%"

echo.
echo === Checking Python
where python >nul 2>&1
if errorlevel 1 (
  echo ERROR: Python was not found on PATH.
  echo Install Python 3.10 or newer from https://www.python.org/downloads/windows/
  exit /b 1
)
python --version

echo.
echo === Checking the Visual Studio 2022 C++ build tools
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: vswhere.exe was not found.
  echo Install "Visual Studio 2022 Build Tools" with the
  echo "Desktop development with C++" workload from
  echo https://visualstudio.microsoft.com/downloads/
  exit /b 1
)
set "VS_INSTALL_DIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL_DIR=%%i"
if not defined VS_INSTALL_DIR (
  echo ERROR: the MSVC x64 toolset was not found.
  echo Add the "Desktop development with C++" workload to Visual Studio 2022.
  exit /b 1
)
echo Found: %VS_INSTALL_DIR%

echo.
echo === Checking CMake
where cmake >nul 2>&1
if errorlevel 1 (
  echo ERROR: cmake was not found on PATH. Install CMake 3.24 or newer.
  exit /b 1
)
cmake --version

echo.
echo === Installing aqtinstall and ninja
python -m pip install --upgrade --disable-pip-version-check aqtinstall ninja
if errorlevel 1 (
  echo ERROR: pip install failed.
  exit /b 1
)

echo.
echo === Installing Qt %QT_VERSION% (%QT_ARCH%)
if exist "%QT_ROOT%\%QT_VERSION%\msvc2022_64\bin\qmake.exe" (
  echo Qt is already present at %QT_ROOT%\%QT_VERSION%\msvc2022_64
) else (
  python -m aqt install-qt windows desktop %QT_VERSION% %QT_ARCH% -O "%QT_ROOT%" -m %QT_MODULES%
  if errorlevel 1 (
    echo ERROR: the Qt installation failed.
    exit /b 1
  )
)

echo.
echo === Fetching nuget.exe and the pinned squirrel.windows package
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop';" ^
  "$lock = Get-Content '%ROOT%buildscripts\packaging\Windows\Squirrel\squirrel.lock.json' -Raw | ConvertFrom-Json;" ^
  "$dir = '%TOOLS%\squirrel'; New-Item -ItemType Directory -Force -Path $dir | Out-Null;" ^
  "function Fetch($url, $sha, $dest) {" ^
  "  if (Test-Path $dest) { if ((Get-FileHash $dest -Algorithm SHA256).Hash -ieq $sha) { Write-Host \"Reusing $dest\"; return } }" ^
  "  Write-Host \"Downloading $url\"; Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing;" ^
  "  $a = (Get-FileHash $dest -Algorithm SHA256).Hash;" ^
  "  if ($a -ine $sha) { throw \"SHA256 mismatch for $url\" }; Write-Host \"Verified $a\" }" ^
  "Fetch $lock.nuget.url $lock.nuget.sha256 (Join-Path $dir 'nuget.exe');" ^
  "Fetch $lock.squirrel.url $lock.squirrel.sha256 (Join-Path $dir ('squirrel.windows.' + $lock.squirrel.version + '.nupkg'))"
if errorlevel 1 (
  echo ERROR: the packaging tool download failed.
  exit /b 1
)

echo.
echo === All dependencies are ready
echo Qt:        %QT_ROOT%\%QT_VERSION%\msvc2022_64
echo Packaging: %TOOLS%\squirrel
echo Next: build.bat /s
exit /b 0
