@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "ROOT_DIR=%%~fI"

if not exist "%ROOT_DIR%\thirdParty\vcpkg\vcpkg.exe" (
    echo Error! vcpkg not found. Run setup.bat first.
    exit /b 1
)

:: Initialize VS developer environment (the directx12-agility package finds d3d12.lib through it)
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

cmake --preset vs2026 -S "%ROOT_DIR%"

if errorlevel 1 (
    echo Error! CMake configure failed.
)

if /i "%1"=="/silent" goto :done
echo.
pause
echo.
cmd /k
:done

endlocal
