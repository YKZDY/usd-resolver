setlocal ENABLEDELAYEDEXPANSION
set OUTDIR=_TraceOutput
set inputs=

for %%i in (.\%OUTDIR%\*.otb) do (
	echo %%i
	set inputs=!inputs! -b "%%i"
)
echo %inputs%
..\_build\host-deps\omni-trace-tools\tools\OtChromeTraceExporter.exe %inputs% -o ./%OUTDIR%/combinedTrace.ct.json -flag OMNIVERSE_EXTENSION2 -flag OMNIOBJECT_EXTENSION -flag USD_NOTICE_EXTENSION
pause