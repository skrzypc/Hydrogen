@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "ROOT_DIR=%%~fI"
set "THIRD_PARTY_DIR=%ROOT_DIR%\thirdParty"
set "VCPKG_DIR=%THIRD_PARTY_DIR%\vcpkg"

where git >nul 2>&1 || (
    echo Error! Git is not installed or not in PATH.
    exit /b 1
)

if not exist "%ROOT_DIR%\data\models" mkdir "%ROOT_DIR%\data\models"

if not exist "%VCPKG_DIR%\.git" (
    echo Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg "%VCPKG_DIR%" || exit /b 1

    pushd "%VCPKG_DIR%"
    git checkout 4334d8b4c8916018600212ab4dd4bbdc343065d1 || exit /b 1
    popd
)

set "VCPKG_ROOT=%VCPKG_DIR%"

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics || exit /b 1
)

echo Installing dependencies...

pushd "%THIRD_PARTY_DIR%"

if exist "%THIRD_PARTY_DIR%\vcpkg.json" (
    call "%VCPKG_DIR%\vcpkg.exe" install --triplet x64-windows || exit /b 1
) else (
    echo Error! Could not find vcpkg.json.
    exit /b 1
)

popd

echo Setup done.
endlocal