param(
    [string]$Port = "",
    [string]$OutFile = "",
    [switch]$ProbeWslLinux,
    [switch]$IncludeMac
)

$ErrorActionPreference = "SilentlyContinue"

if ([string]::IsNullOrWhiteSpace($OutFile)) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutFile = Join-Path (Join-Path (Get-Location) "board-info") "wt32-sc01-plus-audit-$stamp.txt"
}

$lines = [System.Collections.Generic.List[string]]::new()

function Redact-LocalText {
    param([string]$Text)
    if ($null -eq $Text) { return "" }
    $result = $Text
    if ($env:USERPROFILE) { $result = $result.Replace($env:USERPROFILE, "<USERPROFILE>") }
    if ($env:USERNAME) { $result = $result.Replace($env:USERNAME, "<USER>") }
    return $result
}

function Add-Line {
    param([string]$Text = "")
    $safe = Redact-LocalText $Text
    $lines.Add($safe)
    Write-Host $safe
}

function Add-Section {
    param([string]$Title)
    Add-Line ""
    Add-Line "=== $Title ==="
}

function Capture-External {
    param(
        [string]$Label,
        [string]$Command,
        [string[]]$Arguments = @()
    )

    Add-Line "--- $Label ---"
    $cmd = Get-Command $Command -ErrorAction SilentlyContinue
    if (-not $cmd) {
        Add-Line "NOT FOUND: $Command"
        return
    }

    try {
        $output = & $Command @Arguments 2>&1 | Out-String
        if ([string]::IsNullOrWhiteSpace($output)) {
            Add-Line "(no output)"
        } else {
            foreach ($line in ($output.TrimEnd() -split "`r?`n")) {
                Add-Line $line
            }
        }
    } catch {
        Add-Line "ERROR: $($_.Exception.Message)"
    }
}

Add-Line "WT32-SC01-PLUS Lab - Windows host and passive board audit"
Add-Line "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')"
Add-Line "Repository: https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab"
Add-Line "Mode: read-only host inspection; no flash write/erase commands are used"

Add-Section "WINDOWS"
try {
    $os = Get-ComputerInfo | Select-Object WindowsProductName, WindowsVersion, OsBuildNumber, OsArchitecture
    foreach ($p in $os.PSObject.Properties) { Add-Line ("{0}: {1}" -f $p.Name, $p.Value) }
} catch {
    Add-Line "Get-ComputerInfo unavailable"
}

Add-Section "PACKAGE MANAGER"
Capture-External "winget" "winget" @("--version")

Add-Section "CORE TOOLS"
Capture-External "Git" "git" @("--version")
Capture-External "VS Code" "code" @("--version")
Capture-External "Python launcher" "py" @("--version")
Capture-External "Python" "python" @("--version")
Capture-External "pip" "pip" @("--version")
Capture-External "CMake" "cmake" @("--version")
Capture-External "Ninja" "ninja" @("--version")
Capture-External "Make" "make" @("--version")

Add-Section "ESPRESSIF"
Capture-External "ESP-IDF" "idf.py" @("--version")
Capture-External "EIM CLI" "eim" @("--version")

if (Get-Command python -ErrorAction SilentlyContinue) {
    Add-Line "--- esptool (Python module) ---"
    $esptoolVersion = & python -m esptool version 2>&1 | Out-String
    if ([string]::IsNullOrWhiteSpace($esptoolVersion)) {
        Add-Line "NOT FOUND: python -m esptool"
    } else {
        foreach ($line in ($esptoolVersion.TrimEnd() -split "`r?`n")) { Add-Line $line }
    }
} else {
    Add-Line "--- esptool (Python module) ---"
    Add-Line "NOT FOUND: python"
}

Capture-External "OpenOCD" "openocd" @("--version")

Add-Section "PLATFORMIO / ARDUINO"
Capture-External "PlatformIO Core" "pio" @("--version")
Capture-External "Arduino CLI" "arduino-cli" @("version")

$pioBundled = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
if (Test-Path $pioBundled) {
    Add-Line "PlatformIO bundled CLI: <USERPROFILE>\.platformio\penv\Scripts\pio.exe"
    $pioOutput = & $pioBundled --version 2>&1 | Out-String
    foreach ($line in ($pioOutput.TrimEnd() -split "`r?`n")) { Add-Line $line }
}

