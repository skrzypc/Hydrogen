@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "ROOT_DIR=%%~fI"
set "THIRD_PARTY_DIR=%ROOT_DIR%\thirdParty"
set "VCPKG_DIR=%THIRD_PARTY_DIR%\vcpkg"

where git >nul 2>&1 || (
    echo Error! Git is not installed or not in PATH.
    goto :fail
)

if not exist "%VCPKG_DIR%\.git" (
    echo Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg "%VCPKG_DIR%" || goto :fail
)

for /f "tokens=2 delims=:, " %%A in ('findstr "builtin-baseline" "%ROOT_DIR%\thirdParty\vcpkg.json"') do (
    set "VCPKG_BASELINE=%%~A"
)

echo Updating vcpkg...
pushd "%VCPKG_DIR%"
git fetch || goto :fail
git checkout %VCPKG_BASELINE% || goto :fail
popd

set "VCPKG_ROOT=%VCPKG_DIR%"

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics || goto :fail
)

echo Installing dependencies...

pushd "%THIRD_PARTY_DIR%"

if exist "%THIRD_PARTY_DIR%\vcpkg.json" (
    call "%VCPKG_DIR%\vcpkg.exe" install --triplet x64-windows || goto :fail
) else (
    echo Error! Could not find vcpkg.json.
    goto :fail
)

popd

echo.
echo Setup done.

echo.
pause

echo.
cmd /k

endlocal
exit /b 0

:fail
echo.
echo Setup failed. See the messages above.
echo.
pause
cmd /k
exit /b 1
