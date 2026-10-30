// T9KB phone case: candybar shell with the keypad at the bottom and a Waveshare 2.4in LCD module above it,
// plus a snap-in back lid that holds the XIAO ESP32-C5, a MAX98357A amp board and an INMP441 mic board.
// The keypad board and the screen board lie in the same plane, edge to edge. The screen hangs on its four 5.6 mm
// standoffs: M2 screws go through the lid into them, which clamps the screen and holds the lid on.
// Coordinates: x right, y up from the keypad board's bottom edge, z = 0 at the key face (and the screen board's front).
// Export: openscad -D 'PART="top"' / "lid", PRINT=true lays each part flat for printing.

use <../keys/t9kb_keys.scad>

PART  = "case";  // top, lid, case (both, closed), parts (case with keys, boards and modules, for previews)
PRINT = false;

W = 48;          // keypad board
H = 68;
R = 2;

CLR    = 0.3;    // boards to wall
WALL   = 2.0;
PLATE  = 2.56;   // keypad plate, deep key holes (keys PLATE)
SPACER = 1.6;    // keypad board to plate underside (keys SPACER)
KEYCLR = 0.25;
POCKET = 1.0;
BAND   = 1.0;
RELIEF = 0.3;
PCB    = 1.6;
FLOOR  = 2.0;
LIDCLR = 0.15;
SKIRT  = 1.2;
BEAD   = 0.45;
BEAD_Z = -3.6;

// Waveshare 2.4inch LCD Module, mounted with its pin row at the keypad end: 70.6 x 43.2 board, 60 x 43.1 glass
// standing 3 mm off it, 5.6 mm of board past the glass at the top end. Picture: the glass less 1.7 mm at the top and
// sides, 7.5 mm at the keypad end. Four 5.6 mm M2 standoffs on the back, about 2.5 mm in from the board edges.
// Its cable connector is on the back of the top end strip, centred, the plug going in from past the board's end.
SB_W     = 43.2;
SB_L     = 70.6;
GLASS_W  = 43.1;
GLASS_L  = 60;
GLASS_T  = 3.0;
GLASS_E  = 5.6;   // board past the glass at the top end
BEZEL    = [1.7, 1.7, 7.5, 1.7]; // glass that isn't picture: left, right, bottom (keypad end), top
SO_L     = 5.6;   // standoff length off the back of the board
SO_IN    = 2.5;   // standoff centre in from the board's side and end edges
SCR_CLR  = 0.3;
PIN_W    = 22;    // solder relief over the pin row on the board's front, keypad end
PLUG     = 7;     // room past the board's top end for the cable plug and its bend
CABLE    = [18, 4.5]; // lid opening past the board's top end, so the plug's cable comes out the back

// 1510 earpiece (15 x 10 x 3.5) standing on its long edge at the top right, beside the cable plug, firing at the top
// wall. It is taller than the case is deep, so it sits 1 mm into the lid floor and up into the plate. In front of it a
// 1.2 mm air pocket runs along the top wall to a thin slit in the middle of the face, like a modern phone's earpiece
EAR     = [15, 10, 3.5]; // width, height (standing), thickness (toward the top wall)
EAR_AIR = 1.2;
SLIT    = [14, 1.0];

Z_TOP   = SPACER + PLATE;              // 4.16, the face
Z_BOARD = -PCB;                        // back of both boards
Z_GLASS = GLASS_T + 0.1;               // glass pocket roof; the face above it is the lip over the bezel
Z_IN    = Z_BOARD - SO_L;              // lid floor top: the standoff ends land on it
Z_BOT   = Z_IN - FLOOR;
EAR_Z   = Z_IN - 1;                    // earpiece bottom, 1 mm into the lid floor

SCR_X = W / 2 - SB_W / 2;              // screen board left edge
SCR_Y = H;                             // screen board bottom edge, against the keypad board's top edge
GL_Y  = SCR_Y + SB_L - GLASS_E - GLASS_L; // glass bottom edge
L     = H + SB_L + SCR_CLR + PLUG;     // inside length
EAR_X = W + CLR - 0.3 - EAR[0];        // earpiece left edge, against the right wall
EAR_Y = L - EAR_AIR - EAR[2];          // earpiece back face
SOS   = [for (x = [SCR_X + SO_IN, SCR_X + SB_W - SO_IN], y = [SCR_Y + SO_IN, SCR_Y + SB_L - SO_IN]) [x, y]];
SCREW_D = 3.0;   // M2 clearance with room for the standoff positions being off by 0.5 mm
HEAD_D  = 4.4;   // counterbore for the M2 screw head, 1 mm deep in the floor

