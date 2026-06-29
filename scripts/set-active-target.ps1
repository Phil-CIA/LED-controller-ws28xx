param(
  [Parameter(Mandatory = $true)]
  [string]$TargetName
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$identityConfigPath = Join-Path $PSScriptRoot "flash-targets.json"

if (!(Test-Path $identityConfigPath)) {
  throw "Config file not found: $identityConfigPath"
}

$json = Get-Content $identityConfigPath -Raw | ConvertFrom-Json
$targetsNode = $json.PSObject.Properties["targets"]
if ($null -eq $targetsNode -or $null -eq $targetsNode.Value) {
  throw "flash-targets.json is missing the 'targets' object."
}

$targetProp = $targetsNode.Value.PSObject.Properties[$TargetName]
if ($null -eq $targetProp) {
  $available = @($targetsNode.Value.PSObject.Properties.Name) -join ", "
  throw "Unknown target '$TargetName'. Available targets: $available"
}

$json.activeTarget = $TargetName
$json | ConvertTo-Json -Depth 8 | Set-Content -Path $identityConfigPath -Encoding UTF8
Write-Host "Active target set to '$TargetName'." -ForegroundColor Green
