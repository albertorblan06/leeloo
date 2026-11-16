"""Leeloo front enclosure: parametric build script for Autodesk Fusion (assembly design).

Run inside Fusion (Scripts and Add-Ins) or through the Fusion MCP `fusion_mcp_execute` tool:
    g = {}; exec(compile(open(PATH).read(), PATH, 'exec'), g); g['build'](); g['export_stl'](OUT_DIR)

Everything is in millimetres; the Fusion API works in cm, hence the /10 in the helpers.
Frame: perfboard centred on XY with its top (front) face at z=0, front = +Z.
`D` holds measured values and estimates (see README.md); edit it and re-run to regenerate.
"""
import math
import struct

import adsk.core
import adsk.fusion

VI = adsk.core.ValueInput
P = adsk.core.Point3D.create
NEW = adsk.fusion.FeatureOperations.NewBodyFeatureOperation
CUT = adsk.fusion.FeatureOperations.CutFeatureOperation
JOIN = adsk.fusion.FeatureOperations.JoinFeatureOperation

D = dict(
    # perfboard
    pcb_w=50, pcb_h=70, pcb_t=1.6, pcb_hole_d=2.2, pcb_hole_inset=2.0,   # perfboard holes are M2
    # OLED 1.3" module (measured: underside of PCB 12 mm above the perfboard; visible glass 34.5 x 18.2)
    # oled_z 10.6 = 12.0 - 1.4 (user: pins engaged too little at 12.0); fheader_h 8.1 is derived so the 2.5 mm
    # male header body sits on the socket (8.1 + 2.5 = 10.6), it was not measured.
    fheader_h=8.1, fheader_y=30.2,                            # = oled_y + 15.4 so the module pins sit over the socket
     mheader_h=2.5, oled_z=10.6,
    # oled_y: whole module, bosses and window move together. 17.3 (photo) -> 19.3 (+2.0) -> 14.8 (-4.5, user, after test fit)
    oled_pcb_w=35.5, oled_pcb_h=33.5, oled_pcb_t=1.2, oled_y=14.8,
    oled_hole_dx=31.0, oled_hole_dy=28.5, oled_hole_d=3.0,
    glass_w=35.0, glass_h=23.6, glass_t=1.4, glass_top_from_pcb_top=4.75,
    active_w=34.5, active_h=18.2, active_top_from_glass_top=0.6,
    # tact switches (measured totals 7.2 / 5.2 mm above the perfboard)
    sw12_pos=[(13.97, -21.4), (13.97, -6.16)], sw12_body=4.3, sw12_total=7.2, sw12_act_d=6.0,
    sw6_pos=(-10.16, -14.73), sw6_body=3.5, sw6_total=5.2, sw6_act_d=3.5,
    # back side (ESP32-S3-DevKitC-1 on female headers)
    esp_w=25.4, esp_y0=-30.0, esp_len=63.75, esp_t=1.2, hdr_x=11.43, hdr_y0=-27.4, hdr_y1=28.4,
    esp_mod_w=18.0, esp_mod_y0=8.0, esp_mod_y1=33.0, esp_mod_h=3.2,
    usb_w=9.0, usb_len=7.3, usb_h=3.2, usb_x=7.0,
    wire_out=4.0,
    # shell
    clr=0.3,                                  # vertical gap over the OLED glass
    side_clr=0.6,                             # per side around the 50 mm perfboard (0.3 was too tight)
    wall=2.0, plate=1.6, r_corner=4.0, win_margin=0.0,
    win_dy=0.0,                               # extra window offset relative to the OLED (the OLED itself already moved)
    boss_d=5.4, pilot_d=1.6,                  # perfboard corner bosses, M2 self-tapping (5.4 reaches the wall)
    front_fillet=0.8,                         # outer edge of the front face (no sharp edge on the bed side)
    oled_boss_d=4.2, oled_pilot_d=2.0, oled_pilot_depth=3.0,   # OLED bosses, M2.5 x 4 screws
    slot_w=12.0, knob_d=18.0,
    rocker_top_trim=2.3,                      # rocker/slot top end pulled down so it clears the lowered OLED PCB (edge at y -1.95)
    # buttons
    slide_clr=0.15,                           # per side, sliding fit on an FDM printer
    pad_up=2.5,                               # how far the pads stand proud of the plate (console style)
    pad_fillet_rocker=1.0, pad_fillet_select=1.2,
    flange_w=16.0, flange_t=2.2, leg_d=6.0, gap_half=1.5,
    rail_t=1.2, rail_h=3.5, rib_t=1.2, rib_h=0.8,
    sel_flange_d=21.0, sel_flange_t=1.8, sel_leg_d=6.0, sel_leg_gap=0.2, tube_wall=1.2,
)


