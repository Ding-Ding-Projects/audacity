@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem ---------------------------------------------------------------------------
rem Material Audacity: install and verify the Windows build dependencies.
rem
rem Installs Qt 6.10.1 with aqtinstall, checks that the Visual Studio 2022 C++
rem build tools are present, and fetches ninja, nuget and the pinned
rem squirrel.windows package used by the installer.
rem
rem Usage: download-dependencies.bat [/s]
rem ---------------------------------------------------------------------------

set "REQUESTED_SILENT=%SILENT%"
set "SILENT=0"
set "BUILD_ONLY=0"
if /I "%REQUESTED_SILENT%"=="1" set "SILENT=1"
if /I "%~1"=="/s" set "SILENT=1"
if /I "%~1"=="--silent" set "SILENT=1"
if /I "%~1"=="/build" set "BUILD_ONLY=1"
if /I "%~2"=="/build" set "BUILD_ONLY=1"

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
echo === Bootstrapping Python
where python >nul 2>&1
if errorlevel 1 (
  call :InstallWithWinget Python.Python.3.12 "Python 3.12"
  if errorlevel 1 exit /b 1
)
where python >nul 2>&1
if errorlevel 1 if exist "%LocalAppData%\Programs\Python\Python312\python.exe" set "PATH=%LocalAppData%\Programs\Python\Python312;%PATH%"
where python >nul 2>&1
if errorlevel 1 (
  echo ERROR: Python bootstrap completed but Python 3.12 is not reachable in this process.
  exit /b 1
)
python --version

echo.
echo === Bootstrapping the Visual Studio 2022 C++ build tools
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  call :InstallWithWinget Microsoft.VisualStudio.2022.BuildTools "Visual Studio 2022 Build Tools"
  if errorlevel 1 exit /b 1
)
set "VS_INSTALL_DIR="
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL_DIR=%%i"
if not defined VS_INSTALL_DIR (
  call :InstallWithWinget Microsoft.VisualStudio.2022.BuildTools "Visual Studio C++ workload"
  if errorlevel 1 exit /b 1
  set "VS_INSTALL_DIR="
  if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL_DIR=%%i"
)
if not defined VS_INSTALL_DIR (
  echo ERROR: the Visual Studio C++ workload is unavailable after automatic bootstrap.
  exit /b 1
)
echo Found: %VS_INSTALL_DIR%

echo.
echo === Bootstrapping CMake
where cmake >nul 2>&1
if errorlevel 1 (
  call :InstallWithWinget Kitware.CMake "CMake"
  if errorlevel 1 exit /b 1
)
where cmake >nul 2>&1
if errorlevel 1 if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles%\CMake\bin;%PATH%"
where cmake >nul 2>&1
if errorlevel 1 (
  echo ERROR: CMake bootstrap completed but CMake is not reachable in this process.
  exit /b 1
)
cmake --version

echo.
echo === Installing aqtinstall and ninja
python -m pip install --user --upgrade --disable-pip-version-check aqtinstall ninja
if errorlevel 1 (
  echo ERROR: pip install failed.
  exit /b 1
)
for /f "usebackq tokens=*" %%i in (`python -c "import sysconfig; print(sysconfig.get_path('scripts'))"`) do set "PYTHON_SCRIPTS=%%i"
if defined PYTHON_SCRIPTS set "PATH=%PYTHON_SCRIPTS%;%PATH%"
where ninja >nul 2>&1
if errorlevel 1 (
  echo ERROR: ninja was installed but is not reachable in this process.
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
if "%BUILD_ONLY%"=="1" (
  echo Skipping installer-only Squirrel tools for the application build.
  goto dependencies_ready
)
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%ROOT%buildscripts\ci\windows\fetch_squirrel_tools.ps1" -Root "%ROOT%." -ToolsDir "%TOOLS%\squirrel"
if errorlevel 1 (
  echo ERROR: the packaging tool download failed.
  exit /b 1
)

:dependencies_ready
echo.
echo === All dependencies are ready
echo Qt:        %QT_ROOT%\%QT_VERSION%\msvc2022_64
echo Packaging: %TOOLS%\squirrel
echo Next: build.bat /s
exit /b 0

:InstallWithWinget
set "PACKAGE_ID=%~1"
set "PACKAGE_NAME=%~2"
where winget >nul 2>&1
if errorlevel 1 (
  echo ERROR: %PACKAGE_NAME% is missing and winget is unavailable for automatic bootstrap.
  exit /b 1
)
echo Installing %PACKAGE_NAME% with winget...
if /I "%PACKAGE_ID%"=="Microsoft.VisualStudio.2022.BuildTools" (
  winget install --id %PACKAGE_ID% --exact --silent --disable-interactivity --accept-package-agreements --accept-source-agreements --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
) else (
  winget install --id %PACKAGE_ID% --exact --silent --disable-interactivity --accept-package-agreements --accept-source-agreements
)
if errorlevel 1 (
  echo ERROR: automatic bootstrap of %PACKAGE_NAME% failed.
  exit /b 1
)
exit /b 0
