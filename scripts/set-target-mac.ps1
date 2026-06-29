param(
  [Parameter(Mandatory = $true)]
  [string]$TargetName,

  [Parameter(Mandatory = $true)]
  [string]$ExpectedMac
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$identityConfigPath = Join-Path $PSScriptRoot "flash-targets.json"

function ConvertTo-CanonicalMac {
  param([string]$Mac)

  $normalized = ($Mac -replace '[^0-9A-Fa-f]', '').ToUpperInvariant()
  if ($normalized.Length -ne 12) {
    throw "ExpectedMac must contain 12 hex characters."
  }

  return ($normalized -replace '(.{2})(?=.)', '$1:')
}

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

$canonicalMac = ConvertTo-CanonicalMac -Mac $ExpectedMac
$targetProp.Value.expectedMac = $canonicalMac

$json | ConvertTo-Json -Depth 8 | Set-Content -Path $identityConfigPath -Encoding UTF8
Write-Host "Set expectedMac for '$TargetName' to $canonicalMac" -ForegroundColor Green
