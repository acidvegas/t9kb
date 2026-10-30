// T9KB keycaps: one rigid PLA cap per key, held in by a flange under the case plate, no web.
// Each cap has a small rigid plunger on its dome, so the dome does all the work and keeps its click.
// Coordinates follow the board's key face: x right, y down from the top left corner, z = 0 at the key face.
// Export: openscad -D 'PART="caps"' / "legends" / "both", PRINT=true flips them face down for printing.
// The caps alone in one color give engraved legends, no filament swaps.

PART  = "both";  // caps, legends, both (preview)
PRINT = false;   // true: key tops on the bed, for the print files

W = 48;          // board size
H = 68;

SPACER = 1.6;    // case plate underside above the board (case MAT)
PLATE  = 2.56;   // case plate thickness: deep holes guide the caps, so an off-center press barely tips them
OUT    = 1.0;    // how far the keys stick out of the plate
TOP    = SPACER + PLATE + OUT;

BASE   = 0.8;    // cap underside above the board; the flange runs from here up to the plate
FLANGE = 0.75;   // flange width around each cap, held under the plate
HOLE   = 0.25;   // plate hole clearance around each cap (case KEYCLR)
GROW   = 0.15;   // caps grown into that clearance: 0.10 mm left around each cap

// plunger tip above the board, resting on the dome. ButtonWorx domes have no published height; 5 mm round domes
// run 0.24 to 0.38 mm and the peel-and-stick film adds about 0.05 mm. 0.40 leaves at most about 0.1 mm of free play
// on a low dome. Keep TOP - TIP a multiple of the 0.12 mm layers after the 0.2 mm first one (4.76 mm here), or the
// slicer rounds the plunger length. Raise it to 0.52 if the keys feel pre-pressed.
TIP    = 0.40;
PLUNGE = [[5, 1.2], [4, 1.0]]; // dome size, plunger diameter (Snaptron: no more than 25% of the dome)

LEGEND = 0.2;    // legend depth: the 0.2 mm first layer
FAT    = 0.1;    // legend outline growth, keeps every stroke at 0.45 mm or more
KEY_R  = 1.5;    // key corner radius

DIGIT  = "Liberation Sans:style=Bold";
LETTER = "Liberation Sans:style=Regular";
NARROW = "Liberation Sans Narrow:style=Bold";
ICONS  = "FreeSerif";

// x, y, w, h, legend, sublegend, dome size. Legends starting with @ are icons.
KEYS = [
	[9,     5,    13,   5, "@soft",    "",      4],
	[24,    5,    13,   5, "@soft",    "",      4],
	[39,    5,    13,   5, "@soft",    "",      4],
	[7.75,  14.5, 10.5, 7, "@speaker", "",      5],
	[40.25, 14.5, 10.5, 7, "@mute",    "",      5],
	[7.75,  24.5, 10.5, 7, "@call",    "",      5],
	[40.25, 24.5, 10.5, 7, "@end",     "",      5],
	[9,  35, 13, 7, "1", "@mail", 5], [24, 35, 13, 7, "2", "ABC", 5], [39, 35, 13, 7, "3", "DEF",  5],
	[9,  44, 13, 7, "4", "GHI",   5], [24, 44, 13, 7, "5", "JKL", 5], [39, 44, 13, 7, "6", "MNO",  5],
	[9,  53, 13, 7, "7", "PQRS",  5], [24, 53, 13, 7, "8", "TUV", 5], [39, 53, 13, 7, "9", "WXYZ", 5],
	[9,  62, 13, 7, "*", "",      5], [24, 62, 13, 7, "0", "+",   5], [39, 62, 13, 7, "#", "",     5],
];

// d-pad: 18 mm overall. The plate keeps a 1.4 mm ring between the center key and the arrows, held by
// 1.4 mm spokes on the diagonals, so every d-pad piece has its own stop and pressing one never moves another.
DX    = 24;
DY    = 19.5;
DR    = 9;                       // arrows outer radius
DC    = 4;                       // center key radius
RIB   = 1.4;                     // plate ring and spoke width
DA0   = DC + HOLE + RIB + HOLE;  // arrows inner radius
DA    = 7.125;                   // arrow dome radius on the board

$fn = 64;

function Y(y) = H - y;

module rrect(cx, cy, w, h, r) {
	translate([cx, Y(cy)]) offset(r) square([w - 2 * r, h - 2 * r], center = true);
}

// one arrow, a ring quarter pointing at angle a (0 right, 90 up)
module arrow2d(a) {
	translate([DX, Y(DY)]) intersection() {
		difference() {
			circle(DR);
			circle(DA0);
			for (s = [45, 135]) rotate(s) square([2 * DR + 2, RIB + 2 * HOLE], center = true);
		}
		rotate(a) polygon([[0, 0], [2 * DR, -2 * DR], [2 * DR, 2 * DR]]);
	}
}

module center2d() {
	translate([DX, Y(DY)]) circle(DC);
}

