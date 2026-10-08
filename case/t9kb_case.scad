// T9KB case: top shell with key cutouts + snap-in back lid. PETG recommended.
// The board and key mat drop into the top shell face down, then the lid snaps in and clamps them.
// Coordinates follow the board's key face: x right, y down from the top left corner, z = 0 at the key face.
// Export: openscad -D 'PART="top"' / "lid", PRINT=true lays each part flat for printing.

use <../mat/t9kb_mat.scad>

PART  = "case";  // top, lid, case (both, closed)
PRINT = false;

W = 48;          // board
H = 68;
R = 2;

CLR    = 0.3;    // board / mat to wall
WALL   = 2.0;    // side walls
PLATE  = 1.6;    // top plate over the mat base
MAT    = 1.6;    // mat base thickness (mat BEZEL)
KEYCLR = 0.3;    // key cutout clearance
PCB    = 1.6;
BACK   = 3.5;    // room under the board for the parts (tallest is the Qwiic jack, 2.96 mm)
FLOOR  = 2.0;    // lid thickness
LIDCLR = 0.15;   // lid to wall
SKIRT  = 1.2;    // lid skirt thickness, also the ledge the board rests on
BEAD   = 0.45;   // snap bead on the lid skirt, catches 0.3 mm past the 0.15 mm lid gap
BEAD_Z = -3.6;   // snap height

Z_TOP   = MAT + PLATE;             // 3.2
Z_BOARD = -PCB;                    // -1.6, parts side of the board
Z_IN    = Z_BOARD - BACK;          // -5.1, top of the lid
Z_BOT   = Z_IN - FLOOR;            // -7.1

QWIIC_X = 16;    // Qwiic jack center on the top edge (key face x)
WIRE_X  = 24;    // breakout pads center on the bottom edge

$fn = 64;

module board2d(d = 0) {
	offset(R + d) offset(-R) square([W, H]);
}

module ring(d0, d1) {
	difference() {
		board2d(d1);
		board2d(d0);
	}
}

// key-face y to OpenSCAD y
function Y(y) = H - y;

module bead(len) {
	// lead-in on top (the lid goes in from below), steeper retaining face underneath
	rotate([90, 0, 0]) linear_extrude(len, center = true) polygon([[0, -0.5], [BEAD, 0], [0, 0.9]]);
}

module top() {
	difference() {
		union() {
			// plate
			translate([0, 0, MAT]) linear_extrude(PLATE) board2d(CLR + WALL);
			// walls
			translate([0, 0, Z_BOT]) linear_extrude(Z_TOP - Z_BOT) ring(CLR, CLR + WALL);
		}
		// key holes
		translate([0, 0, MAT - 0.01]) linear_extrude(PLATE + 0.02) keys2d(KEYCLR);
		// snap groove for the lid bead
		translate([0, 0, BEAD_Z - 0.7]) linear_extrude(1.6) ring(CLR - 0.01, CLR + BEAD + 0.1);
		// Qwiic slot
		translate([QWIIC_X - 4.5, H - 0.5, Z_BOARD - 3.4]) cube([9, CLR + WALL + 1.5, 3.6]);
		// wire hole
		translate([WIRE_X - 3.5, -CLR - WALL - 1, Z_BOARD - 2.6]) cube([7, CLR + WALL + 1.5, 2.2]);
	}
}

module lid() {
	difference() {
		union() {
			translate([0, 0, Z_BOT]) linear_extrude(FLOOR) board2d(CLR - LIDCLR);
			// skirt: rises to the board and holds it against the mat
			translate([0, 0, Z_IN - 0.01]) linear_extrude(BACK + 0.01) ring(CLR - LIDCLR - SKIRT, CLR - LIDCLR);
			// snap beads on the long sides, clear of the corners
			for (x = [-CLR + LIDCLR, W + CLR - LIDCLR]) translate([x, H / 2, BEAD_Z]) rotate([0, 0, x < W / 2 ? 180 : 0]) bead(40);
		}
		// clear the Qwiic jack and its plug
		translate([QWIIC_X - 5, Y(5), Z_IN]) cube([10, 10, BACK + 1]);
		// clear the breakout pads and their wires
		translate([WIRE_X - 8, Y(H + 5), Z_IN]) cube([16, 7, BACK + 1]);
	}
}

if (PART == "top") {
	if (PRINT) translate([0, H, Z_TOP]) rotate([180, 0, 0]) top();
	else color("#d0d0d0") top();
}
if (PART == "lid") {
	if (PRINT) translate([0, 0, -Z_BOT]) lid();
	else color("#9a9a9a") lid();
}
if (PART == "case") {
	color("#d0d0d0") top();
	color("#9a9a9a") lid();
}
