# LiPo + TP4056 charging (bring-up, ESP32 NOT connected)

Battery: 3.7 V 500 mAh LiPo (EEMB 403048). Module: TP4056 USB-C with dual protection (DW01 + FS8205). Test with the module and battery only; do not connect the ESP32 until the power path is validated (ROADMAP Phase 1).

## Procedure

1. Measure the battery alone at the JST (red +, black -). Healthy range 3.0-4.2 V. Below ~2.5 V do not charge.
2. Check JST polarity against the module before connecting (they often come reversed). Battery to B+ / B-.
3. Plug a 5 V USB-C charger. Do not load OUT+ / OUT-.
4. Red LED = charging, green/blue = done.

## Cell change (2026-11-26)

The project cell is now one **18650** split from a 7.4 V 1500 mAh 2S RC pack (the 500 mAh LiPo above was only used for this bring-up). Before using it:

- [ ] Cell rating confirmed (1500 mAh pack = two cells of 1500 mAh, or 750 mAh each?): ____ mAh. At 750 mAh the 1 A charge is 1.3 C: raise R_prog to ~2.4 k.
- [ ] Both cells of the pack measured separately, 3.0-4.2 V each: ____ / ____ V. Below ~2.5 V: discard.
- [ ] Strip between the cells cut in the middle without shorting anything; tab left on each cell; the pack's protection board and balance lead removed. The spare cell is stored at ~3.8 V with its ends insulated.
- [ ] Wires soldered to the tab, not to the can (use a short, hot iron).
- [ ] TP4056 R_prog left at 1.2 k (~1 A = 0.67 C for 1500 mAh); charge current measured: ____ mA.

## Checklist

- [x] Battery alone measured 3.8 V before charging (2026-11-24).
- [x] Red LED on when USB is plugged in.
- [x] B+/B- read 4.07 V right after connecting under charge (expected: resting voltage plus charge current drop).
- [x] Resting voltage rose 3.8 V -> 3.9 V after 5 min of charge, measured with USB unplugged.
- [ ] IN+/IN- reads ~5 V.
- [ ] Charge current measured in series on B+ (mA range): ____ mA. Factory R_prog `122` (1.2 k) gives ~1 A = 2C for this cell; replace with 4.7-5 k for ~250 mA (I[mA] = 1200 / R[k]).
- [ ] Battery and TP4056 chip temperature checked during charge (battery must stay at most warm).
- [x] Charge finished: TP4056 LED went off, resting voltage 4.14 V after unplugging (2026-11-24).
- [ ] OUT+/OUT- reads the same as B+/B- with the battery charged and USB unplugged.

## Result: battery direct to the devkit 5V pin does NOT work

- [x] OUT+ to 5V pin, OUT- to GND: red power LED on, OLED stays off.
- [x] 3V3 pin measured 2.47 V (below the ~2.43 V brownout threshold of the ESP32-S3; OLED not powered). The devkit regulator needs ~4.5 V+ in; the battery gives ~4.1 V (and less as it discharges), possibly minus a diode on the 5V pin.
- Not tried: measuring the 5V pin under load (to see whether the devkit has a diode in that path).
- Decision: a boost to 5 V (MT3608 set to exactly 5.0 V BEFORE connecting the ESP32, +100-470 uF at its output) into the 5V pin, or a low-dropout 3.3 V LDO (MCP1700-3.3, HT7333, AP2112K-3.3; not AMS1117, not XC6206) into 3V3 with the ESP32 USB unplugged. Parts not owned yet; keep developing on USB power meanwhile.
- Ordered (2026-11-25): SX1308 boost module (the usual "MT3608" clone, 2-24 V in), to be set to exactly 5.0 V with the multimeter BEFORE connecting the ESP32. MCP1700/HT7333 are only ~250 mA, marginal for Wi-Fi peaks; prefer AP2112K (600 mA) if the LDO route is used instead.
- [ ] SX1308 arrived, output set to 5.0 V with no load.
- [ ] 100-470 uF added at its output; 5V pin fed from it, 3V3 reads 3.3 V, OLED on.
- [ ] Wi-Fi upload mode runs without brownout on battery.
- Do not: feed 3V3 directly from the battery (4.2 V > 3.6 V max), or drop voltage with series diodes (only works with a full battery).

## Next

- Validate the regulator on its own (boost to 5 V into the devkit 5V pin, or 3.3 V LDO into 3V3) before connecting the ESP32. The battery at 4.2 V must never reach the 3V3 pin directly (max 3.6 V).
- Add 100-470 uF + 100 nF on the supply input and a Schottky diode if USB and battery can both feed the board.
- Battery measurement: 100k/100k divider to an ADC1 pin (GPIO1 or GPIO2) with 100 nF.
