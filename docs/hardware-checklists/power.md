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

## Estimated power budget (2026-11-27, NOT measured)

Estimates from typical datasheet figures for the ESP32-S3 module, an SH1106 panel and the development board's own overhead, for the real power path: **1S LiPo 1500 mAh -> TP4056 -> SX1308 boost -> 5V pin of the devkit**. The firmware does not manage power yet (CPU at 240 MHz, no light sleep). Accuracy is about ±30-40 %; the "off" figure is the least certain because it depends on the board revision.

Assumptions: 5 V rail -> AMS1117 (about 5 mA of its own quiescent current) -> 3.3 V; boost efficiency 80-85 % (lower at light load); mean cell voltage 3.7 V; usable capacity 85 % of nominal (about 1275 mAh) because of the protection cut-off and the boost at low input voltage.

| State | Battery current | Time on one charge (that state only) |
|---|---|---|
| Reading | ~80 mA (65-100) | ~16 h |
| Menus (screen on) | ~88 mA | ~14.5 h |
| Recording | ~110 mA | ~11.5 h |
| Wi-Fi session (add a book / send a recording) | ~270 mA (220-340) | ~4.7 h |
| "Off" (deep sleep) | ~20 mA (12-30) | ~2.7 days (1.8-4.4) |

- Normal use (80 % reading, 7 % menus, 8 % recording, 5 % Wi-Fi, always on): about 92 mA average, **about 14 hours of use**.
- Mixed day, the rest of the time "off": 1.5 h reading + 10 min menus + 15 min recording + 10 min Wi-Fi = about 645 mAh per day, **about 2 days**; a heavy reader (3 h reading) about 735 mAh, **about 1.7 days**.
- The "off" state is about 68 % of the daily drain. The chip itself sleeps at microamperes, but the development board does not switch off: the AMS1117 alone draws about 5 mA, plus the power LED, the USB-serial chip and the SX1308's own quiescent current. With a negligible "off" the same two days would last about 6 and 4 days.

How to improve, in order of effect:
1. A slide switch between the TP4056 `OUT+` and the SX1308 `VIN` makes "off" cost nothing.
2. Lower the CPU to 80 MHz and use light sleep between words while reading (probably -25 to -35 % of the reading current; to be measured).
3. Lower the brightness (a few mA of the panel).

Measure it: an ammeter in series with the battery positive, mA range, in each state (reading, menus, recording, Wi-Fi, off), waiting for the value to settle; replace this table with the readings.

- [ ] Reading current measured: ______ mA.
- [ ] Menus: ______ mA. Recording: ______ mA. Wi-Fi session (average / peak): ______ / ______ mA.
- [ ] "Off" (deep sleep) current: ______ mA. If it is above ~5 mA, note the board revision and which of the LED, the USB chip or the regulator is the cause: ______.