// XIAO ESP32-C5 (21 x 17.8, USB-C on a short end) flat on the lid floor behind the screen, USB-C out the left wall
XIAO_L = 21;
XIAO_W = 17.8;
XIAO_Y = SCR_Y + 10;                   // its lower edge
XIAO_X = -CLR + 1.25;                  // its USB-C end, in from the wall for a 0.95 mm end wall
USB_Z  = Z_IN + 1.0 + 1.6;             // USB-C center: XIAO PCB on the floor, 3.2 mm receptacle on top

// MAX98357A amp boards (18.7 x 17.7) flat on the lid floor, headers down through floor slots along the 18.7 mm edge
// on the left: the earpiece amp behind the screen beside the XIAO, the speaker amp behind the keypad on the right
AMP_L = 18.7;
AMP_W = 17.7;
AMP_X = W + CLR - LIDCLR - SKIRT - 0.4 - AMP_W;
AMPS  = [[AMP_X, XIAO_Y], [AMP_X, 29]];

// Waveshare 2030 speaker (20 x 30 x 5.5) in a 1 mm floor pocket behind the left of the keypad, grille in the floor
SPK   = [20, 30, 5.5];
SPK_X = 1.6;
SPK_Y = 27;

// INMP441 mic board (12.9 round) flat against the back of the keypad board at the bottom right, port facing the lid.
// It sits on a raised seat; a channel in the seat runs from its port to a hole in the bottom end of the case
MIC_D = 12.9;
MIC_T = 2.0;     // board plus the mic on it
MIC_X = W - 8.0;
MIC_Y = 8.0;
MIC_Z = Z_BOARD - MIC_T - 1.0;         // seat top, 1 mm clear of the keypad board for the parts on the mic board
CH_W  = 2.4;     // sound channel, across the whole seat
CH_H  = 1.0;

// 1027 coin vibration motor (10 round, 2.7 thick) stuck down in a 1 mm floor pocket behind the screen
MOT   = [10, 2.7];
MOT_X = W / 2;
MOT_Y = SCR_Y + 42;

WIRE_X = 24;     // keypad breakout pads, bottom edge

// posts behind four domes (key-face x, y down from the top edge): d-pad center, 4, 6 and 0
POSTS = [[24, 19.5], [9, 44], [39, 44], [24, 62]];
POST_D   = 3;
POST_PRE = 0.1;

$fn = 64;

module shell2d(d = 0) {
	offset(R + d) offset(-R) square([W, L]);
}

module ring(d0, d1) {
	difference() {
		shell2d(d1);
		shell2d(d0);
	}
}

module bead(len) {
	// 0.2 mm root inside the skirt so the bead is one solid with it, not a separate body touching its face
	rotate([90, 0, 0]) linear_extrude(len, center = true) polygon([[0, -0.5], [BEAD, 0], [0, 0.9], [-0.2, 0.9], [-0.2, -0.5]]);
}

