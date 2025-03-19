#Requires -Version 5.1
param (
    [Parameter(Mandatory=$true)]
    [string]$Version
)

# Script to create a release branch, update version numbers in various files,
# and push changes to remote repository
# Usage: .\make_release_branch.ps1 -Version "1.0.0"

# Help function
function Show-Help {
    Write-Host "Usage: $($MyInvocation.MyCommand.Name) -Version <Version>"
    Write-Host ""
    Write-Host "Example: $($MyInvocation.MyCommand.Name) -Version 1.0.0"
    exit 1
}

# Validate input
if ([string]::IsNullOrEmpty($Version)) {
    Write-Host "Invalid version"
    Show-Help
}

try {
    # Update CMakeLists.txt
    $cmakeFile = "CMakeLists.txt"
    $cmakeContent = Get-Content $cmakeFile -Raw
    $cmakeContent = $cmakeContent -replace 'project\((.+) VERSION (.+)\)', "project(1 VERSION $Version)"
    Set-Content $cmakeFile -Value $cmakeContent -NoNewline

    # Git operations
    git checkout -b $Version
    if ($LASTEXITCODE -ne 0) { throw "Failed to create branch" }
    
    git add "CMakeLists.txt"
    if ($LASTEXITCODE -ne 0) { throw "Failed to stage files" }
    
    git commit -m "Release $Version"
    if ($LASTEXITCODE -ne 0) { throw "Failed to commit changes" }

    Write-Host "Release branch $Version created successfully"
}
catch {
    Write-Error "Error: $_"
    exit 1
}