# ---------------------------------------------------------------- helpers
def plane_at(c, z):
    pi = c.constructionPlanes.createInput()
    pi.setByOffset(c.xYConstructionPlane, VI.createByReal(z / 10.0))
    return c.constructionPlanes.add(pi)


def ext(c, sk, dist, op, target=None, profiles=None):
    col = adsk.core.ObjectCollection.create()
    for p in (profiles if profiles is not None else sk.profiles):
        col.add(p)
    ei = c.features.extrudeFeatures.createInput(col, op)
    ei.setDistanceExtent(False, VI.createByReal(dist / 10.0))
    if target is not None:
        ei.participantBodies = [target]
    return c.features.extrudeFeatures.add(ei)


def rect_sketch(c, name, x0, y0, x1, y1, z):
    sk = c.sketches.add(plane_at(c, z))
    sk.name = name
    sk.sketchCurves.sketchLines.addTwoPointRectangle(P(x0 / 10, y0 / 10, 0), P(x1 / 10, y1 / 10, 0))
    return sk


def circles_sketch(c, name, pts, d, z):
    sk = c.sketches.add(plane_at(c, z))
    sk.name = name
    for x, y in pts:
        sk.sketchCurves.sketchCircles.addByCenterRadius(P(x / 10, y / 10, 0), d / 20)
    return sk


def stadium_sketch(c, name, cx, ya, yb, r, z):
    """Slot outline: two arcs of radius r centred at (cx, ya) and (cx, yb), ya > yb."""
    sk = c.sketches.add(plane_at(c, z))
    sk.name = name
    L = sk.sketchCurves.sketchLines
    A = sk.sketchCurves.sketchArcs
    L.addByTwoPoints(P((cx - r) / 10, yb / 10, 0), P((cx - r) / 10, ya / 10, 0))
    L.addByTwoPoints(P((cx + r) / 10, yb / 10, 0), P((cx + r) / 10, ya / 10, 0))
    A.addByThreePoints(P((cx + r) / 10, ya / 10, 0), P(cx / 10, (ya + r) / 10, 0), P((cx - r) / 10, ya / 10, 0))
    A.addByThreePoints(P((cx - r) / 10, yb / 10, 0), P(cx / 10, (yb - r) / 10, 0), P((cx + r) / 10, yb / 10, 0))
    return sk


def box(c, name, x0, y0, x1, y1, z0, z1, op=NEW, target=None):
    f = ext(c, rect_sketch(c, name, x0, y0, x1, y1, z0), z1 - z0, op, target)
    if op == NEW:
        f.bodies.item(0).name = name
        return f.bodies.item(0)
    return None


def cyl(c, name, cx, cy, d, z0, z1, op=NEW, target=None):
    f = ext(c, circles_sketch(c, name, [(cx, cy)], d, z0), z1 - z0, op, target)
    if op == NEW:
        f.bodies.item(0).name = name
        return f.bodies.item(0)
    return None


def new_comp(root, name, x=0.0, y=0.0, z=0.0):
    m = adsk.core.Matrix3D.create()
    m.translation = adsk.core.Vector3D.create(x / 10.0, y / 10.0, z / 10.0)
    occ = root.occurrences.addNewComponent(m)
    occ.component.name = name
    return occ


