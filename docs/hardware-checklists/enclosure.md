# Enclosure fit (front + back half)

Parts: `hardware/enclosure/front_shell.stl`, `back_cover.stl`, `rocker.stl`, `select_cap.stl`. Screws: 4 × M2.5 × 4 mm (OLED); 4 × M2 × 10 mm (back cover through the perfboard into the front bosses).

## Setup
- [ ] Printed in PLA/PETG, 0.2 mm layers, no supports, in the orientation stored in the STLs.
- [ ] Remove any stringing from the slot, the Ø18 hole and the guide rails/tube with a hobby knife.

## Checks
- [ ] OLED hole spacing measured with a caliper: ____ × ____ mm (model: 31.0 × 28.5). Bosses line up with the holes.
- [ ] OLED screws (M2.5 × 4) bite into the pilot holes and do not pierce the plate; OLED does not move when pushed.
- [ ] OLED visible area fully shows through the window; no pixel is cut off at the top or bottom (the OLED and its bosses sit at oled_y = 14.8, 4.5 mm lower than the 19.3 build: if still off, note by how much: ____ mm).
- [ ] OLED header pins fully engage in the socket with the OLED screwed down (OLED underside at 10.6 mm).
- [ ] Perfboard corner holes line up with the bosses; M2 screws bite into the Ø1.6 pilot holes.
- [ ] Perfboard (50 mm) drops into the cavity without force; wires at the top/bottom edges are not squashed.
- [ ] Back cover: the lip enters the front shell with no force and no gap at the sides/bottom; the four M2 × 10 screws pull the cover down and clamp the perfboard without bending it.
- [ ] Battery (36.6 × 46 × 8.25, on edge): slides into the top bay between the rib and the end wall, is held by the side stops, and its leads can reach the board along either side of the rib. Real cell measured: ____ × ____ × ____ mm.
- [ ] TP4056: slides in on edge between the floor stop and the retaining rib; the USB-C receptacle lines up with the opening and a charging cable seats fully (plug overmold fits the recess). Measured: PCB thickness ____ mm, receptacle length ____ mm, USB-C overhang past the PCB edge ____ mm.
- [ ] Charging works with the case closed (check the charge LED with the cover off: it faces into the case).
- [ ] Wi-Fi upload still works with the case closed (the battery sits about 8 mm from the ESP32 antenna: note the RSSI before/after: ____ / ____ dBm).
- [ ] Rocker: pressing the upper end clicks the UP switch and the lower end clicks DOWN; it returns on its own and does not rub the slot.
- [ ] SELECT cap: clicks the switch, returns, and does not catch in the guide tube.
- [ ] Neither button rattles when the shell is shaken (`slide_clr` is now 0.08 mm per side).
- [ ] SELECT reads released at rest (multimeter or firmware). `sel_leg_gap` is only 0.05 mm, below the ±0.3 mm switch-height tolerance: if it reads pressed, raise it.
- [ ] First print without supports: the bed-side chamfers are clean; the rocker flange and SELECT flange undersides and the cover's lip ledge print without sagging or stringing.
- [ ] Rocker and SELECT still slide and return with the new chamfered flanges; SELECT still stops against the plate (its retaining ring is now about 0.5 mm).
- [ ] The 1.35 mm lip enters the front shell without force; the rounded cavity corners (r = 2.0) don't interfere with the PCB, the battery or the TP4056.
- [ ] TP4056 doesn't lean toward the bottom wall (wall-side retainer) and its USB-C stays aligned with the opening. A PCB longer than 28.0 mm won't fit: it sits flush against the +X wall.
- [ ] Filament colours: shell black, rocker white, SELECT red.
- [ ] Front edge of the shell feels smooth; the buttons stand proud and feel right (2.5 mm).

## Notes
- If a button binds, raise `slide_clr` (0.08 mm per side; was 0.15 and wobbled) in `build_enclosure.py` and re-export. If the rocker catches only at the ends when tilted, raise `rocker_end_clr` (0.15 mm).
- If a switch does not click or clicks permanently, adjust `sw12_total` / `sw6_total` to the measured heights and re-export.
