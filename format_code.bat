@echo off
setlocal enabledelayedexpansion

:: Run formatting ignoring the last time it was run
call "%~dp0repo.bat" format --force