def edges_at_z(body, z):
    """Edges of `body` that lie entirely in the plane z (mm)."""
    out = adsk.core.ObjectCollection.create()
    for e in body.edges:
        bb = e.boundingBox
        if abs(bb.minPoint.z * 10 - z) < 1e-3 and abs(bb.maxPoint.z * 10 - z) < 1e-3:
            out.add(e)
    return out


def fillet(c, edges, r):
    fi = c.features.filletFeatures.createInput()
    fi.addConstantRadiusEdgeSet(edges, VI.createByReal(r / 10.0), True)
    c.features.filletFeatures.add(fi)


def chamfer(c, edges, d):
    ci = c.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, VI.createByReal(d / 10.0), True)
    c.features.chamferFeatures.add(ci)


# ---------------------------------------------------------------- build
def build():
    d = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct)
    root = d.rootComponent
    for i in range(root.occurrences.count - 1, -1, -1):
        root.occurrences.item(i).deleteMe()
    k = D
    occs = {}

    # ---- digital twin of the electronics ----
    o = new_comp(root, 'Perfboard'); c = o.component; occs['Perfboard'] = o
    b = box(c, 'Perfboard', -k['pcb_w'] / 2, -k['pcb_h'] / 2, k['pcb_w'] / 2, k['pcb_h'] / 2, -k['pcb_t'], 0)
    ix = k['pcb_w'] / 2 - k['pcb_hole_inset']; iy = k['pcb_h'] / 2 - k['pcb_hole_inset']
    corners = [(ix, iy), (-ix, iy), (ix, -iy), (-ix, -iy)]
    ext(c, circles_sketch(c, 'holes', corners, k['pcb_hole_d'], -k['pcb_t']), k['pcb_t'], CUT, b)

    o = new_comp(root, 'Header_F_1x4', 0, k['fheader_y'], 0); c = o.component; occs['Header'] = o
    box(c, 'Header body', -5.08, -1.27, 5.08, 1.27, 0, k['fheader_h'])

    o = new_comp(root, 'OLED_1.3', 0, k['oled_y'], k['oled_z']); c = o.component; occs['OLED'] = o
    w, h, t = k['oled_pcb_w'], k['oled_pcb_h'], k['oled_pcb_t']
    b = box(c, 'OLED PCB', -w / 2, -h / 2, w / 2, h / 2, 0, t)
    ext(c, rect_sketch(c, 'notch', -5.5, -h / 2, 5.5, -h / 2 + 5.0, 0), t, CUT, b)
    hx, hy = k['oled_hole_dx'] / 2, k['oled_hole_dy'] / 2
    oled_holes = [(hx, hy), (-hx, hy), (hx, -hy), (-hx, -hy)]
    ext(c, circles_sketch(c, 'holes', oled_holes, k['oled_hole_d'], 0), t, CUT, b)
    gtop = h / 2 - k['glass_top_from_pcb_top']
    box(c, 'OLED glass', -k['glass_w'] / 2, gtop - k['glass_h'], k['glass_w'] / 2, gtop, t, t + k['glass_t'])
    atop = gtop - k['active_top_from_glass_top']
    abot = atop - k['active_h']
    box(c, 'OLED visible area', -k['active_w'] / 2, abot, k['active_w'] / 2, atop, t + k['glass_t'], t + k['glass_t'] + 0.05)
    box(c, 'OLED male header', -5.08, h / 2 - 1.35 - 1.27, 5.08, h / 2 - 1.35 + 1.27, -k['mheader_h'], 0)
    oled_top_pcb_z = k['oled_z'] + t
    glass_top_z = oled_top_pcb_z + k['glass_t']

    for i, (x, y) in enumerate(k['sw12_pos']):
        o = new_comp(root, 'Tact_12x12_%d' % (i + 1), x, y, 0); c = o.component; occs['T12_%d' % i] = o
        box(c, 'Switch body', -6, -6, 6, 6, 0, k['sw12_body'])
        cyl(c, 'Switch actuator', 0, 0, k['sw12_act_d'], k['sw12_body'], k['sw12_total'])
    x6, y6 = k['sw6_pos']
    o = new_comp(root, 'Tact_6x6', x6, y6, 0); c = o.component; occs['T6'] = o
    box(c, 'Switch body', -3, -3, 3, 3, 0, k['sw6_body'])
    cyl(c, 'Switch actuator', 0, 0, k['sw6_act_d'], k['sw6_body'], k['sw6_total'])

    pz = -k['pcb_t']
    o = new_comp(root, 'Headers_F_ESP'); c = o.component; occs['HdrESP'] = o
    for s in (-1, 1):
        xc = s * k['hdr_x']
        box(c, 'Header strip %s' % ('L' if s < 0 else 'R'), xc - 1.27, k['hdr_y0'], xc + 1.27, k['hdr_y1'], pz - k['fheader_h'], pz)
    ez1 = pz - k['fheader_h']; ez0 = ez1 - k['esp_t']
    o = new_comp(root, 'ESP32_S3_DevKitC1'); c = o.component; occs['ESP'] = o
    box(c, 'ESP PCB', -k['esp_w'] / 2, k['esp_y0'], k['esp_w'] / 2, k['esp_y0'] + k['esp_len'], ez0, ez1)
    box(c, 'ESP module', -k['esp_mod_w'] / 2, k['esp_mod_y0'], k['esp_mod_w'] / 2, k['esp_mod_y1'], ez0 - k['esp_mod_h'], ez0)
    for s in (-1, 1):
        xc = s * k['usb_x']
        box(c, 'USB-C %s' % ('L' if s < 0 else 'R'), xc - k['usb_w'] / 2, k['esp_y0'], xc + k['usb_w'] / 2, k['esp_y0'] + k['usb_len'], ez0 - k['usb_h'], ez0)

    o = new_comp(root, 'Wire_keepout'); c = o.component; occs['Wires'] = o
    ye = k['pcb_h'] / 2
    box(c, 'Wire top', -20, ye, 20.4, ye + k['wire_out'], -2.5, 2.5)
    box(c, 'Wire bottom', -11, -ye - k['wire_out'], -6, -ye, -2.5, 2.5)

    # ---- front shell ----
    o = new_comp(root, 'FrontShell'); c = o.component; occs['Shell'] = o
    hw = k['pcb_w'] / 2 + k['side_clr'] + k['wall']
    hh = k['pcb_h'] / 2 + k['wire_out'] + k['side_clr'] + k['wall']
    z_bot = -k['pcb_t']
    z_plate = glass_top_z + k['clr'] + 0.1          # underside of the front plate
    z_top = z_plate + k['plate']
    sk = rect_sketch(c, 'shell outer', -hw, -hh, hw, hh, z_bot)
    body = ext(c, sk, z_top - z_bot, NEW).bodies.item(0)
    body.name = 'Front shell'
    edges = adsk.core.ObjectCollection.create()
    for e in body.edges:
        g = e.geometry
        if g.objectType == adsk.core.Line3D.classType():
            dv = g.startPoint.vectorTo(g.endPoint)
            if abs(abs(dv.z) - dv.length) < 1e-6:
                edges.add(e)
    fillet(c, edges, k['r_corner'])
    fillet(c, edges_at_z(body, z_top), k['front_fillet'])     # soften the front-face perimeter

    cw = k['pcb_w'] / 2 + k['side_clr']; ch = k['pcb_h'] / 2 + k['wire_out'] + k['side_clr']
    ext(c, rect_sketch(c, 'shell cavity', -cw, -ch, cw, ch, z_bot), z_plate - z_bot, CUT, body)

    # plate cutouts
    sk = c.sketches.add(plane_at(c, z_plate)); sk.name = 'plate cutouts'
    L = sk.sketchCurves.sketchLines; A = sk.sketchCurves.sketchArcs
    m = k['win_margin']
    wx = k['active_w'] / 2 + m
    wy0 = k['oled_y'] + abot - m + k['win_dy']; wy1 = k['oled_y'] + atop + m + k['win_dy']
    L.addTwoPointRectangle(P(-wx / 10, wy0 / 10, 0), P(wx / 10, wy1 / 10, 0))   # plain window: lit area only
    sx = k['sw12_pos'][0][0]; ya = k['sw12_pos'][1][1]; yb = k['sw12_pos'][0][1]   # upper / lower switch
    yt = ya - k['rocker_top_trim']                # centre of the slot's upper end arc (legs stay on the switches)
    r = k['slot_w'] / 2
    L.addByTwoPoints(P((sx - r) / 10, yb / 10, 0), P((sx - r) / 10, yt / 10, 0))
    L.addByTwoPoints(P((sx + r) / 10, yb / 10, 0), P((sx + r) / 10, yt / 10, 0))
    A.addByThreePoints(P((sx + r) / 10, yt / 10, 0), P(sx / 10, (yt + r) / 10, 0), P((sx - r) / 10, yt / 10, 0))
    A.addByThreePoints(P((sx - r) / 10, yb / 10, 0), P(sx / 10, (yb - r) / 10, 0), P((sx + r) / 10, yb / 10, 0))
    sk.sketchCurves.sketchCircles.addByCenterRadius(P(x6 / 10, y6 / 10, 0), k['knob_d'] / 20)
    ext(c, sk, 2.0 + k['plate'], CUT, body)

    # perfboard corner bosses (for the back half) with M2.5 pilot holes
    ext(c, circles_sketch(c, 'bosses', corners, k['boss_d'], 0), z_plate, JOIN, body)
    ext(c, circles_sketch(c, 'pilot holes', corners, k['pilot_d'], 0), 6.0, CUT, body)

    # OLED bosses: the module is clamped between these and the screw heads (M2.5 x 4 from below)
    oled_pts = [(x, k['oled_y'] + y) for x, y in oled_holes]
    ext(c, circles_sketch(c, 'oled bosses', oled_pts, k['oled_boss_d'], oled_top_pcb_z), z_plate - oled_top_pcb_z, JOIN, body)
    ext(c, circles_sketch(c, 'oled pilot holes', oled_pts, k['oled_pilot_d'], oled_top_pcb_z), k['oled_pilot_depth'], CUT, body)

    # rocker guides: two rails along the flange and a fulcrum rib across the middle
    yc = (ya + yb) / 2
    rx = k['flange_w'] / 2 + k['slide_clr']
    y_lo = yb - r
    y_hi = yt + r - k['slide_clr']   # rails end with the flange, short of the OLED PCB
    rail_z0 = z_plate - k['rail_h']
    for s in (-1, 1):
        x0 = sx + s * rx
        box(c, 'rail', min(x0, x0 + s * k['rail_t']), y_lo, max(x0, x0 + s * k['rail_t']), y_hi, rail_z0, z_plate, JOIN, body)
    box(c, 'fulcrum rib', sx - rx - 0.01, yc - k['rib_t'] / 2, sx + rx + 0.01, yc + k['rib_t'] / 2, z_plate - k['rib_h'], z_plate, JOIN, body)

    # SELECT guide tube around the cap flange
    tr_out = k['sel_flange_d'] / 2 + k['slide_clr'] + k['tube_wall']
    tr_in = k['sel_flange_d'] / 2 + k['slide_clr']
    tube_z0 = z_plate - k['sel_flange_t'] - 0.6
    sk = c.sketches.add(plane_at(c, tube_z0)); sk.name = 'select tube'
    sk.sketchCurves.sketchCircles.addByCenterRadius(P(x6 / 10, y6 / 10, 0), tr_out / 10)
    sk.sketchCurves.sketchCircles.addByCenterRadius(P(x6 / 10, y6 / 10, 0), tr_in / 10)
    ring = min(sk.profiles, key=lambda p: p.areaProperties().area)
    ext(c, sk, z_plate - tube_z0, JOIN, body, profiles=[ring])

    # ---- rocker (UP/DOWN) ----
    o = new_comp(root, 'Rocker'); c = o.component; occs['Rocker'] = o
    fz0 = z_plate - k['rib_h'] - k['flange_t']          # flange sits on the fulcrum crest
    fz1 = fz0 + k['flange_t']
    plen_lo = yb - (r - k['slide_clr']); plen_hi = yt + (r - k['slide_clr'])
    rk = box(c, 'Rocker', sx - k['flange_w'] / 2, plen_lo, sx + k['flange_w'] / 2, plen_hi, fz0, fz1)
    ce = adsk.core.ObjectCollection.create()
    for e in rk.edges:
        g = e.geometry
        if g.objectType != adsk.core.Line3D.classType():
            continue
        dv = g.startPoint.vectorTo(g.endPoint)
        if abs(abs(dv.y) - dv.length) < 1e-6 and abs(g.startPoint.z * 10 - fz0) < 1e-3 and abs(abs(g.startPoint.x * 10 - sx) - k['flange_w'] / 2) < 1e-3:
            ce.add(e)
    chamfer(c, ce, k['flange_t'] - 0.1)
    pr = r - k['slide_clr']
    ext(c, stadium_sketch(c, 'necks', sx, yt, yb, pr, fz1), z_plate + k['plate'] - fz1, JOIN, rk)
    ext(c, rect_sketch(c, 'neck gap', sx - 9, yc - k['gap_half'], sx + 9, yc + k['gap_half'], fz1), z_plate + k['plate'] - fz1, CUT, rk)
    ext(c, stadium_sketch(c, 'pad', sx, yt, yb, pr, z_top), k['pad_up'], JOIN, rk)
    fillet(c, edges_at_z(rk, z_top + k['pad_up']), k['pad_fillet_rocker'])
    leg_z0 = k['sw12_total']
    ext(c, circles_sketch(c, 'legs', [(sx, ya), (sx, yb)], k['leg_d'], leg_z0), fz0 - leg_z0, JOIN, rk)

    # ---- SELECT cap ----
    o = new_comp(root, 'SelectCap'); c = o.component; occs['Select'] = o
    sz0 = z_plate - k['sel_flange_t']
    sc = cyl(c, 'Select cap', x6, y6, k['sel_flange_d'], sz0, z_plate)
    ce = adsk.core.ObjectCollection.create()
    for e in sc.edges:
        g = e.geometry
        if g.objectType == adsk.core.Circle3D.classType() and abs(g.center.z * 10 - sz0) < 1e-3:
            ce.add(e)
    chamfer(c, ce, k['sel_flange_t'] - 0.1)
    cyl(c, 'pad', x6, y6, k['knob_d'] - 2 * k['slide_clr'], z_plate, z_top + k['pad_up'], JOIN, sc)
    fillet(c, edges_at_z(sc, z_top + k['pad_up']), k['pad_fillet_select'])
    leg_bot = k['sw6_total'] + k['sel_leg_gap']
    cyl(c, 'leg', x6, y6, k['sel_leg_d'], leg_bot, sz0, JOIN, sc)

    apply_colours(d, occs)
    return d, occs, dict(z_plate=z_plate, z_top=z_top, fz0=fz0, fz1=fz1, yc=yc, sx=sx)


