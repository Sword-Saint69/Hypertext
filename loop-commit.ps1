$ErrorActionPreference = "Continue"

Write-Host "Starting background auto-committer (every 60 seconds)..." -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop." -ForegroundColor Yellow
Write-Host "========================================================="

while ($true) {
    # Check if there are changes before running so we don't spam the console if it's idle
    $status = git status --porcelain
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        Write-Host ""
        Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Changes detected! Committing..." -ForegroundColor Cyan
        .\autocommit.ps1 "Auto-save"
    }
    
    Start-Sleep -Seconds 60
}
