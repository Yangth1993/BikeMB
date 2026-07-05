param(
    [string]$Port = "COM5",
    [int]$Baud = 115200
)

$ErrorActionPreference = "Stop"
$scriptPath = Join-Path $env:TEMP "bikemb_serial_monitor.py"

$pythonCode = @'
import serial
import sys
import time

port = sys.argv[1]
baud = int(sys.argv[2])

ser = serial.Serial()
ser.port = port
ser.baudrate = baud
ser.timeout = 0.2
ser.dtr = False
ser.rts = False

try:
    ser.open()
except Exception as exc:
    print(f"[BikeMB] Failed to open {port}: {exc}")
    print("[BikeMB] Close other serial monitors, then try again.")
    sys.exit(1)

print(f"[BikeMB] Serial monitor opened on {port} @ {baud}.")
print("[BikeMB] Press Ctrl+C to stop.")

try:
    while True:
        data = ser.readline()
        if data:
            timestamp = time.strftime("%H:%M:%S")
            line = data.decode("utf-8", "replace").rstrip("\r\n")
            print(f"[{timestamp}] {line}", flush=True)
except KeyboardInterrupt:
    print("\n[BikeMB] Serial monitor stopped.")
finally:
    ser.close()
'@

Set-Content -Encoding UTF8 -LiteralPath $scriptPath -Value $pythonCode

Write-Host "BikeMB serial monitor"
Write-Host "Port: $Port"
Write-Host "Baud: $Baud"
Write-Host ""
python $scriptPath $Port $Baud