module top() {
	difference() {
		union() {
			// face plate over the keypad and the screen
			translate([0, 0, SPACER]) linear_extrude(PLATE) shell2d(CLR + WALL);
			// keypad spacer: rim on the board edge, the rest relieved (as in t9kb_case.scad)
			linear_extrude(SPACER) difference() {
				offset(R + CLR) offset(-R) square([W, H]);
				offset(R - BAND) offset(-R) square([W, H]);
			}
			translate([0, 0, RELIEF]) linear_extrude(SPACER - RELIEF) difference() {
				offset(R + CLR) offset(-R) square([W, H]);
				caps2d(POCKET);
			}
			// solid over the screen from the back of its board up into the plate; the pockets below cut it down to a
			// frame around the board, a seat on its two end strips, and the lip over the glass edges
			translate([0, 0, Z_BOARD]) linear_extrude(SPACER - Z_BOARD + 0.01) intersection() {
				shell2d(CLR + 0.01);
				translate([-5, SCR_Y]) square([W + 10, L]);
			}
			// walls
			translate([0, 0, Z_BOT]) linear_extrude(Z_TOP - Z_BOT) ring(CLR, CLR + WALL);
		}
		// key holes
		translate([0, 0, SPACER - 0.01]) linear_extrude(PLATE + 0.02) caps2d(KEYCLR);
		// screen board pocket, glass pocket, then a window over the picture only
		translate([SCR_X - SCR_CLR, SCR_Y, Z_BOARD - 1]) cube([SB_W + 2 * SCR_CLR, SB_L + SCR_CLR, PCB + 1]);
		// glass pocket, 0.4 mm longer at the keypad end: the measured strips add up to 71.0 on a 70.6 board
		translate([W / 2 - GLASS_W / 2 - 0.2, GL_Y - 0.6, -0.01]) cube([GLASS_W + 0.4, GLASS_L + 0.8, Z_GLASS + 0.01]);
		translate([W / 2 - GLASS_W / 2 + BEZEL[0], GL_Y + BEZEL[2], Z_GLASS - 0.01]) cube([GLASS_W - BEZEL[0] - BEZEL[1], GLASS_L - BEZEL[2] - BEZEL[3], Z_TOP - Z_GLASS + 0.02]);
		// relief over the solder joints of the pin row on the board's front, keypad end strip
		translate([W / 2 - PIN_W / 2, SCR_Y, -0.01]) cube([PIN_W, GL_Y - 0.6 - SCR_Y + 0.01, 1.01]);
		// snap groove for the lid bead
		translate([0, 0, BEAD_Z - 0.7]) linear_extrude(1.6) ring(CLR - 0.01, CLR + BEAD + 0.1);
		// USB-C through the left wall
		translate([-CLR - WALL - 1, XIAO_Y + XIAO_W / 2 - 5, USB_Z - 2.1]) cube([WALL + CLR + 1.5, 10, 4.2]);
		// keypad breakout pad wires
		translate([WIRE_X - 3.5, -CLR - WALL - 1, Z_BOARD - 2.6]) cube([7, CLR + WALL + 1.5, 2.2]);
		// earpiece pocket up into the plate, the air pocket along the top wall to the middle, and the slit in the face
		translate([EAR_X - 0.2, EAR_Y - 0.2, Z_BOARD - 1]) cube([EAR[0] + 0.4 + CLR, EAR[2] + 0.4, EAR_Z + EAR[1] + 0.2 - Z_BOARD + 1]);
		translate([W / 2 - SLIT[0] / 2 - 1, EAR_Y + EAR[2] + 0.2 - 0.01, Z_BOARD - 1]) cube([EAR_X + EAR[0] - (W / 2 - SLIT[0] / 2 - 1) + 0.2 + CLR, L - EAR_Y - EAR[2] - 0.2 + 0.01 + CLR, Z_TOP - 0.8 - Z_BOARD + 1]);
		translate([W / 2 - SLIT[0] / 2, L - EAR_AIR / 2 - SLIT[1] / 2, Z_TOP - 0.81]) cube([SLIT[0], SLIT[1], 0.82]);
		// mic sound hole through the bottom end, lined up with the lid's channel
		translate([MIC_X - CH_W / 2, -CLR - WALL - 1, MIC_Z - CH_H]) cube([CH_W, CLR + WALL + 1.5, CH_H]);
	}
}

