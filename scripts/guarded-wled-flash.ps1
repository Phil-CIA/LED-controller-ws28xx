param(
  [Parameter(Mandatory = $true)]
  [string]$FirmwareBin,

  [int]$Baud = 921600,

  [string]$FlashAddress = "0x10000",

  [string]$TargetName = "",

  [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$esptoolExe = "C:/Users/user/.platformio/penv/Scripts/esptool.exe"
$identityConfigPath = Join-Path $PSScriptRoot "flash-targets.json"

function Fail-Closed {
  param([string]$Message, [int]$Code = 2)
  Write-Host ""
  Write-Host "FLASH PRECHECK FAILED" -ForegroundColor Red
  Write-Host $Message -ForegroundColor Yellow
  Write-Host "No flash was performed." -ForegroundColor Yellow
  exit $Code
}

function Invoke-Esptool {
  param(
    [string[]]$ArgumentList
  )

  $stdoutFile = [System.IO.Path]::GetTempFileName()
  $stderrFile = [System.IO.Path]::GetTempFileName()
  try {
    $proc = Start-Process -FilePath $esptoolExe `
              -ArgumentList $ArgumentList `
              -NoNewWindow -PassThru -Wait `
              -RedirectStandardOutput $stdoutFile `
              -RedirectStandardError $stderrFile
    $stdout = if (Test-Path $stdoutFile) { Get-Content $stdoutFile -Raw } else { "" }
    $stderr = if (Test-Path $stderrFile) { Get-Content $stderrFile -Raw } else { "" }
    return [pscustomobject]@{
      ExitCode = $proc.ExitCode
      Text     = ($stdout + "`n" + $stderr)
    }
  }
  finally {
    Remove-Item $stdoutFile -ErrorAction SilentlyContinue
    Remove-Item $stderrFile -ErrorAction SilentlyContinue
  }
}

function Get-TargetConfig {
  param([string]$RequestedTarget)

  if (!(Test-Path $identityConfigPath)) {
    Fail-Closed "Flash target config not found at $identityConfigPath"
  }

  $json = Get-Content $identityConfigPath -Raw | ConvertFrom-Json
  $activeTarget = ([string]$json.activeTarget).Trim()
  $selectedTarget = if ([string]::IsNullOrWhiteSpace($RequestedTarget)) { $activeTarget } else { $RequestedTarget.Trim() }

  if ([string]::IsNullOrWhiteSpace($selectedTarget)) {
    Fail-Closed "No flash target selected. Set activeTarget or pass -TargetName."
  }

  $targetsNode = $json.PSObject.Properties["targets"]
  if ($null -eq $targetsNode -or $null -eq $targetsNode.Value) {
    Fail-Closed "flash-targets.json missing 'targets' object."
  }

  $targetProp = $targetsNode.Value.PSObject.Properties[$selectedTarget]
  if ($null -eq $targetProp) {
    $available = @($targetsNode.Value.PSObject.Properties.Name) -join ", "
    Fail-Closed "Unknown target '$selectedTarget'. Available targets: $available"
  }

  $target = $targetProp.Value
  $port = ([string]$target.port).Trim().ToUpperInvariant()
  $chip = ([string]$target.chip).Trim()
  $expectedMac = ([string]$target.expectedMac).Trim().ToUpperInvariant()
  $description = ([string]$target.description).Trim()

  if ([string]::IsNullOrWhiteSpace($port)) {
    Fail-Closed "Target '$selectedTarget' has no 'port' configured."
  }
  if ([string]::IsNullOrWhiteSpace($chip)) {
    Fail-Closed "Target '$selectedTarget' has no 'chip' configured."
  }

  return [pscustomobject]@{
    Name        = $selectedTarget
    Description = $description
    Port        = $port
    Chip        = $chip
    ExpectedMac = $expectedMac
  }
}

function Get-DetectedPorts {
  $ports = New-Object System.Collections.ArrayList
  $pnpPorts = @(Get-PnpDevice -Class Ports -ErrorAction SilentlyContinue)

  foreach ($p in $pnpPorts) {
    if ($null -eq $p.FriendlyName) { continue }
    $m = [regex]::Match([string]$p.FriendlyName, 'COM\d+')
    if ($m.Success) {
      [void]$ports.Add([string]$m.Value.ToUpperInvariant())
    }
  }

  return @($ports | Sort-Object -Unique)
}

function Probe-Chip {
  param(
    [string]$Port,
    [string]$ExpectedChip
  )

  $probe = Invoke-Esptool -ArgumentList @("--port", $Port, "chip-id")
  if ($probe.ExitCode -ne 0) {
    Fail-Closed "Chip probe failed on $Port. Output:`n$($probe.Text)"
  }

  if ($probe.Text -notmatch [regex]::Escape($ExpectedChip)) {
    Fail-Closed "Port $Port detected chip does not match expected '$ExpectedChip'. Output:`n$($probe.Text)"
  }
}

function Read-Mac {
  param([string]$Port)

  $probe = Invoke-Esptool -ArgumentList @("--port", $Port, "read-mac")
  if ($probe.ExitCode -ne 0) {
    return ""
  }

  $macMatch = [regex]::Match($probe.Text, 'MAC:\s+([0-9A-Fa-f:]{17,23})')
  if (!$macMatch.Success) {
    return ""
  }

  return $macMatch.Groups[1].Value.Trim().ToUpperInvariant()
}

if (!(Test-Path $esptoolExe)) {
  Fail-Closed "esptool executable not found at $esptoolExe"
}

$firmwarePath = Resolve-Path -Path $FirmwareBin -ErrorAction SilentlyContinue
if ($null -eq $firmwarePath) {
  Fail-Closed "Firmware binary not found: $FirmwareBin"
}

if ($Baud -lt 115200) {
  Fail-Closed "Baud must be >= 115200. Received: $Baud"
}

if ($FlashAddress -notmatch '^0x[0-9A-Fa-f]+$') {
  Fail-Closed "FlashAddress must be a hex address like 0x10000. Received: $FlashAddress"
}

$target = Get-TargetConfig -RequestedTarget $TargetName
$expectedPort = $target.Port
$expectedChip = $target.Chip
Write-Host "Selected target: $($target.Name) [$($target.Description)] on $expectedPort" -ForegroundColor Cyan

if ($Force) {
  Write-Host "WARNING: Force mode enabled. Prechecks are bypassed." -ForegroundColor Yellow
}

if (-not $Force) {
  $ports = @(Get-DetectedPorts)
  if ($ports -notcontains $expectedPort) {
    $seen = if (@($ports).Count -gt 0) { $ports -join ", " } else { "none" }
    Fail-Closed "Expected target on $expectedPort, but detected ports: $seen"
  }

  Probe-Chip -Port $expectedPort -ExpectedChip $expectedChip
  Write-Host "Precheck: chip probe matched $expectedChip on $expectedPort" -ForegroundColor Green

  $detectedMac = Read-Mac -Port $expectedPort
  if ([string]::IsNullOrWhiteSpace($detectedMac)) {
    Fail-Closed "Could not read immutable MAC from $expectedPort."
  }
  Write-Host "Precheck: detected MAC $detectedMac on $expectedPort" -ForegroundColor Green

  $expectedMac = $target.ExpectedMac
  if ([string]::IsNullOrWhiteSpace($expectedMac)) {
    Fail-Closed "Target '$($target.Name)' is missing expectedMac in flash-targets.json. Refusing to flash without MAC pinning."
  }

  if (![string]::IsNullOrWhiteSpace($expectedMac) -and $detectedMac -ne $expectedMac) {
    Fail-Closed "MAC mismatch on $expectedPort. Expected $expectedMac but detected $detectedMac."
  }
}

$flashArgs = @(
  "--chip", "esp32",
  "--port", $expectedPort,
  "--baud", [string]$Baud,
  "--before", "default-reset",
  "--after", "hard-reset",
  "write-flash",
  "--no-progress",
  "-z",
  $FlashAddress,
  ('"{0}"' -f [string]$firmwarePath)
)

Write-Host "Running: esptool $($flashArgs -join ' ')" -ForegroundColor Cyan
$result = Invoke-Esptool -ArgumentList $flashArgs
Write-Host $result.Text
if ($result.ExitCode -ne 0) {
  Fail-Closed "WLED flash failed. See esptool output above."
}

Write-Host "WLED flash completed successfully on $expectedPort." -ForegroundColor Green
