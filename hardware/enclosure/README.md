# Leeloo enclosure (front shell + back cover)

Size: **62.6 × 94.4 mm**, 40.4 mm thick closed (front 16.8 + back 23.6; the back cover's lip adds 2 mm inside the front shell). The width keeps the old 55.2 : 83.2 ratio; the length grows at the top for the battery bay (+9.85 mm) and at the bottom for the TP4056 bay (+1.35 mm). Both are computed in `layout()` from `D`.

Files:

| File | What | Print orientation (already applied in the STL) |
|---|---|---|
| `front_shell.stl` | Front shell: plate with OLED window, rocker slot and Ø18 hole; OLED bosses, button guides, perfboard bosses, battery-bay rib | Plate on the bed, open side up. No supports. **Black.** |
| `back_cover.stl` | Back cover: floor, alignment lip, 4 screw posts, battery bay (rib + side stops), TP4056 bay (stop + retainer) and USB-C opening with an outer recess | Floor on the bed, lip up. No supports. **Black.** |
| `rocker.stl` | UP/DOWN menu scroll rocker | Pad on the bed, legs up. No supports. **White.** |
| `select_cap.stl` | SELECT / accept cap (Ø18) | Pad on the bed, legs up. No supports. **Red.** |
| `build_enclosure.py` | Fusion script that generates everything above (`D` holds all dimensions) | |

Slicer: PLA or PETG, 0.4 mm nozzle, 0.2 mm layers, 3 walls, 20–30 % infill, no supports, no brim needed.
Print the two caps at 100 % scale. `slide_clr` is **0.08 mm per side** (0.15 wobbled): if a cap binds, raise it in `D`, re-run the script and re-export. The rocker ends keep the old 0.15 mm (`rocker_end_clr`) because tilting moves its neck ~0.13 mm along Y (0.08 bound at ±2°). `sel_leg_gap` is only **0.05 mm**, less than the ±0.3 mm switch-height tolerance: if SELECT reads pressed at rest, raise it.

## Hardware needed
- OLED: **4 × M2.5 × 4 mm** pan-head screws, from below through the OLED PCB into the bosses (pilot hole Ø2.0, self-tapping). A longer screw (5–6 mm) would pierce the front plate.
- Perfboard (corner holes are **M2**): the four corner bosses have Ø1.6 pilot holes for M2 self-tapping screws. The back cover closes with **4 × M2 × 10 mm** from behind (Ø4.2 counterbore, 4 mm of post, through the perfboard, ~4.4 mm into the pilots).
- Battery: 36.6 × 46 × 8.25 mm LiPo, on edge in the top bay (46 along X). The checklist `battery-tp4056.md` still names the smaller EEMB 403048, so check which cell is really used; its R_prog note (1.2 k = 2C) is wrong for a bigger cell.
- TP4056 USB-C: 28 × 17.5 mm PCB, 4.15 mm thick including the USB-C (measured), on edge in the bottom bay with the USB-C towards +X (the rocker side).

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
- **OLED position (`oled_y` = 14.8):** history 17.3 (photo estimate) → 19.3 (+2.0) → **14.8 (−4.5, user, after a test fit)**. The whole module moves together: mounting holes/bosses (x ±15.5, y 0.55 and 29.05), window (plain rectangle, lit area 34.5 × 18.2, y 8.0–26.2) and the female header in the model (y 30.2, so the pins sit over the socket). The module's lower PCB edge (y −1.95) now reaches over the top of the rocker, so the rocker and its slot are 2.3 mm shorter at the top end (`rocker_top_trim`); the legs still sit on the switches. If the position is still off, adjust `oled_y`.
- The front plate leaves 0.4 mm over the OLED glass.

- Perfboard clearance is 0.6 mm per side (`side_clr`) after 0.3 mm was too tight. Since the case is wider now, nothing locates the perfboard sideways except the four screws; the battery rib stops at x ±25.6 so cables can pass on both sides.
- TP4056 estimates: PCB thickness 1.2 mm, USB-C receptacle 7.3 mm long × 9.0 mm wide, USB-C flush with the inner wall and centred on the 17.5 mm edge. The opening is 9.6 × 3.55 mm with a 13 × 7.5 × 1.0 mm outer recess for the plug overmold; the wall is thinner there because the corner is rounded. Measure the real receptacle and edit `tp_*`.
- The battery sits about 8 mm from the ESP32-S3 PCB antenna (top end): expect some Wi-Fi range loss (see the checklist).
- The screw posts are modelled at the perfboard holes; the ESP32 itself is held only by its headers (no rest on the back cover yet). No power switch, regulator or cable routing between the bays is modelled.

Fit checklist: `docs/hardware-checklists/enclosure.md`.
