// T9KB case: top shell with key cutouts + a screwed-on back lid.
// The keycaps and board drop into the top shell face down, then the lid goes on and 4 M1.4 self-tapping screws
// through the board's corner holes (H1-H4) into the face plate clamp the stack. The Qwiic jack sits in a lid window.
// Under the plate a spacer holds the board 1.6 mm down, with a pocket for every keycap flange.
// Coordinates follow the board's key face: x right, y down from the top left corner, z = 0 at the key face.
// Export: openscad -D 'PART="top"' / "lid", PRINT=true lays each part flat for printing.

use <../keys/t9kb_keys.scad>

PART  = "case";  // top, lid, case (both, closed), keys (case with the keycaps)
PRINT = false;

W = 48;          // board
H = 68;
R = 2;

CLR    = 0.3;    // board to wall
WALL   = 2.0;    // side walls
PLATE  = 2.56;   // top plate, deep key holes (keys PLATE)
SPACER = 1.6;    // board to plate underside, room for the keycap flanges (keys SPACER)
KEYCLR = 0.25;   // key hole clearance (keys HOLE)
POCKET = 1.0;    // flange pocket around each key: 0.75 flange + 0.25 clearance
BAND   = 1.0;    // spacer rim that rests on the board edge
RELIEF = 0.3;    // the rest of the spacer stays this far off the board, clear of the dome adhesive
PCB    = 1.6;
BACK   = 1.8;    // room under the board for the parts (U1 0.8 mm); the 2.96 mm Qwiic jack drops into a lid window, flush with the back
FLOOR  = 1.2;    // lid thickness
LIDCLR = 0.15;   // lid to wall
SKIRT  = 1.2;    // lid skirt thickness, also the ledge the board rests on

Z_TOP   = SPACER + PLATE;          // 4.16
Z_BOARD = -PCB;                    // -1.6, parts side of the board
Z_IN    = Z_BOARD - BACK;          // -3.4, top of the lid
Z_BOT   = Z_IN - FLOOR;            // -4.6

QWIIC_X = 16;    // Qwiic jack center on the top edge (key face x)
WIRE_X  = 24;    // breakout pads center on the bottom edge

// M1.4 screw holes in the board corners (key-face x, y down), 1.5 mm holes on the board
SCREWS   = [[1.2, 66.3], [46.6, 66.6], [1.3, 1.5], [46.5, 1.3]];
PILOT    = 1.2;  // self-tapping pilot in the top, stops 0.56 mm under the face
SCREW_D  = 1.7;  // lid clearance hole
BOSS_D   = 3.4;  // lid boss under each board corner: 0.85 mm wall, two printed lines

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

module top() {
	difference() {
		union() {
			// plate
			translate([0, 0, SPACER]) linear_extrude(PLATE) board2d(CLR + WALL);
			// spacer: rim on the board edge, the rest relieved
			linear_extrude(SPACER) ring(-BAND, CLR);
			translate([0, 0, RELIEF]) linear_extrude(SPACER - RELIEF) difference() {
				board2d(CLR);
				caps2d(POCKET);
			}
			// walls
			translate([0, 0, Z_BOT]) linear_extrude(Z_TOP - Z_BOT) ring(CLR, CLR + WALL);
		}
		// key holes
		translate([0, 0, SPACER - 0.01]) linear_extrude(PLATE + 0.02) caps2d(KEYCLR);
		// Qwiic slot, open through the back edge
		translate([QWIIC_X - 4.5, H - 0.5, Z_BOT - 1]) cube([9, CLR + WALL + 1.5, Z_BOARD - Z_BOT + 1.2]);
		// screw pilots up into the plate
		for (s = SCREWS) translate([s[0], Y(s[1]), -0.01]) cylinder(d = PILOT, h = Z_TOP - 0.56, $fn = 24);
		// wire hole
		translate([WIRE_X - 3.5, -CLR - WALL - 1, Z_BOARD - 2.6]) cube([7, CLR + WALL + 1.5, 2.2]);
	}
}

module lid() {
	difference() {
		union() {
			translate([0, 0, Z_BOT]) linear_extrude(FLOOR) board2d(CLR - LIDCLR);
			// skirt: rises to the board and holds it against the spacer
			translate([0, 0, Z_IN - 0.01]) linear_extrude(BACK + 0.01) ring(CLR - LIDCLR - SKIRT, CLR - LIDCLR);
			// screw bosses under the board corners
			intersection() {
				for (s = SCREWS) translate([s[0], Y(s[1]), Z_IN - 0.01]) cylinder(d = BOSS_D, h = BACK + 0.01, $fn = 32);
				translate([0, 0, Z_IN - 1]) linear_extrude(BACK + 2) board2d(CLR - LIDCLR);
			}
		}
		// Qwiic jack and plug: a window through the floor, the jack sits in it
		translate([QWIIC_X - 5, Y(7), Z_BOT - 1]) cube([10, 12, FLOOR + BACK + 2]);
		// screw clearance holes
		for (s = SCREWS) translate([s[0], Y(s[1]), Z_BOT - 1]) cylinder(d = SCREW_D, h = FLOOR + BACK + 2, $fn = 24);
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
if (PART == "case" || PART == "keys") {
	color("#d0d0d0") top();
	color("#9a9a9a") lid();
}
if (PART == "keys") part();
