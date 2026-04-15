@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0

if not defined OMNI_USD_FLAVOR set OMNI_USD_FLAVOR=usd
if not defined OMNI_USD_VER set OMNI_USD_VER=25.02
if not defined OMNI_PYTHON_VER set OMNI_PYTHON_VER=3.11

:: Explicitly pass flavor/version/python args so they reach premake even if
:: the repo.toml ${env:...} token resolution produces an empty string.
set FLAVOR_ARGS=--usd-flavor %OMNI_USD_FLAVOR% --usd-ver %OMNI_USD_VER% --python-ver %OMNI_PYTHON_VER%

:: Generate the USD Resolver version header and redist deps
call "%SCRIPT_DIR%tools\generate.bat" %FLAVOR_ARGS% %*
if !errorlevel! neq 0 (goto :end)

:: Build USD Resolver with repo_build
call "%SCRIPT_DIR%repo.bat" build %FLAVOR_ARGS% %*
if !errorlevel! neq 0 (goto :end)

:end
    exit /b !errorlevel!