# Library appearance names depend on the Fusion UI language (Spanish first, English fallback).
COLOURS = {
    # Escaped (backslash-u 00e1 = a with acute) so the file stays ASCII: Fusion's open() defaults to ASCII
    'Shell': ('Pl\u00e1stico - Mate (negro)', 'Plastic - Matte (Black)'),      # body: black
    'Rocker': ('Pl\u00e1stico - Mate (blanco)', 'Plastic - Matte (White)'),    # menu scroll: white
    'Select': ('Pl\u00e1stico - Mate (rojo)', 'Plastic - Matte (Red)'),        # accept: red
}


def apply_colours(d, occs):
    app = adsk.core.Application.get()
    lib = app.materialLibraries.itemByName(
        next(l.name for l in app.materialLibraries if 'aspectos' in l.name.lower() or 'appearance' in l.name.lower()))
    for key, names in COLOURS.items():
        lib_app = next((lib.appearances.itemByName(n) for n in names if lib.appearances.itemByName(n)), None)
        if lib_app is None:
            raise RuntimeError('appearance not found in the library: %s' % (names,))
        mine = d.appearances.itemByName(lib_app.name) or d.appearances.addByCopy(lib_app, lib_app.name)
        for b in occs[key].bRepBodies:
            b.appearance = mine


