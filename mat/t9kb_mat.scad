// T9KB TPU key mat: black body with flush white legends, sized to the T9KB board (48 x 68 mm).
// Coordinates follow the board's key face: x right, y down from the top left corner.
// Export: openscad -D 'PART="body"' / "legends" / "both", PRINT=true flips it face down for printing.

PART  = "both";  // body, legends, both (preview)
PRINT = false;   // true: key tops on the bed, for the print files

W = 48;          // board size
H = 68;
R = 2;           // board corner radius

BEZEL  = 1.6;    // flat part between the keys, rests on the board; the case plate sits on it
RAISE  = 2.0;    // how far the keys stand above the flat part
Z0     = 1.0;    // underside of the keys above the board
WEB    = 0.6;    // flexible web joining each key to the flat part
GROOVE = 0.6;    // gap around each key
LEGEND = 0.4;    // legend depth, 2 layers at 0.2 mm

DOME_H = 0.5;    // dome height on the board incl. adhesive, measure yours
GAP    = 0.1;    // plunger clearance above the dome

KEY_R = 1.5;     // key corner radius
TOP   = BEZEL + RAISE;

DIGIT  = "Liberation Sans:style=Bold";
LETTER = "Liberation Sans Narrow:style=Bold";
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

// d-pad: 18 mm ring, 10.5 mm round center, arrow domes centered in the ring
DX = 24;
DY = 19.5;
DR = 9;
DC = 5.25;
DA = (DC + DR) / 2;

$fn = 64;

function Y(y) = H - y;

module rrect(cx, cy, w, h, r) {
	translate([cx, Y(cy)]) offset(r) square([w - 2 * r, h - 2 * r], center = true);
}

module outline() {
	offset(R) offset(-R) square([W, H]);
}

// every key shape, grown by d
module keys2d(d = 0) {
	for (k = KEYS) offset(d) rrect(k[0], k[1], k[2], k[3], KEY_R);
	translate([DX, Y(DY)]) circle(DR + d);
}

// the separate moving parts: rectangle keys, d-pad center, 4 arrows
module islands2d() {
	for (k = KEYS) rrect(k[0], k[1], k[2], k[3], KEY_R);
	translate([DX, Y(DY)]) {
		circle(DC);
		difference() {
			circle(DR);
			circle(DC + GROOVE);
			for (a = [45, 135]) rotate(a) square([2 * DR + 2, GROOVE], center = true);
		}
	}
}

// dome centers and plunger diameters
DOMES = concat(
	[for (k = KEYS) [k[0], k[1], k[6] == 5 ? 2.0 : 1.5]],
	[[DX, DY, 2.0]],
	[for (a = [0, 90, 180, 270]) [DX + DA * cos(a), DY - DA * sin(a), 1.5]]
);

// --- icons, about 5 mm, strokes 0.45 mm or more for a 0.4 mm nozzle ---

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
		arc2d(1.05, 1.55, -45, 45);
		arc2d(1.95, 2.45, -45, 45);
	}
}

module mic() {
	hull() {
		translate([0, 1.5]) circle(0.75);
		translate([0, 0.3]) circle(0.75);
	}
	translate([0, 0.5]) arc2d(1.2, 1.7, 200, 340);
	translate([-0.25, -1.9]) square([0.5, 0.8]);
	translate([-1.0, -2.3]) square([2.0, 0.5]);
}

module mute() {
	difference() {
		mic();
		rotate(-45) square([1.1, 7], center = true);
	}
	rotate(-45) square([0.5, 6.2], center = true);
}

module handset() {
	offset(0.12) text("\U01F4DE", 5.6, ICONS, halign = "center", valign = "center");
}

module mail() {
	difference() {
		square([4.6, 3.2], center = true);
		square([3.7, 2.3], center = true);
	}
	bar([-2.0, 1.3], [0, -0.2], 0.45);
	bar([2.0, 1.3], [0, -0.2], 0.45);
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
		else translate([-4.3, 0]) text(k[4], k[4] == "*" ? 5 : 3.6, DIGIT, halign = "center", valign = "center");
		if (k[5] == "@mail") translate([2.1, 0]) icon(k[5]);
		else if (k[5] == "+") translate([2.0, 0]) text(k[5], 3.2, DIGIT, halign = "center", valign = "center");
		else if (k[5] != "") translate([2.0, 0]) text(k[5], 2.1, LETTER, halign = "center", valign = "center");
	}
	// arrows; the center select key has no legend
	translate([DX, Y(DY)]) for (a = [0, 90, 180, 270]) rotate(a) translate([DA, 0]) polygon([[1.1, 0], [-0.7, 1.2], [-0.7, -1.2]]);
}

module legends() {
	translate([0, 0, TOP - LEGEND]) linear_extrude(LEGEND) intersection() {
		legends2d();
		islands2d();
	}
}

module body() {
	difference() {
		union() {
			// flat part between the keys
			linear_extrude(BEZEL) difference() {
				outline();
				keys2d(GROOVE);
			}
			// keys
			translate([0, 0, Z0]) linear_extrude(TOP - Z0) islands2d();
			// web across the grooves, level with the key undersides
			translate([0, 0, Z0]) linear_extrude(WEB) difference() {
				keys2d(GROOVE);
				islands2d();
			}
			// plungers
			for (d = DOMES) translate([d[0], Y(d[1]), DOME_H + GAP]) cylinder(d = d[2], h = Z0 - DOME_H - GAP + 0.01);
		}
		legends();
	}
}

module part() {
	if (PART == "body" || PART == "both") color("#303030") body();
	if (PART == "legends" || PART == "both") color("white") translate([0, 0, PART == "both" ? 0.01 : 0]) legends(); // lifted in previews to avoid z-fighting
}

if (PRINT) translate([0, H, TOP]) rotate([180, 0, 0]) part();
else part();
