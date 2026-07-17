param(
    [Parameter(Position=0)]
    [string]$Message = ""
)

$ErrorActionPreference = "Stop"

# 1. Find the current version from git tags
# Get the latest tag that looks like vX.Y
$lastTag = git tag --list "v*" --sort=-v:refname | Select-Object -First 1

if ([string]::IsNullOrWhiteSpace($lastTag)) {
    $major = 0
    $minor = 0
    $lastTag = "v0.0"
} elseif ($lastTag -match "^v(\d+)\.(\d+)") {
    $major = [int]$matches[1]
    $minor = [int]$matches[2]
} else {
    Write-Error "Could not parse version from tag: $lastTag"
    exit 1
}

# 2. Increment logic: 0.1 -> ... -> 0.10 -> 1.0 -> 1.1 ...
if ($minor -ge 10) {
    $major++
    $minor = 0
} else {
    $minor++
}

$newVersion = "v$major.$minor"
$versionString = "$major.$minor.0" # keeping patch version 0 for C++ files

Write-Host "Current version : $lastTag" -ForegroundColor Yellow
Write-Host "Next version    : $newVersion" -ForegroundColor Green
Write-Host ""

# 3. Update C++ header: version.hpp
$versionFile = "include/hypercore/version.hpp"
if (Test-Path $versionFile) {
    $content = Get-Content $versionFile -Raw
    $content = $content -replace "(inline constexpr int VERSION_MAJOR\s*=\s*)\d+;", "`${1}$major;"
    $content = $content -replace "(inline constexpr int VERSION_MINOR\s*=\s*)\d+;", "`${1}$minor;"
    $content = $content -replace "(inline constexpr const char\* VERSION_STRING\s*=\s*)`"[^`"]+`";", "`${1}`"$versionString`";"
    Set-Content -Path $versionFile -Value $content -NoNewline
    Write-Host "Updated $versionFile" -ForegroundColor Cyan
}

# 4. Update CMakeLists.txt
$cmakeFile = "CMakeLists.txt"
if (Test-Path $cmakeFile) {
    $content = Get-Content $cmakeFile -Raw
    $content = $content -replace "(VERSION\s+)\d+\.\d+\.\d+", "`${1}$versionString"
    Set-Content -Path $cmakeFile -Value $content -NoNewline
    Write-Host "Updated $cmakeFile" -ForegroundColor Cyan
}

# 5. Git commit and tag
git add .
$status = git status --porcelain
if ([string]::IsNullOrWhiteSpace($status)) {
    Write-Host "No changes to commit." -ForegroundColor Yellow
    exit 0
}

# Determine commit message
$commitMsg = if ([string]::IsNullOrWhiteSpace($Message)) { 
    "Release $newVersion" 
} else { 
    "$Message ($newVersion)" 
}

Write-Host "Committing: `"$commitMsg`"" -ForegroundColor Cyan
git commit -m "$commitMsg"

Write-Host "Tagging   : $newVersion" -ForegroundColor Cyan
git tag -a $newVersion -m "Release $newVersion"

Write-Host "Pushing   : Commits and tags to origin..." -ForegroundColor Cyan
git push origin HEAD
git push origin $newVersion

Write-Host ""
Write-Host "✅ Successfully released $newVersion!" -ForegroundColor Green
