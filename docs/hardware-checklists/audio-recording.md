# Audio recording hardware checks

The firmware uses the existing ESP32-S3, OLED, and three-button wiring. Connect
the I2S microphone as follows:

| Microphone pin | ESP32-S3 |
|---|---|
| VDD | 3V3 |
| GND | GND |
| SCK/BCLK | GPIO15 |
| WS/LRCLK | GPIO16 |
| SD/DOUT | GPIO17 |
| L/R | GND |

## Firmware acceptance

- Confirm the microphone module supports standard I2S and its supply is 3.3 V.
- Build and flash the production firmware only after the user approves the
  device update; do not use the standalone mic-test image for these checks.
- From **Grabar**, capture a short spoken sample, pause, wait, resume, and stop.
  Verify the paused interval is absent from the sample duration.
- Record with enough free space for the 8 kHz, two-minute mode and verify the
  WAV header, mono channel count, 16-bit samples, and 8 kHz rate on a computer.
- With free space between the two supported thresholds, verify the OLED reports
  the 16 kHz, 30-second fallback. With less than the short-mode threshold,
  verify the device refuses recording and asks the user to free space without
  removing books or an existing WAV.
- Reboot with a pending WAV and verify it remains listed. Verify **Borrar audio**
  requires confirmation and leaves the file when cancelled.
- Start **Subir audio**, connect a phone to the Leeloo AP, enter the displayed
  PIN, receive the WAV, play/download it, and verify the device copy is removed
  only after complete browser receipt and acknowledgement.
- Interrupt the browser transfer before acknowledgement and verify the WAV
  remains available for another attempt.
