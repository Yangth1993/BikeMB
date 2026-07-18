# Change: add cloud AI speaker

## Goal

Define a future experimental AI assistant capability for BikeMB. The device should use Wi-Fi to talk directly to cloud STT, DeepSeek, and TTS services, start recording only while the user holds a dedicated physical button, answer through the onboard speaker, and support configured cloud audio streams.

This change is documentation-only. It does not implement firmware code.

## Scope

- Add a product and technical requirement boundary for a BikeMB cloud AI assistant mode.
- Keep BikeMB's primary product position as a bicycle computer.
- Use the verified ES7210 microphone input and ES8311 speaker output paths as the intended audio foundation.
- Use device-direct cloud access for AI requests and cloud music streams.
- Use DeepSeek as the V1 default LLM behind a replaceable provider interface.
- Keep STT and TTS behind independent replaceable provider interfaces; use Alibaba Cloud Model Studio speech APIs as the initial implementation target.
- Require a dedicated physical AI button and hold-to-talk behavior before recording.
- Provide a light status indicator on the dashboard and a dedicated AI page for detailed status and controls.
- Separate stream playback from future music catalog resolution so point-song support can be added later.
- Define safe fallback behavior when Wi-Fi, AI service, speech conversion, or stream playback fails.

## Non-goals

- No firmware implementation in this change.
- No always-on wake word in V1.
- No touch-triggered recording in V1.
- No voice point-song flow in V1.
- No ride-context question answering or device-control commands in V1.
- No local full music library or SD/TF card music requirement.
- No child companion robot product pivot.
- No requirement to bind BikeMB to a single AI provider.
- No cloud AI dependency for core bicycle computer display functions.

## Risks

- Wi-Fi, cloud AI, speech recognition, TTS, and music streaming can increase latency, power draw, memory pressure, and firmware size.
- API tokens, Wi-Fi passwords, and personal data must not be committed to the repository.
- The current audio input and output paths share I2S and codec resources, so later implementation needs explicit audio session ownership.
- Candidate all-in-one Arduino audio libraries use their own I2S lifecycle and may impose GPL-3.0 obligations, so decoder selection needs a separate implementation spike.
- Network or provider failures must not block speed display, battery display, page switching, or other core ride UI behavior.