module lid() {
	difference() {
		union() {
			translate([0, 0, Z_BOT]) linear_extrude(FLOOR) shell2d(CLR - LIDCLR);
			// skirt: rises to the back of both boards and holds their edges against the top
			translate([0, 0, Z_IN - 0.01]) linear_extrude(Z_BOARD - Z_IN + 0.01) ring(CLR - LIDCLR - SKIRT, CLR - LIDCLR);
			// posts behind four domes
			for (d = POSTS) translate([d[0], H - d[1], Z_IN - 0.01]) cylinder(d = POST_D, h = Z_BOARD - Z_IN + POST_PRE + 0.01, $fn = 24);
			// rib under the keypad board's top edge, open for the Qwiic jack and plug (x 10-22)
			translate([0, H - 1.5, Z_IN - 0.01]) linear_extrude(Z_BOARD - Z_IN + 0.01) difference() {
				square([W, 1.5]);
				translate([10, -1]) square([12, 4]);
			}
			// XIAO cradle: rim on three sides, the USB-C end against the wall
			translate([0, 0, Z_IN - 0.01]) linear_extrude(1.51) difference() {
				translate([-CLR + LIDCLR, XIAO_Y - 1.6]) square([XIAO_X + XIAO_L + 1.9 + CLR - LIDCLR, XIAO_W + 3.2]);
				translate([-CLR - 1, XIAO_Y - 0.3]) square([XIAO_X + XIAO_L + 1.3 + CLR, XIAO_W + 0.6]);
			}
			// amp cradles
			for (a = AMPS) translate([0, 0, Z_IN - 0.01]) linear_extrude(1.21) difference() {
				translate([a[0] - 1.2, a[1] - 1.2]) square([AMP_W + 2.4, AMP_L + 2.4]);
				translate([a[0] - 0.2, a[1] - 0.2]) square([AMP_W + 0.4, AMP_L + 0.4]);
			}
			// speaker rim around its floor pocket
			translate([0, 0, Z_IN - 0.01]) linear_extrude(1.01) difference() {
				translate([SPK_X - 1, SPK_Y - 1.2]) square([SPK[0] + 2.2, SPK[1] + 2.4]);
				translate([SPK_X - 0.2, SPK_Y - 0.2]) square([SPK[0] + 0.4, SPK[1] + 0.4]);
			}
			// mic seat: a solid boss up to the seat, then a ring that keeps the board in place
			translate([MIC_X, MIC_Y, Z_IN - 0.01]) cylinder(d = MIC_D + 2.8, h = MIC_Z - Z_IN + 0.01);
			translate([MIC_X, MIC_Y, MIC_Z - 0.01]) difference() {
				cylinder(d = MIC_D + 2.8, h = MIC_T - 0.3);
				translate([0, 0, -1]) cylinder(d = MIC_D + 0.4, h = MIC_T + 1);
			}
			// snap beads on the long sides, one under the keypad and one above the XIAO
			for (x = [-CLR + LIDCLR, W + CLR - LIDCLR], y = [[H / 2, 36], [SCR_Y + 50, 30]])
				translate([x, y[0], BEAD_Z]) rotate([0, 0, x < W / 2 ? 180 : 0]) bead(y[1]);
		}
		// skirt opening along the XIAO's USB-C end, so it sits against the wall
		translate([-CLR - 1, XIAO_Y - 0.3, Z_IN]) cube([SKIRT + 1.5, XIAO_W + 0.6, 6]);
		// keypad breakout pads and their wires
		translate([WIRE_X - 8, -5, Z_IN]) cube([16, 7, -Z_IN]);
		// vibration motor pocket
		translate([MOT_X, MOT_Y, Z_IN - 1]) cylinder(d = MOT[0] + 0.4, h = 1.01);
		// screws into the screen standoffs: clearance hole and a head counterbore
		for (s = SOS) {
			translate([s[0], s[1], Z_BOT - 1]) cylinder(d = SCREW_D, h = FLOOR + 2, $fn = 24);
			translate([s[0], s[1], Z_BOT - 1]) cylinder(d = HEAD_D, h = 2, $fn = 32);
		}
		// mic sound channel: across the seat under the whole board, through the skirt to the bottom end
		translate([MIC_X - CH_W / 2, -3, MIC_Z - CH_H]) cube([CH_W, MIC_Y + MIC_D / 2 + 0.2 + 3, CH_H + 0.01]);
		// XIAO header slots: 7 pins per side on 2.54 mm, rows 1.28 mm in from the long edges, spacer 2.5 mm wide
		for (y = [XIAO_Y + 1.28, XIAO_Y + XIAO_W - 1.28])
			translate([XIAO_X + 0.8, y - 1.45, Z_BOT - 1]) cube([XIAO_L - 1.6, 2.9, FLOOR + 2]);
		// amp header slots: the 7-pin row along the 18.7 mm edge on the left
		for (a = AMPS) translate([a[0] + 1.27 - 1.45, a[1] + 0.4, Z_BOT - 1]) cube([2.9, AMP_L - 0.8, FLOOR + 2]);
		// speaker pocket 1 mm into the floor and a slotted grille under it
		translate([SPK_X - 0.2, SPK_Y - 0.2, Z_IN - 1]) cube([SPK[0] + 0.4, SPK[1] + 0.4, 1.01]);
		for (i = [0:6]) translate([SPK_X + 3, SPK_Y + 4 + i * 3.5, Z_BOT - 1]) cube([SPK[0] - 6, 1.2, FLOOR + 1]);
		// earpiece foot pocket 1 mm into the floor, and a notch in the skirt where it stands
		translate([EAR_X - 0.2, EAR_Y - 0.2, EAR_Z]) cube([EAR[0] + 0.4 + CLR, EAR[2] + 0.4, 1.01]);
		translate([EAR_X - 0.2, EAR_Y - 0.2, Z_IN - 0.01]) cube([EAR[0] + 1, EAR[2] + 0.4, Z_BOARD - Z_IN + 1]);
		// screen cable: opening past the board's top end so the plug's cable comes out the back
		translate([W / 2 - CABLE[0] / 2, SCR_Y + SB_L + SCR_CLR + 1, Z_BOT - 1]) cube([CABLE[0], CABLE[1], FLOOR + 2]);
	}
	// end wall across the skirt opening, as high as the XIAO board, so it can't slide out the open end
	translate([-CLR + LIDCLR, XIAO_Y - 0.3, Z_IN - 0.01]) cube([XIAO_X - 0.15 - (-CLR + LIDCLR), XIAO_W + 0.6, 1.01]);
}

