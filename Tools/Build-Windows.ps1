[CmdletBinding()]
param(
    [string]$EngineRoot,
    [ValidateSet('Verify','Editor','Import','Shipping','All')][string]$Stage = 'Verify',
    [switch]$AllowEngineMismatch
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectRoot 'Wildlight.uproject'
$Lock = Get-Content (Join-Path $ProjectRoot 'engine-lock.json') -Raw | ConvertFrom-Json
if ([System.Environment]::OSVersion.Platform -ne [System.PlatformID]::Win32NT) { throw 'Run this script on Windows.' }
if (-not $EngineRoot) {
    $Candidate = Join-Path ${env:ProgramFiles} ('Epic Games\UE_' + $Lock.association)
    if (Test-Path $Candidate) { $EngineRoot = $Candidate }
}
if (-not $EngineRoot) { throw 'Supply -EngineRoot with the installed Unreal folder, for example C:\Program Files\Epic Games\UE_5.8.' }
$EngineRoot = (Resolve-Path $EngineRoot).Path
$VersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path $VersionFile)) { throw "Invalid Unreal folder: missing $VersionFile" }
$Version = Get-Content $VersionFile -Raw | ConvertFrom-Json
$ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
if ($ActualVersion -ne $Lock.version -and -not $AllowEngineMismatch) {
    throw "This project targets Unreal $($Lock.version); installed version is $ActualVersion. Use the matching engine or explicitly pass -AllowEngineMismatch to test a migration."
}
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UAT = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
foreach ($Binary in @($Build,$Editor,$UAT,$Project)) {
    if (-not (Test-Path $Binary)) { throw "Required file missing: $Binary" }
}
$VSWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $VSWhere)) { throw 'Install Visual Studio 2022 with Game development with C++, the MSVC toolset and Windows SDK.' }
$VS = & $VSWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $VS) { throw 'A Visual Studio installation with the C++ compiler was not found.' }
Write-Host "Unreal $ActualVersion | $EngineRoot"
Write-Host "C++ toolchain | $VS"
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed with exit code $LASTEXITCODE : $Program" }
}
if ($Stage -eq 'Verify') { Write-Host 'Paths and installed toolchain verified. No compile or gameplay verification has run.'; exit 0 }
if ($Stage -in @('Editor','Import','All')) {
    Invoke-Checked $Build @('WildlightEditor','Win64','Development',$Project,'-WaitMutex','-NoHotReload')
}
if ($Stage -in @('Import','All')) {
    Invoke-Checked $Editor @($Project,'-run=pythonscript',('-script=' + (Join-Path $PSScriptRoot 'Import-Assets.py')),'-unattended','-nop4','-nosplash','-NullRHI')
}
if ($Stage -in @('Shipping','All')) {
    $Output = Join-Path $ProjectRoot 'Builds\Windows'
    Invoke-Checked $UAT @('BuildCookRun',('-project=' + $Project),'-nop4','-platform=Win64','-clientconfig=Shipping','-build','-cook','-stage','-pak','-iostore','-archive',('-archivedirectory=' + $Output),'-utf8output')
    Write-Host "Shipping output: $Output. Test the actual packaged executable on a clean Windows machine before release."
}
