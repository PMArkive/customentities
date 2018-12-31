@echo off
setlocal

set buildscripts=%~dp0\..\..\..\buildscripts

call %buildscripts%\gitclone.bat https://github.com/Microsoft/Detours.git master Detours
call %buildscripts%\gitclone.bat https://github.com/asmjit/asmjit.git master asmjit

endlocal
exit