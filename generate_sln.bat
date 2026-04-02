@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "ROOT_DIR=%%~fI"

set "BUILD_DIR=%ROOT_DIR%\build"
set "VCPKG_ROOT=%ROOT_DIR%\thirdParty\vcpkg"
set "VCPKG_INSTALLED=%ROOT_DIR%\thirdParty\vcpkg_installed"

if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error! vcpkg not found. Run setup.bat first.
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Initialize VS developer environment
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error! vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)
for /f "tokens=*" %%i in ('"%VSWHERE%" -latest -property installationPath') do set "VS_PATH=%%i"
if not exist "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Error! vcvarsall.bat not found.
    exit /b 1
)
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Running CMake configure...

cmake -B %BUILD_DIR% -S . ^
    -G "Visual Studio 18 2026" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DVCPKG_INSTALLED_DIR="%VCPKG_INSTALLED%"

if errorlevel 1 (
    echo Error! CMake configure failed.
    exit /b 1
)

echo.
pause

echo.
cmd /k

endlocal