def interferences(d, occs, names=None):
    col = adsk.core.ObjectCollection.create()
    for n, oc in occs.items():
        if names is None or n in names:
            for b in oc.bRepBodies:
                col.add(b)
    inp = d.createInterferenceInput(col)
    inp.areCoincidentFacesIncluded = False
    res = d.analyzeInterference(inp)
    return [(res.item(i).entityOne.name, res.item(i).entityTwo.name) for i in range(res.count)]


def check(d, occs, geo):
    print('interferences (rest pose):', interferences(d, occs))
    # rocker tilt about the fulcrum crest, against the shell only
    rock = occs['Rocker']
    axis = adsk.core.Vector3D.create(1, 0, 0)
    for deg in (3, -3):
        # the flange rolls on the rib edge that stays in contact, not on the rib centre
        yp = geo['yc'] + math.copysign(D['rib_t'] / 2, deg)
        m = adsk.core.Matrix3D.create()
        m.setToRotation(math.radians(deg), axis, P(geo['sx'] / 10, yp / 10, geo['fz1'] / 10))
        rock.transform2 = m
        print('rocker %+d deg vs shell:' % deg, interferences(d, occs, ('Rocker', 'Shell')))
    rock.transform2 = adsk.core.Matrix3D.create()


def run(_context: str):
    d, occs, geo = build()
    check(d, occs, geo)


