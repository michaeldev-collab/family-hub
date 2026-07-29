// Family Hub — Waveshare ESP32-S3-Touch-LCD-7B wall case
// OpenSCAD · Units: mm
//
// Mounting:
//   Panel → case : Waveshare peelable adhesive sticker on the module back
//                  (flat landing pad inside; NO M3 posts, NO raised seat lip)
//   Case  → wall : TWO Velcro Command strips (recesses on outer back)
//   Outer front rim frames the glass edge (alignment + light retention)
//
// Facing screen (+Z toward viewer, +X right, +Y up):
//   LEFT  — large bay for USB-C / microSD
//   TOP   — large bay for Boot / Reset / SD
//   RIGHT — MX switch bay (PCB up, switch at bottom)
//
// Build (from hardware/cad):
//   openscad -o family-hub-7b-case.stl family-hub-7b-case.scad
//   # optional: copy dated builds into exports/

$fn = 48;

/* ===== Official Waveshare touch 7B drawing ===== */
panel_w = 192.96;
panel_h = 110.76;
active_w = 154.88;
active_h = 86.72;
bezel_left = 19.04;
bezel_right = 19.04;
bezel_top = 11.00;
bezel_bottom = 13.04;

/* ===== Case ===== */
wall = 2.4;
front_rim = 2.0;        // outer lip around glass (frames the panel)
clearance = 0.5;        // around panel outer edge
cavity_z = 22.0;        // electronics depth behind stuck panel
back_thick = 2.8;       // Command-strip + sticker landing face
corner_r = 4.0;         // measured with radius gauge

inner_w = panel_w + 2 * clearance;
inner_h = panel_h + 2 * clearance;
outer_w = inner_w + 2 * wall;
outer_h = inner_h + 2 * wall;
outer_z = back_thick + cavity_z + front_rim;

/* ===== Adhesive sticker landing (panel → case) ===== */
// Flat pad on the inside of the back where the Waveshare peel sticker bonds.
// Sized to the metal/PCB back footprint; tweak if your sticker is smaller.
sticker_pad_w = 120;
sticker_pad_h = 70;
// Slightly proud so glue contacts pad first, not the whole back texture
sticker_pad_raise = 0.4;

/* ===== TWO Velcro Command strips (case → wall) ===== */
// Typical large Velcro Command strip footprint (pair: one wall, one case)
cmd_w = 70;
cmd_h = 20;
cmd_recess = 0.8;
// Two strips, left/right of center on the outer back
cmd_gap = 24;           // clear space between the two strips
cmd_y = (outer_h - cmd_h) / 2;

/* ===== LEFT: big USB / microSD access ===== */
left_access_y = wall + clearance + 28;
left_access_h = 55;
left_access_z = back_thick + 3;
left_access_d = cavity_z - 4;

/* ===== TOP: big Boot / Reset / SD access ===== */
top_access_x = wall + clearance + 20;
top_access_w = 120;
top_access_z = back_thick + 3;
top_access_d = cavity_z - 4;

/* ===== RIGHT: MX switch bay ===== */
sw_pcb_w = 20.8;
sw_pcb_len = 81.5;
sw_pcb_thick = 6.0;
sw_face = 15.4;
sw_clear = 0.7;
sw_from_bottom = 16;
sw_z = back_thick + 5;

module rounded_rect(size, r) {
  w = size[0]; h = size[1];
  hull() {
    translate([r, r]) circle(r = r);
    translate([w - r, r]) circle(r = r);
    translate([r, h - r]) circle(r = r);
    translate([w - r, h - r]) circle(r = r);
  }
}

module case_body() {
  difference() {
    linear_extrude(outer_z)
      rounded_rect([outer_w, outer_h], corner_r);

    // Electronics cavity (open to front rim plane)
    translate([wall, wall, back_thick])
      cube([inner_w, inner_h, cavity_z + front_rim + 0.2]);

    // Front window — active glass (outer rim holds around the bezel)
    translate([
      wall + clearance + bezel_left - 0.3,
      wall + clearance + bezel_bottom - 0.3,
      outer_z - front_rim - 0.05
    ])
      cube([active_w + 0.6, active_h + 0.6, front_rim + 0.2]);

    // Shallow pocket so the panel outer bezel nests in the rim
    // (not a raised seat lip — just the outer frame wrapping the edge)
    translate([wall, wall, outer_z - front_rim - 1.0])
      cube([inner_w, inner_h, 1.1]);

    // --- LEFT: big USB / microSD access ---
    translate([-0.2, left_access_y, left_access_z])
      cube([wall + 0.4, left_access_h, left_access_d]);

    // --- TOP: big Boot / Reset / SD access ---
    translate([top_access_x, outer_h - wall - 0.1, top_access_z])
      cube([top_access_w, wall + 0.4, top_access_d]);

    // --- RIGHT: MX face + PCB pocket ---
    translate([outer_w - wall - 0.1, wall + clearance + sw_from_bottom, sw_z])
      cube([wall + 0.4, sw_face, sw_face]);

    translate([
      outer_w - wall - sw_pcb_thick - sw_clear,
      wall + clearance + sw_from_bottom - sw_clear,
      sw_z - sw_clear
    ])
      cube([
        sw_pcb_thick + sw_clear + 0.3,
        sw_pcb_len + 2 * sw_clear,
        max(sw_face, sw_pcb_w) + 2 * sw_clear
      ]);

    // --- TWO Velcro Command strip recesses (outer back, Z=0) ---
    cmd_x0 = (outer_w - (2 * cmd_w + cmd_gap)) / 2;
    translate([cmd_x0, cmd_y, -0.02])
      cube([cmd_w, cmd_h, cmd_recess]);
    translate([cmd_x0 + cmd_w + cmd_gap, cmd_y, -0.02])
      cube([cmd_w, cmd_h, cmd_recess]);
  }

  // Adhesive landing pad — inside back, centered (panel sticker bonds here)
  translate([
    (outer_w - sticker_pad_w) / 2,
    (outer_h - sticker_pad_h) / 2,
    back_thick
  ])
    cube([sticker_pad_w, sticker_pad_h, sticker_pad_raise]);
}

case_body();
