$appName = "plugin-hello"
$currentDir = (Get-Location).Path

schtasks /end /tn "$appName-service"
schtasks /delete /tn "$appName-service" /f

timeout /t 1

# Move to $currentDir's parent directory and remove the directory
cd ..
Remove-Item -Path "$env:ALLXON_PLUGIN_DIR" -Recurse -Force