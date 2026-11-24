# LiPo + TP4056 charging (bring-up, ESP32 NOT connected)

Battery: 3.7 V 500 mAh LiPo (EEMB 403048). Module: TP4056 USB-C with dual protection (DW01 + FS8205). Test with the module and battery only; do not connect the ESP32 until the power path is validated (ROADMAP Phase 1).

## Procedure

1. Measure the battery alone at the JST (red +, black -). Healthy range 3.0-4.2 V. Below ~2.5 V do not charge.
2. Check JST polarity against the module before connecting (they often come reversed). Battery to B+ / B-.
3. Plug a 5 V USB-C charger. Do not load OUT+ / OUT-.
4. Red LED = charging, green/blue = done.

## Checklist

- [x] Battery alone measured 3.8 V before charging (2026-11-24).
- [x] Red LED on when USB is plugged in.
- [x] B+/B- read 4.07 V right after connecting under charge (expected: resting voltage plus charge current drop).
- [x] Resting voltage rose 3.8 V -> 3.9 V after 5 min of charge, measured with USB unplugged.
- [ ] IN+/IN- reads ~5 V.
- [ ] Charge current measured in series on B+ (mA range): ____ mA. Factory R_prog `122` (1.2 k) gives ~1 A = 2C for this cell; replace with 4.7-5 k for ~250 mA (I[mA] = 1200 / R[k]).
- [ ] Battery and TP4056 chip temperature checked during charge (battery must stay at most warm).
- [ ] LED turns green/blue; resting voltage after unplugging reads 4.15-4.20 V.
- [ ] OUT+/OUT- reads the same as B+/B- with the battery charged and USB unplugged.

## Next

- Validate the regulator on its own (boost to 5 V into the devkit 5V pin, or 3.3 V LDO into 3V3) before connecting the ESP32. The battery at 4.2 V must never reach the 3V3 pin directly (max 3.6 V).
- Add 100-470 uF + 100 nF on the supply input and a Schottky diode if USB and battery can both feed the board.
- Battery measurement: 100k/100k divider to an ADC1 pin (GPIO1 or GPIO2) with 100 nF.