// every cap outline grown by d (case uses this for the plate holes and flange pockets)
module caps2d(d = 0) {
	for (k = KEYS) offset(d) rrect(k[0], k[1], k[2], k[3], KEY_R);
	offset(d) center2d();
	for (a = [0, 90, 180, 270]) offset(d) arrow2d(a);
}

function plunger(size) = [for (p = PLUNGE) if (p[0] == size) p[1]][0];

// one cap: body, flange and plunger. dome = [x, y, size]
module cap(dome) {
	translate([0, 0, BASE]) linear_extrude(TOP - BASE) offset(GROW) children();
	translate([0, 0, BASE]) linear_extrude(SPACER - BASE) offset(FLANGE) children();
	translate([dome[0], Y(dome[1]), TIP]) cylinder(d = plunger(dome[2]), h = BASE - TIP + 0.01, $fn = 32);
}

module caps() {
	for (k = KEYS) cap([k[0], k[1], k[6]]) rrect(k[0], k[1], k[2], k[3], KEY_R);
	cap([DX, DY, 5]) center2d();
	for (a = [0, 90, 180, 270]) cap([DX + DA * cos(a), DY - DA * sin(a), 4]) arrow2d(a);
}

// --- icons, about 5 mm, strokes 0.5 mm or more ---

module arc2d(r1, r2, a0, a1) {
	polygon(concat([for (a = [a0:5:a1]) r2 * [cos(a), sin(a)]], [for (a = [a1:-5:a0]) r1 * [cos(a), sin(a)]]));
}

module bar(p1, p2, w = 0.5) {
	hull() {
		translate(p1) circle(d = w, $fn = 16);
		translate(p2) circle(d = w, $fn = 16);
	}
}

module speaker() {
	translate([-0.9, 0]) {
		translate([-1.6, -0.75]) square([0.9, 1.5]);
		polygon([[-0.7, -0.75], [0.6, -1.9], [0.6, 1.9], [-0.7, 0.75]]);
		arc2d(1.15, 1.65, -45, 45);
		arc2d(2.15, 2.65, -45, 45);
	}
}

module mic() {
	hull() {
		translate([0, 1.5]) circle(0.75);
		translate([0, 0.3]) circle(0.75);
	}
	translate([0, 0.5]) arc2d(1.2, 1.7, 215, 325);
	translate([-0.25, -1.9]) square([0.5, 0.8]);
	translate([-1.0, -2.3]) square([2.0, 0.5]);
}

module mute() {
	difference() {
		mic();
		rotate(-45) square([1.5, 7], center = true);
	}
	rotate(-45) square([0.5, 6.2], center = true);
}

module handset() {
	offset(0.12) text("\U01F4DE", 5.6, ICONS, halign = "center", valign = "center");
}

module mail() {
	difference() {
		square([4.6, 3.2], center = true);
		square([3.6, 2.2], center = true);
	}
	bar([-1.9, 1.2], [0, -0.2], 0.5);
	bar([1.9, 1.2], [0, -0.2], 0.5);
}

module icon(name) {
	if (name == "@soft") square([5, 0.6], center = true);
	else if (name == "@speaker") speaker();
	else if (name == "@mute") mute();
	else if (name == "@call") handset();
	else if (name == "@end") rotate(-110) handset();
	else if (name == "@mail") mail();
}

module legends2d() {
	for (k = KEYS) translate([k[0], Y(k[1])]) {
		if (k[4][0] == "@") icon(k[4]);
		else translate([-4.6, 0]) offset(FAT) text(k[4], k[4] == "*" ? 5 : 3.6, DIGIT, halign = "center", valign = "center");
		if (k[5] == "@mail") translate([2.1, 0]) icon(k[5]);
		else if (k[5] == "+") translate([2.0, 0]) offset(FAT) text(k[5], 3.4, DIGIT, halign = "center", valign = "center");
		else if (len(k[5]) == 4) translate([1.8, 0]) offset(FAT) text(k[5], 2.2, NARROW, halign = "center", valign = "center", spacing = 1.15);
		else if (k[5] != "") translate([1.8, 0]) offset(FAT) text(k[5], 2.3, LETTER, halign = "center", valign = "center", spacing = 1.15);
	}
	// arrows; the center select key has no legend
	translate([DX, Y(DY)]) for (a = [0, 90, 180, 270]) rotate(a) translate([DA, 0]) polygon([[1.1, 0], [-0.7, 1.2], [-0.7, -1.2]]);
}

module legends() {
	translate([0, 0, TOP - LEGEND]) linear_extrude(LEGEND) intersection() {
		legends2d();
		caps2d(-0.3);
	}
}

module part() {
	if (PART == "caps" || PART == "both") color("#00AE42") difference() {
		caps();
		legends();
	}
	if (PART == "legends" || PART == "both") color("#202020") translate([0, 0, PART == "both" ? 0.01 : 0]) legends(); // lifted in previews to avoid z-fighting
}

if (PRINT) translate([0, H, TOP]) rotate([180, 0, 0]) part();
else part();
