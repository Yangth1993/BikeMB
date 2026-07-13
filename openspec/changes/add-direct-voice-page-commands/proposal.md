# Direct Voice Page Commands

## Summary

Add a disabled-by-default voice command path in the existing BikeMB firmware to validate ESP-SR direct command recognition. The first version runs without a wake word and routes recognized page commands through the existing dashboard next/previous command entry points.

## Scope

- Add a `voice` layer for ESP-SR command recognition.
- Keep voice recognition behind `BIKE_MB_ENABLE_VOICE_COMMANDS`, default off.
- Add a dedicated PlatformIO environment for voice validation.
- Recognize the English commands `Next page` and `Previous page` first, because the current Arduino ESP-SR wrapper loads the English MultiNet model and local sdkconfig disables the Chinese MultiNet model.
- Do not add cloud AI, TTS, or general assistant behavior in this change.

## Risks

- Direct command mode can false trigger because it has no wake word.
- ESP-SR consumes the I2S microphone stream, so this validation mode should not run at the same time as the mic RMS self-test.
- Chinese commands require a later model/configuration change, most likely in ESP-IDF or a custom Arduino ESP-SR configuration.