# ---------------------------------------------------------------- print layout + STL export
def export_stl(out_dir):
    """Export front_shell.stl / rocker.stl / select_cap.stl, each laid out as it should print."""
    app = adsk.core.Application.get()
    d = adsk.fusion.Design.cast(app.activeProduct)
    root = d.rootComponent
    jobs = [('FrontShell', 'front_shell'), ('Rocker', 'rocker'), ('SelectCap', 'select_cap')]
    em = d.exportManager
    for i in range(root.occurrences.count - 1, -1, -1):
        if root.occurrences.item(i).component.name.startswith('print_'):
            root.occurrences.item(i).deleteMe()
    for comp_name, fname in jobs:
        src = None
        for oc in root.occurrences:
            if oc.component.name == comp_name:
                src = oc
        path = '%s/%s.stl' % (out_dir, fname)
        opts = em.createSTLExportOptions(src, path)
        opts.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
        opts.sendToPrintUtility = False
        print(fname, 'exported' if em.execute(opts) else 'EXPORT FAILED')
        orient_stl_for_print(path)


def orient_stl_for_print(path):
    """Rewrite a binary STL in place: rotate 180 deg about X (front plate / pad top face down), then sit it on z=0.

    Fusion's STL export keeps the assembly pose, so the print orientation is applied to the file instead.
    """
    with open(path, 'rb') as f:
        data = bytearray(f.read())
    n = struct.unpack('<I', data[80:84])[0]
    tris = []
    for i in range(n):
        v = list(struct.unpack('<12f', data[84 + 50 * i: 84 + 50 * i + 48]))
        for j in (0, 3, 6, 9):           # normal, v1, v2, v3
            v[j + 1] = -v[j + 1]
            v[j + 2] = -v[j + 2]
        tris.append(v)
    zmin = min(v[j + 2] for v in tris for j in (3, 6, 9))
    out = bytearray(data[:84])
    for v in tris:
        for j in (3, 6, 9):
            v[j + 2] -= zmin
        out += struct.pack('<12fH', *v, 0)
    with open(path, 'wb') as f:
        f.write(out)


def stl_bbox(path):
    """Bounding box (mm) and triangle count of a binary STL, stdlib only."""
    with open(path, 'rb') as f:
        data = f.read()
    n = struct.unpack('<I', data[80:84])[0]
    lo = [1e18] * 3
    hi = [-1e18] * 3
    for i in range(n):
        vals = struct.unpack('<12fH', data[84 + 50 * i: 84 + 50 * (i + 1)])
        for v in range(3):
            for ax in range(3):
                c = vals[3 + 3 * v + ax]
                lo[ax] = min(lo[ax], c)
                hi[ax] = max(hi[ax], c)
    return n, lo, hi
