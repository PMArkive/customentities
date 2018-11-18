@echo off
setlocal

set pydir=C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python36_64

"%pydir%\python.exe" "%~dp0\..\..\vcxproj_gen2\vcxproj_gen2\builder.py" --script="%~dp0\projects\extension.py" --sm-path="%~dp0\..\sourcemod" --mm-path="%~dp0\..\metamod" --hl2sdk-root="%~dp0\.."

if errorlevel 1 (
	pause
	exit
)

endlocal