// stand-ins for the previews: keypad board, screen module, XIAO, amp, mic
module keypad_board() { color("#1d6b3a") translate([0, 0, -PCB]) cube([W, H, PCB]); }
module screen_board() { color("#1a3a8a") translate([SCR_X, SCR_Y, -PCB]) cube([SB_W, SB_L, PCB]); }
module glass() { color("#101010") translate([W / 2 - GLASS_W / 2, GL_Y, 0]) cube([GLASS_W, GLASS_L, GLASS_T]); }
module standoffs() { color("gold") for (s = SOS) translate([s[0], s[1], Z_IN]) cylinder(d = 3.4, h = SO_L, $fn = 6); }
module xiao() {
	color("#303030") translate([XIAO_X, XIAO_Y, Z_IN]) cube([XIAO_L, XIAO_W, 1]);
	color("silver") translate([XIAO_X - 0.5, XIAO_Y + XIAO_W / 2 - 4.5, Z_IN + 1]) cube([7.5, 9, 3.2]);
}
module amp() { color("#7a1fa2") for (a = AMPS) translate([a[0], a[1], Z_IN]) cube([AMP_W, AMP_L, 1.6]); }
module speaker() { color("#404040") translate([SPK_X, SPK_Y, Z_IN - 1]) cube(SPK); }
module mic() { color("#c03030") translate([MIC_X, MIC_Y, MIC_Z]) cylinder(d = MIC_D, h = MIC_T); }
module motor() { color("silver") translate([MOT_X, MOT_Y, Z_IN - 1]) cylinder(d = MOT[0], h = MOT[1]); }
module earpiece() { color("#404040") translate([EAR_X, EAR_Y, EAR_Z]) cube([EAR[0], EAR[2], EAR[1]]); }

module parts() {
	keypad_board();
	screen_board();
	glass();
	standoffs();
	xiao();
	amp();
	mic();
	motor();
	speaker();
	earpiece();
}

if (PART == "top") {
	if (PRINT) translate([0, L, Z_TOP]) rotate([180, 0, 0]) top();
	else color("#d0d0d0") top();
}
if (PART == "lid") {
	if (PRINT) translate([0, 0, -Z_BOT]) lid();
	else color("#9a9a9a") lid();
}
if (PART == "case" || PART == "parts") {
	color("#d0d0d0") top();
	color("#9a9a9a") lid();
}
if (PART == "parts") {
	color("#00AE42") difference() { caps(); legends(); }
	color("#202020") legends();
	parts();
}
// single stand-ins for the preview page exports
if (PART == "keypad_board") keypad_board();
if (PART == "screen_board") screen_board();
if (PART == "glass") glass();
if (PART == "standoffs") standoffs();
if (PART == "xiao") xiao();
if (PART == "amp") amp();
if (PART == "mic") mic();
if (PART == "motor") motor();
if (PART == "speaker") speaker();
if (PART == "earpiece") earpiece();
