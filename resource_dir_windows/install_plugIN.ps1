throw "Error trying to do a task"

$appName = "plugin-hello"
$appGuid = ($env:ALLXON_PLUGIN_DIR).replace('C:\ProgramData\allxon\plugIN\', '')
$currentDir = (Get-Location).Path

# Main Logic
if (Test-Path $env:ALLXON_PLUGIN_DIR) {
  Write-Host "plugIN $env:ALLXON_PLUGIN_DIR already installed."
  exit 0
}

Copy-Item -Path "$currentDir\$appGuid" -Destination $env:ALLXON_PLUGIN_DIR -Recurse -Force

# Generating XML content for the scheduled task
$xmlContent = @"
<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.2" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task">
<RegistrationInfo>
  <URI>\$appName-service</URI>
</RegistrationInfo>
<Triggers>
  <LogonTrigger>
    <Enabled>true</Enabled>
  </LogonTrigger>
</Triggers>
<Principals>
  <Principal>
    <UserId>S-1-5-18</UserId>
    <RunLevel>LeastPrivilege</RunLevel>
  </Principal>
</Principals>
<Settings>
  <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>
  <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>
  <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>
  <AllowHardTerminate>true</AllowHardTerminate>
  <StartWhenAvailable>false</StartWhenAvailable>
  <RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>
  <IdleSettings>
    <StopOnIdleEnd>true</StopOnIdleEnd>
    <RestartOnIdle>false</RestartOnIdle>
  </IdleSettings>
  <AllowStartOnDemand>true</AllowStartOnDemand>
  <Enabled>true</Enabled>
  <Hidden>false</Hidden>
  <RunOnlyIfIdle>false</RunOnlyIfIdle>
  <WakeToRun>false</WakeToRun>
  <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
  <Priority>7</Priority>
</Settings>
<Actions>
  <Exec>
    <Command>"$env:ALLXON_PLUGIN_DIR\$appName"</Command>
    <Arguments>"$env:ALLXON_PLUGIN_DIR"</Arguments>
  </Exec>
</Actions>
</Task>
"@

# Save XML to file
$xmlFilePath = Join-Path $env:ALLXON_PLUGIN_DIR "service.xml"
$xmlContent | Set-Content -Path $xmlFilePath

# Create and run the scheduled task
schtasks /Create /XML "$xmlFilePath" /TN "$appName-service"
schtasks /Run /TN "$appName-service"