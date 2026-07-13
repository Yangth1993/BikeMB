from pathlib import Path

Import("env")

framework_libs = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32-libs"))
srmodels = framework_libs / "esp32s3" / "esp_sr" / "srmodels.bin"

if not srmodels.exists():
    raise FileNotFoundError(f"ESP-SR model image not found: {srmodels}")

env.Append(FLASH_EXTRA_IMAGES=[("0xC10000", str(srmodels))])
print(f"[BikeMB] ESP-SR model image will be uploaded: {srmodels}")
