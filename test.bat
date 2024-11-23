@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0

:: Run tests using repo_test
call "%SCRIPT_DIR%repo.bat" test %*
if !errorlevel! neq 0 exit /b !errorlevel!
