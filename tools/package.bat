@echo off
setlocal enabledelayedexpansion

call "%~dp0packman\python.bat" %~dp0repoman\package.py %*
if !errorlevel! neq 0 exit /b !errorlevel!
