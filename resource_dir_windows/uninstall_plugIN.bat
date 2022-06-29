@echo off

@whoami /groups | find "S-1-16-12288" > nul && goto :admin
@REM Create vbs to invoke UAC
SET "ELEVATE_CMDLINE=cd /d "%~dp0" & call "%~f0" %*"
ECHO Set objShell = CreateObject("Shell.Application") 1>temp.vbs
ECHO Set objWshShell = WScript.CreateObject("WScript.Shell") 1>>temp.vbs
ECHO Set objWshProcessEnv = objWshShell.Environment("PROCESS") 1>>temp.vbs
ECHO strCommandLine = Trim(objWshProcessEnv("ELEVATE_CMDLINE")) 1>>temp.vbs
ECHO objShell.ShellExecute "cmd", "/c " ^& strCommandLine, "", "runas" 1>>temp.vbs
cscript //nologo temp.vbs & del temp.vbs & exit /b
:admin

SET APP_NAME=plugin-hello
SET CURRENT_DIR=%~dp0

@REM Main 
:main
schtasks /end /tn %APP_NAME%-service
schtasks /delete /tn %APP_NAME%-service /f

goto 2>nul & rmdir "%ALLXON_PLUGIN_DIR%" /s /q