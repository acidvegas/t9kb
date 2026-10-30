// T9KB preview scenes for the README and the preview page
use <../case/t9kb_case.scad>
use <../keys/t9kb_keys.scad>

VIEW = "keys";   // keys, under, case, exploded, section

module board() { color("#1d6b3a") translate([0, 0, -1.6]) linear_extrude(1.6) offset(2) offset(-2) square([48, 68]); }
module domes() { for (d = [[9,5,4],[24,5,4],[39,5,4],[7.75,14.5,5],[40.25,14.5,5],[7.75,24.5,5],[40.25,24.5,5],[24,19.5,5],[31.125,19.5,4],[16.875,19.5,4],[24,12.375,4],[24,26.625,4],
	[9,35,5],[24,35,5],[39,35,5],[9,44,5],[24,44,5],[39,44,5],[9,53,5],[24,53,5],[39,53,5],[9,62,5],[24,62,5],[39,62,5]])
	color("silver") translate([d[0], 68 - d[1], 0]) scale([1, 1, 0.4 / (d[2] / 2)]) sphere(d[2] / 2, $fn = 32); }

if (VIEW == "keys") part();
if (VIEW == "under") rotate([180, 0, 0]) part();
if (VIEW == "case") { top(); part(); }
if (VIEW == "exploded") { color("#d0d0d0") translate([0, 0, 22]) top(); translate([0, 0, 7]) part(); board(); domes(); color("#9a9a9a") translate([0, 0, -10]) lid(); }
if (VIEW == "section") difference() {
	union() { color("#d0d0d0") top(); color("#9a9a9a") lid(); part(); board(); domes(); }
	translate([-10, 68 - 44, -20]) cube([70, 40, 40]);
}
