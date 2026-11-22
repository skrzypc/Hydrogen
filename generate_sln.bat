@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "ROOT_DIR=%%~fI"

if not exist "%ROOT_DIR%\thirdParty\vcpkg\vcpkg.exe" (
    echo Error! vcpkg not found. Run setup.bat first.
    goto :fail
)

:: Initialize VS developer environment (the directx12-agility package finds d3d12.lib through it)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error! vswhere.exe not found. Is Visual Studio installed?
    goto :fail
)
for /f "tokens=*" %%i in ('"%VSWHERE%" -latest -property installationPath') do set "VS_PATH=%%i"
if not exist "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Error! vcvarsall.bat not found.
    goto :fail
)
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Running CMake configure...

cmake --preset vs2026 -S "%ROOT_DIR%" || (
    echo Error! CMake configure failed.
    goto :fail
)

if /i "%1"=="/silent" goto :done
echo.
pause
echo.
cmd /k
:done

endlocal
exit /b 0

:fail
echo.
if /i "%1"=="/silent" exit /b 1
echo Generate failed. See the messages above.
echo.
pause
cmd /k
exit /b 1
