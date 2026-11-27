# Switching off and on (deep sleep)

Status: **written, not yet run on hardware.** Spec: `docs/DEVICE-MENU.md` "Apagar". Code: `src/power.cpp`,
`Ui` screens `PowerConfirm` and `PoweringOff`. SELECT is GPIO6 (RTC-capable), wired to GND with the internal
pull-up only (`buttons-input.md`).

## Switching off
- [ ] On the root menu, hold SELECT (~0.8 s): `APAGAR` appears with `¿Apagar Leeloo?`, `No` focused.
- [ ] `No`, or another long press, returns to the root menu. Nothing else changes.
- [ ] From a sub-menu a long press still goes back one level only; the question appears only on the root.
- [ ] `Sí, apagar` shows `Apagando...` for about 0.7 s and then the panel goes completely dark (no faint glow).
- [ ] Serial shows no crash or watchdog reset while switching off. Record any message: ______.
- [ ] With Wi-Fi `Encendido` (access point up), switch off: the `Leeloo` network disappears before the panel goes dark.
- [ ] Switch off right after a book upload and right after deleting a book: after waking, the library is intact.
- [ ] Open a book, read a few words, leave with a long press, switch off, wake: the book resumes where it was left.

## Switching on
- [ ] Hold SELECT: the splash appears after about one second and the menu follows. Measure from press to splash: ______ s.
- [ ] A short tap (~0.2 s) does **not** switch it on: the device goes back to sleep and the panel stays dark.
- [ ] Press SELECT again while `Apagando...` is on and keep it down for 3 s: the device wakes at once and boots again (bounded, no loop). Release and switch off normally: it sleeps.
- [ ] Hold SELECT through the whole wake: it does not wake and fall asleep again in a loop.
- [ ] Press UP or DOWN while it sleeps: nothing happens (only SELECT is a wake source).
- [ ] Settings (brightness, Wi-Fi mode, reading step) are the same after waking.
- [ ] Plugging in USB, pressing the board's RESET and flashing still boot normally (the hold check applies only after a SELECT wake).

## Power
- [ ] Measure the supply current in deep sleep with the battery path of `battery-tp4056.md` (USB unplugged): ______ mA. The development board's regulator and USB chip keep drawing some; this is not a power switch. If it flattens the battery in days, a hardware switch or a different regulator is needed.
- [ ] After a night switched off with a charged battery, the voltage drop is: ______ V.
- [ ] If the RTC pull-up is not enough (the device wakes by itself or never wakes), note the symptom here: ______. The alternative is an external 100 kΩ pull-up on GPIO6.
