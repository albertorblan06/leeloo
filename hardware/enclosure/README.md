# Leeloo front enclosure (print-ready, front half only)

Files:

| File | What | Print orientation (already applied in the STL) |
|---|---|---|
| `front_shell.stl` | Front shell: plate with OLED window, rocker slot and Ø18 hole; OLED bosses, button guides, perfboard bosses | Plate on the bed, open side up. No supports. **Black.** |
| `rocker.stl` | UP/DOWN menu scroll rocker | Pad on the bed, legs up. No supports. **White.** |
| `select_cap.stl` | SELECT / accept cap (Ø18) | Pad on the bed, legs up. No supports. **Red.** |
| `build_enclosure.py` | Fusion script that generates everything above (`D` holds all dimensions) | |

Slicer: PLA or PETG, 0.4 mm nozzle, 0.2 mm layers, 3 walls, 20–30 % infill, no supports, no brim needed.
Print the two caps at 100 % scale; if they bind, raise `slide_clr` in `D` (0.15 mm per side now), re-run the script and re-export.

## Hardware needed
- OLED: **4 × M2.5 × 4 mm** pan-head screws, from below through the OLED PCB into the bosses (pilot hole Ø2.0, self-tapping). A longer screw (5–6 mm) would pierce the front plate.
- Perfboard (corner holes are **M2**): the four corner bosses have Ø1.6 pilot holes for M2 self-tapping screws (used by the back half, not designed yet).

## How the parts work
- **OLED:** clamped between the four bosses and the screw heads, so it has no play. Its PCB underside sits **10.6 mm** above the perfboard (was 12.0: at 12.0 the header pins only half engaged).
- **Rocker:** its flange sits on a fulcrum rib and is guided by two rails; the legs rest on the two 12×12 switch actuators. Pressing one end rolls it on the rib edge (checked free for ±3°, the switches need about ±1.9°).
- **SELECT cap:** flange guided by a tube; leg 0.2 mm above the 6×6 actuator; the switch spring returns it against the plate.
- **Look and feel:** the buttons stand 2.5 mm proud of the plate, with a 1.0 mm (rocker) / 1.2 mm (SELECT) fillet on the top edge. The front-face perimeter of the shell has a 0.8 mm fillet; the open rim stays square for the back half.
- FDM can't give literally zero clearance: the buttons slide with 0.15 mm per side and long guides, so they don't wobble.

## Assumptions to check before printing (not caliper-measured)
- OLED mounting holes: 31.0 × 28.5 mm centres, Ø3.0 (`oled_hole_dx`, `oled_hole_dy`). The photo hints the Y spacing may be up to 29.1 mm; measure it.
- Perfboard mounting holes: M2 (modelled Ø2.2), 2.0 mm from each edge.
- Switch heights 7.2 / 5.2 mm above the perfboard (from the user) and actuator Ø6 / Ø3.5 are modelled; ±0.3 mm of real tolerance shows up as a small gap or a light preload.
- 6×6 switch Y position (−14.73 mm from the board centre) comes from the original sketch.
- Glass 35.0 × 23.6 mm is from the photos. The OLED height (10.6 mm) is the original 12.0 mm minus the 1.4 mm the user found extra.
- Female header height 8.1 mm is derived (10.6 − 2.5 mm male body), not measured.
- **OLED position is a bet:** the user asked for the screen 1.5–3 mm higher than the first print; the photos could not confirm it (parallax), so the **whole OLED moved 2.0 mm up** (`oled_y` 17.3 → 19.3): its mounting holes/bosses (y 5.05 and 33.55, x ±15.5) and the window (plain rectangle, lit area 34.5 × 18.2, y 12.5–30.7) move together. The module's top edge now overhangs the perfboard's top edge by about 1 mm, and the female header in the model (y 32.7) was not moved, so measure where the header really is. If it is still off, adjust `oled_y`.
- The front plate leaves 0.4 mm over the OLED glass.

- Cavity is 51.2 mm wide (0.6 mm per side, `side_clr`) after the 50 mm perfboard fitted too tightly at 0.3 mm. Shell size 55.2 × 83.2 × 16.8 mm.

Not designed yet: the back half (DevKitC, USB-C openings, antenna, battery), so this shell can't hold the board by itself.

Fit checklist: `docs/hardware-checklists/enclosure.md`.
