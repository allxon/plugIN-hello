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
SET APP_GUID=%ALLXON_PLUGIN_DIR:"%programdata%\allxon\plugIN\"=%
SET CURRENT_DIR=%~dp0

@REM Main 
:main
IF EXIST "%ALLXON_PLUGIN_DIR%" echo plugIN %ALLXON_PLUGIN_DIR% already installed & exit /b 0

xcopy "%CURRENT_DIR%" "%ALLXON_PLUGIN_DIR%" /E /F /Y /I

echo ^<?xml version="1.0" encoding="UTF-16"?^>^
 ^<Task version="1.2" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task"^>^
 ^<RegistrationInfo^>^
 ^<URI^>\%APP_NAME%-service^</URI^>^
 ^</RegistrationInfo^>^
 ^<Triggers^>^
 ^<LogonTrigger^>^
 ^<Enabled^>true^</Enabled^>^
 ^</LogonTrigger^>^
 ^</Triggers^>^
 ^<Principals^>^
 ^<Principal^>^
 ^<UserId^>S-1-5-18^</UserId^>^
 ^<RunLevel^>LeastPrivilege^</RunLevel^>^
 ^</Principal^>^
 ^</Principals^>^
 ^<Settings^>^
 ^<MultipleInstancesPolicy^>IgnoreNew^</MultipleInstancesPolicy^>^
 ^<DisallowStartIfOnBatteries^>false^</DisallowStartIfOnBatteries^>^
 ^<StopIfGoingOnBatteries^>false^</StopIfGoingOnBatteries^>^
 ^<AllowHardTerminate^>true^</AllowHardTerminate^>^
 ^<StartWhenAvailable^>false^</StartWhenAvailable^>^
 ^<RunOnlyIfNetworkAvailable^>false^</RunOnlyIfNetworkAvailable^>^
 ^<IdleSettings^>^
 ^<StopOnIdleEnd^>true^</StopOnIdleEnd^>^
 ^<RestartOnIdle^>false^</RestartOnIdle^>^
 ^</IdleSettings^>^
 ^<AllowStartOnDemand^>true^</AllowStartOnDemand^>^
 ^<Enabled^>true^</Enabled^>^
 ^<Hidden^>false^</Hidden^>^
 ^<RunOnlyIfIdle^>false^</RunOnlyIfIdle^>^
 ^<WakeToRun^>false^</WakeToRun^>^
 ^<ExecutionTimeLimit^>PT0S^</ExecutionTimeLimit^>^
 ^<Priority^>7^</Priority^>^
 ^</Settings^>^
 ^<Actions^>^
 ^<Exec^>^
 ^<Command^>"%ALLXON_PLUGIN_DIR%\%APP_NAME%"^</Command^>^
 ^<Arguments^>"%ALLXON_PLUGIN_DIR%"^</Arguments^>^
 ^</Exec^>^
 ^</Actions^>^
 ^</Task^> > "%ALLXON_PLUGIN_DIR%\service.xml"
schtasks /Create /XML "%ALLXON_PLUGIN_DIR%\service.xml" /TN "%APP_NAME%-service" 
schtasks /run /tn %APP_NAME%-service