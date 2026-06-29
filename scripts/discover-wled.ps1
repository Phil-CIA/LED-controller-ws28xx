param(
  [string]$WledHost = "wled.local",
  [string]$TargetName = "",
  [switch]$OpenUi
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$identityConfigPath = Join-Path $PSScriptRoot "flash-targets.json"

function ConvertTo-NormalizedMac {
  param([string]$Mac)
  return ($Mac -replace '[^0-9A-Fa-f]', '').ToUpperInvariant()
}

function Get-TargetConfig {
  param([string]$RequestedTarget)

  if (!(Test-Path $identityConfigPath)) {
    throw "Flash target config not found at $identityConfigPath"
  }

  $json = Get-Content $identityConfigPath -Raw | ConvertFrom-Json
  $activeTarget = ([string]$json.activeTarget).Trim()
  $selectedTarget = if ([string]::IsNullOrWhiteSpace($RequestedTarget)) { $activeTarget } else { $RequestedTarget.Trim() }
  if ([string]::IsNullOrWhiteSpace($selectedTarget)) {
    throw "No target selected. Set activeTarget or pass -TargetName."
  }

  $targetsNode = $json.PSObject.Properties["targets"]
  if ($null -eq $targetsNode -or $null -eq $targetsNode.Value) {
    throw "flash-targets.json missing 'targets' object."
  }

  $targetProp = $targetsNode.Value.PSObject.Properties[$selectedTarget]
  if ($null -eq $targetProp) {
    $available = @($targetsNode.Value.PSObject.Properties.Name) -join ", "
    throw "Unknown target '$selectedTarget'. Available targets: $available"
  }

  $target = $targetProp.Value
  return [pscustomobject]@{
    Name        = $selectedTarget
    Description = ([string]$target.description).Trim()
    ExpectedMac = ([string]$target.expectedMac).Trim().ToUpperInvariant()
  }
}

function Test-WledJson {
  param([string]$CandidateHost)
  try {
    $uri = "http://$CandidateHost/json/info"
    $resp = Invoke-RestMethod -Uri $uri -Method Get -TimeoutSec 3
    if ($null -ne $resp.ver -or $null -ne $resp.name) {
      return $resp
    }
  }
  catch {
    return $null
  }
  return $null
}

function Find-IpByMac {
  param([string]$ExpectedMac)

  if ([string]::IsNullOrWhiteSpace($ExpectedMac)) {
    return ""
  }

  $target = ConvertTo-NormalizedMac -Mac $ExpectedMac
  $arpOutput = arp -a
  foreach ($line in $arpOutput) {
    $match = [regex]::Match([string]$line, '^\s*(\d+\.\d+\.\d+\.\d+)\s+([0-9a-fA-F\-]{17})\s+\w+')
    if (!$match.Success) { continue }

    $ip = $match.Groups[1].Value
    $mac = $match.Groups[2].Value
    if ((ConvertTo-NormalizedMac -Mac $mac) -eq $target) {
      return $ip
    }
  }

  return ""
}

$found = $null
$resolvedHost = ""
$target = Get-TargetConfig -RequestedTarget $TargetName
Write-Host "Discovery target: $($target.Name) [$($target.Description)]" -ForegroundColor Cyan

$found = Test-WledJson -CandidateHost $WledHost
if ($null -ne $found) {
  $resolvedHost = $WledHost
}
else {
  $expectedMac = $target.ExpectedMac
  $ipByMac = Find-IpByMac -ExpectedMac $expectedMac
  if (![string]::IsNullOrWhiteSpace($ipByMac)) {
    $found = Test-WledJson -CandidateHost $ipByMac
    if ($null -ne $found) {
      $resolvedHost = $ipByMac
    }
  }
}

if ($null -eq $found) {
  Write-Host "WLED not reachable yet." -ForegroundColor Yellow
  Write-Host "Next steps:" -ForegroundColor Yellow
  Write-Host "1) Join WLED AP on a phone/laptop (WLED-xxxxxx) and set home Wi-Fi credentials." -ForegroundColor Yellow
  Write-Host "2) Wait 20-30s for join/reboot." -ForegroundColor Yellow
  Write-Host "3) Re-run this script or task." -ForegroundColor Yellow
  exit 3
}

$uiUrl = "http://$resolvedHost"
Write-Host "WLED reachable at $uiUrl" -ForegroundColor Green
if ($null -ne $found.name) {
  Write-Host "Name: $($found.name)" -ForegroundColor Green
}
if ($null -ne $found.ver) {
  Write-Host "Version: $($found.ver)" -ForegroundColor Green
}

Write-Host "Set/verify in WLED UI: GPIO18, LED count 300, color order GRB, max current 12000 mA." -ForegroundColor Cyan

if ($OpenUi) {
  Start-Process $uiUrl | Out-Null
}
