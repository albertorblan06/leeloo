# Enclosure fit (front half)

Parts: `hardware/enclosure/front_shell.stl`, `rocker.stl`, `select_cap.stl`. Screws: 4 × M2.5 × 4 mm (OLED); M2 for the perfboard (back half).

## Setup
- [ ] Printed in PLA/PETG, 0.2 mm layers, no supports, in the orientation stored in the STLs.
- [ ] Remove any stringing from the slot, the Ø18 hole and the guide rails/tube with a hobby knife.

## Checks
- [ ] OLED hole spacing measured with a caliper: ____ × ____ mm (model: 31.0 × 28.5). Bosses line up with the holes.
- [ ] OLED screws (M2.5 × 4) bite into the pilot holes and do not pierce the plate; OLED does not move when pushed.
- [ ] OLED visible area fully shows through the window; no pixel is cut off at the top or bottom (the OLED and its bosses sit at oled_y = 14.8, 4.5 mm lower than the 19.3 build: if still off, note by how much: ____ mm).
- [ ] OLED header pins fully engage in the socket with the OLED screwed down (OLED underside at 10.6 mm).
- [ ] Perfboard corner holes line up with the bosses; M2 screws bite into the Ø1.6 pilot holes (back half pending).
- [ ] Perfboard (50 mm) drops into the 51.2 mm cavity without force; wires at the top/bottom edges are not squashed.
- [ ] Rocker: pressing the upper end clicks the UP switch and the lower end clicks DOWN; it returns on its own and does not rub the slot.
- [ ] SELECT cap: clicks the switch, returns, and does not catch in the guide tube.
- [ ] Neither button rattles when the shell is shaken.
- [ ] Filament colours: shell black, rocker white, SELECT red.
- [ ] Front edge of the shell feels smooth; the buttons stand proud and feel right (2.5 mm).

## Notes
- If a button binds, raise `slide_clr` (0.15 mm per side) in `build_enclosure.py` and re-export.
- If a switch does not click or clicks permanently, adjust `sw12_total` / `sw6_total` to the measured heights and re-export.