Add-Section "VS CODE EXTENSIONS"
if (Get-Command code -ErrorAction SilentlyContinue) {
    $extensions = code --list-extensions --show-versions 2>&1 |
        Select-String -Pattern "espressif|platformio|arduino|remote-wsl|cpptools|cmake"
    if ($extensions) {
        foreach ($item in $extensions) { Add-Line $item.ToString() }
    } else {
        Add-Line "No matching embedded-development extensions found"
    }
} else {
    Add-Line "VS Code CLI not found"
}

Add-Section "WSL"
Capture-External "WSL version" "wsl" @("--version")
Capture-External "WSL status" "wsl" @("--status")
Capture-External "WSL distributions" "wsl" @("--list", "--verbose")

if ($ProbeWslLinux -and (Get-Command wsl -ErrorAction SilentlyContinue)) {
    Add-Line "--- Default WSL Linux tool probe ---"
    $wslProbe = "printf '%s\n' '--- os ---'; grep PRETTY_NAME /etc/os-release 2>/dev/null; uname -a; printf '%s\n' '--- tools ---'; git --version 2>/dev/null; python3 --version 2>/dev/null; cmake --version 2>/dev/null | head -1; ninja --version 2>/dev/null; gcc --version 2>/dev/null | head -1"
    $output = & wsl -- bash -lc $wslProbe 2>&1 | Out-String
    foreach ($line in ($output.TrimEnd() -split "`r?`n")) { Add-Line $line }
} else {
    Add-Line "Linux-in-WSL probe skipped (use -ProbeWslLinux to enable it)"
}

Add-Section "USB / SERIAL CANDIDATES"
$serialPorts = Get-CimInstance Win32_SerialPort |
    Select-Object DeviceID, Name, PNPDeviceID

if ($serialPorts) {
    foreach ($item in $serialPorts) {
        Add-Line ("SERIAL: {0} | {1} | {2}" -f $item.DeviceID, $item.Name, $item.PNPDeviceID)
    }
} else {
    Add-Line "No Win32_SerialPort entries found"
}

$usbCandidates = Get-CimInstance Win32_PnPEntity |
    Where-Object {
        $_.Name -match "ESP|JTAG|Serial|UART|CP210|CH340|CH343|CH910|Silicon Labs|WCH"
    } |
    Select-Object Name, Manufacturer, PNPDeviceID

if ($usbCandidates) {
    foreach ($item in $usbCandidates) {
        Add-Line ("PNP: {0} | {1} | {2}" -f $item.Name, $item.Manufacturer, $item.PNPDeviceID)
    }
} else {
    Add-Line "No ESP/JTAG/USB-UART candidates found by name"
}

Add-Section "OPTIONAL BOARD QUERY"
if ([string]::IsNullOrWhiteSpace($Port)) {
    Add-Line "Board query skipped. Re-run with -Port COMx after identifying the board's serial port."
    Add-Line "Example: .\tools\windows\wt32-sc01-plus-audit.ps1 -Port COM7"
} elseif (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Add-Line "Cannot query $Port because Python was not found"
} else {
    Add-Line "Requested port: $Port"
    Add-Line "The following esptool commands are read-only with respect to flash contents."

    foreach ($commandName in @("chip-id", "flash-id")) {
        Add-Line "--- esptool $commandName ---"
        $output = & python -m esptool --port $Port $commandName 2>&1 | Out-String
        foreach ($line in ($output.TrimEnd() -split "`r?`n")) { Add-Line $line }
    }

    if ($IncludeMac) {
        Add-Line "--- esptool read-mac ---"
        Add-Line "NOTE: the chip MAC is a unique hardware identifier; redact it before public posting if desired."
        $output = & python -m esptool --port $Port read-mac 2>&1 | Out-String
        foreach ($line in ($output.TrimEnd() -split "`r?`n")) { Add-Line $line }
    } else {
        Add-Line "MAC query skipped (use -IncludeMac to enable it)."
    }
}

Add-Section "PUBLIC REPORTING NOTES"
Add-Line "Before posting this report publicly, review it for serial numbers, MAC addresses, local paths, proxy details, or other identifiers you do not want to disclose."
Add-Line "Please also report visible PCB/module markings and whether the USB connection is native ESP USB Serial/JTAG or an external USB-UART bridge."

try {
    $parent = Split-Path -Parent $OutFile
    if ($parent -and -not (Test-Path $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    $lines | Set-Content -Path $OutFile -Encoding utf8
    Write-Host ""
    Write-Host "Saved report: $OutFile"
} catch {
    Write-Host ""
    Write-Host "Could not save report: $($_.Exception.Message)"
    exit 1
}
