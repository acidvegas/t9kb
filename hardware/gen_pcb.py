#!/usr/bin/env python3
# T9 I2C keypad v2 - board generator (KiCad 9 pcbnew API)
# 24 metal-dome keys, TCA8418 scanner.
# Key face (dome pads + legends) is the KiCad bottom layer, every part is on the KiCad top layer, so JLC assembles the top side.
# Layout coordinates below are as seen looking at the key face; kx() mirrors them into KiCad's top-view coordinates.
# Builds t9kb.kicad_pcb with parts placed and nets assigned; routing is done by freerouting afterwards.

import math
import os
import pcbnew

HERE   = os.path.dirname(os.path.abspath(__file__))
LIB    = os.path.join(HERE, 't9.pretty')
KFP    = '/usr/share/kicad/footprints'
OUT    = os.path.join(HERE, 't9kb.kicad_pcb')
OX, OY = 100.0, 100.0 # sheet offset
W, H   = 48.0, 68.0   # board size

# name, x, y, matrix row, matrix col, key w, key h, silk label, silk sublabel, dome size
# 43mm wide key area centered on x=24: soft keys 13x5, d-pad 18mm circle with a 10.5mm round center, side keys 10.5x7, number keys 13x7, 2mm gaps
KEYS = [
	('SOFT_L',  9,     5.0,  0, 3, 13,   5, '', '', 4),
	('SOFT_M',  24,    5.0,  0, 4, 13,   5, '', '', 4),
	('SOFT_R',  39,    5.0,  0, 5, 13,   5, '', '', 4),
	('SPK',     7.75,  14.5, 1, 3, 10.5, 7, '', '', 5),
	('UP',      24,    12.375, 1, 4, 0,  0, '', '', 4),
	('CONTACT', 40.25, 14.5, 1, 5, 10.5, 7, '', '', 5),
	('LEFT',    16.875, 19.5, 2, 3, 0,   0, '', '', 4),
	('OK',      24,    19.5, 2, 4, 0,    0, '', '', 5),
	('RIGHT',   31.125, 19.5, 2, 5, 0,   0, '', '', 4),
	('CALL',    7.75,  24.5, 3, 3, 10.5, 7, '', '', 5),
	('DOWN',    24,    26.625, 3, 4, 0,  0, '', '', 4),
	('END',     40.25, 24.5, 3, 5, 10.5, 7, '', '', 5),
]
KP = [('1', ''), ('2', 'ABC'), ('3', 'DEF'), ('4', 'GHI'), ('5', 'JKL'), ('6', 'MNO'),
      ('7', 'PQRS'), ('8', 'TUV'), ('9', 'WXYZ'), ('*', ''), ('0', '+'), ('#', '')]
for i, (lab, sub) in enumerate(KP):
	r, c = divmod(i, 3)
	KEYS.append(('K' + {'*': 'STAR', '#': 'HASH'}.get(lab, lab), 9 + 15 * c, 35 + 9 * r, r, c, 13, 7, lab, sub, 5))

ROWS = ['ROW0', 'ROW1', 'ROW2', 'ROW3']
COLS = ['COL0', 'COL1', 'COL2', 'COL3', 'COL4', 'COL5']

# TCA8418RTWR (WQFN-24) pin -> net. ROW4-7 / COL6-9 unused (internal pull-ups).
TCA = {
	'8': 'ROW0', '7': 'ROW1', '6': 'ROW2', '5': 'ROW3',
	'9': 'COL0', '10': 'COL1', '11': 'COL2', '12': 'COL3', '13': 'COL4', '14': 'COL5',
	'19': 'GND', '20': 'RST', '21': '+3V3', '22': 'SDA', '23': 'SCL', '24': 'INT', '25': 'GND',
}



def kx(x):
	return W - x


def mm(x, y):
	return pcbnew.VECTOR2I(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + y))


board = pcbnew.BOARD()
ds = board.GetDesignSettings()
ds.SetCopperLayerCount(2)
ds.SetBoardThickness(pcbnew.FromMM(1.6))
ds.m_TrackMinWidth   = pcbnew.FromMM(0.15)
ds.m_MinClearance    = pcbnew.FromMM(0.15)
ds.m_ViasMinSize     = pcbnew.FromMM(0.55)
ds.m_MinThroughDrill = pcbnew.FromMM(0.3)
ds.m_HoleToHoleMin   = pcbnew.FromMM(0.25)
ds.m_CopperEdgeClearance = pcbnew.FromMM(0.3)
nc = ds.m_NetSettings.GetDefaultNetclass()
nc.SetClearance(pcbnew.FromMM(0.15))
nc.SetTrackWidth(pcbnew.FromMM(0.15))
nc.SetViaDiameter(pcbnew.FromMM(0.6))
nc.SetViaDrill(pcbnew.FromMM(0.3))

nets = {}
def net(name):
	if name not in nets:
		n = pcbnew.NETINFO_ITEM(board, name)
		board.Add(n)
		nets[name] = n
	return nets[name]


def add_fp(lib, name, ref, value, x, y, rot=0, back=False, padnets=None):
	fp = pcbnew.FootprintLoad(lib, name)
	fp.SetReference(ref)
	fp.SetValue(value)
	fp.SetPosition(mm(x, y))
	fp.SetOrientationDegrees(rot)
	board.Add(fp)
	if back:
		fp.Flip(mm(x, y), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
	for p in fp.Pads():
		if padnets and p.GetNumber() in padnets:
			p.SetNet(net(padnets[p.GetNumber()]))
	return fp


def line(layer, x1, y1, x2, y2, w=0.15):
	s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT)
	s.SetStart(mm(x1, y1))
	s.SetEnd(mm(x2, y2))
	s.SetLayer(layer)
	s.SetWidth(pcbnew.FromMM(w))
	board.Add(s)


def arc(layer, cx, cy, r, a0, a1, w=0.15):
	s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_ARC)
	s.SetCenter(mm(cx, cy))
	s.SetStart(mm(cx + r * math.cos(math.radians(a0)), cy + r * math.sin(math.radians(a0))))
	s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(a1 - a0, pcbnew.DEGREES_T), True)
	s.SetLayer(layer)
	s.SetWidth(pcbnew.FromMM(w))
	board.Add(s)


def circle(layer, cx, cy, r, w=0.15):
	s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_CIRCLE)
	s.SetCenter(mm(cx, cy))
	s.SetEnd(mm(cx + r, cy))
	s.SetLayer(layer)
	s.SetWidth(pcbnew.FromMM(w))
	board.Add(s)


def rrect(layer, cx, cy, w, h, r, lw=0.15):
	x0, x1, y0, y1 = cx - w / 2, cx + w / 2, cy - h / 2, cy + h / 2
	line(layer, x0 + r, y0, x1 - r, y0, lw)
	line(layer, x0 + r, y1, x1 - r, y1, lw)
	line(layer, x0, y0 + r, x0, y1 - r, lw)
	line(layer, x1, y0 + r, x1, y1 - r, lw)
	arc(layer, x0 + r, y0 + r, r, 180, 270, lw)
	arc(layer, x1 - r, y0 + r, r, 270, 360, lw)
	arc(layer, x1 - r, y1 - r, r, 0, 90, lw)
	arc(layer, x0 + r, y1 - r, r, 90, 180, lw)


def text(layer, s, x, y, size=1.0, thick=0.15, mirror=False, angle=0, width=None):
	t = pcbnew.PCB_TEXT(board)
	t.SetText(s)
	t.SetTextAngleDegrees(angle)
	t.SetPosition(mm(x, y))
	t.SetLayer(layer)
	t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(width or size), pcbnew.FromMM(size)))
	t.SetTextThickness(pcbnew.FromMM(thick))
	t.SetMirrored(mirror)
	board.Add(t)


# --- per-side collision model for auto-placement: circles (domes, vias, holes) + courtyard boxes ---
obst = {False: [], True: []}

def fp_box(fp):
	bb = fp.GetCourtyard(pcbnew.B_CrtYd if fp.IsFlipped() else pcbnew.F_CrtYd).BBox()
	return (pcbnew.ToMM(bb.GetLeft()) - OX, pcbnew.ToMM(bb.GetTop()) - OY, pcbnew.ToMM(bb.GetRight()) - OX, pcbnew.ToMM(bb.GetBottom()) - OY)


def hits(box, back):
	x0, y0, x1, y1 = box
	if x0 < 0.4 or y0 < 0.4 or x1 > W - 0.4 or y1 > H - 0.4:
		return True
	for o in obst[back]:
		if o[0] == 'c':
			_, cx, cy, r = o
			nx, ny = min(max(cx, x0), x1), min(max(cy, y0), y1)
			if (nx - cx) ** 2 + (ny - cy) ** 2 < r * r:
				return True
		elif not (x1 <= o[1] or x0 >= o[3] or y1 <= o[2] or y0 >= o[4]):
			return True
	return False


def fixed(fp):
	obst[fp.IsFlipped()].append(('r',) + fp_box(fp))
	return fp


def auto(lib, name, ref, value, ax, ay, padnets, rots=(0, 90), rmax=9.0, back=False):
	fp = add_fp(lib, name, ref, value, ax, ay, back=back, padnets=padnets)
	r = 0.0
	while r <= rmax:
		for a in ([0] if r == 0 else range(0, 360, 15)):
			x, y = ax + r * math.cos(math.radians(a)), ay + r * math.sin(math.radians(a))
			for rot in rots:
				fp.SetPosition(mm(x, y))
				fp.SetOrientationDegrees(rot)
				box = fp_box(fp)
				if not hits(box, back):
					obst[back].append(('r',) + box)
					return fp
		r += 0.25
	raise SystemExit(f'no room for {ref} near ({ax}, {ay})')


# Board outline
rrect(pcbnew.Edge_Cuts, W / 2, H / 2, W, H, 2.0, 0.1)

# Dome switches on the key face: frame = row, center (via-in-pad) = column
for i, (name, x, y, r, c, kw, kh, lab, sub, dome) in enumerate(KEYS):
	fp = add_fp(LIB, f'MetalDome_{dome}mm', f'SW{i + 1}', name, kx(x), y, back=True, padnets={'1': COLS[c], '2': ROWS[r]})
	for p in fp.Pads():
		if p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH:
			v = p.GetPosition()
			obst[False].append(('c', pcbnew.ToMM(v.x) - OX, pcbnew.ToMM(v.y) - OY, pcbnew.ToMM(p.GetSize().x) / 2 + 0.15))
	obst[True].append(('c', kx(x), y, dome * 0.8))
	# no tracks or vias inside the bare (unmasked) contact area
	ko = pcbnew.ZONE(board)
	ko.SetIsRuleArea(True)
	ko.SetDoNotAllowTracks(True)
	ko.SetDoNotAllowVias(True)
	ko.SetDoNotAllowPads(False)
	ko.SetDoNotAllowCopperPour(True)
	ko.SetLayer(pcbnew.B_Cu)
	ol = ko.Outline()
	ol.NewOutline()
	for a in range(0, 360, 15):
		ol.Append(mm(kx(x) + dome * 0.62 * math.cos(math.radians(a)), y + dome * 0.62 * math.sin(math.radians(a))))
	board.Add(ko)
	if kw:
		rrect(pcbnew.B_SilkS, kx(x), y, kw, kh, 1.5)
	if lab:
		text(pcbnew.B_SilkS, lab, kx(x - 4.6), y, 2.0, 0.3, mirror=True)
	if sub:
		text(pcbnew.B_SilkS, sub, kx(x + 4.55), y, 1.0, 0.15, mirror=True, width=0.8) # narrow so PQRS/WXYZ fit the 13mm key

# d-pad: 18mm circle and 10.5mm center button, broken where they cross the arrow dome pads
for rad, gap in ((9.0, 12), (5.25, 16)):
	for a in range(0, 360, 90):
		arc(pcbnew.B_SilkS, kx(24), 19.5, rad, a + gap, a + 90 - gap)

# Breakout / test pads on the parts side, bottom edge
fixed(add_fp(LIB, 'Pads_1x06_P2.54mm_SMD', 'J2', 'Breakout', 24, H - 1.3,
	padnets={'1': 'GND', '2': '+3V3', '3': 'SDA', '4': 'SCL', '5': 'INT', '6': 'RST'}))
for i, lab in enumerate(['GND', '3V3', 'SDA', 'SCL', 'INT', 'RST']):
	text(pcbnew.F_SilkS, lab, 24 - 6.35 + i * 2.54, H - 4.0, 1.0, 0.15, angle=90)

# --- electronics (all on the parts side) ---
# Qwiic behind the left soft key (the middle soft key's center via sits where it would go at top center)
fixed(add_fp(f'{KFP}/Connector_JST.pretty', 'JST_SH_SM04B-SRSS-TB_1x04-1MP_P1.00mm_Horizontal', 'J1', 'Qwiic', kx(16), 3.1, rot=180,
	padnets={'1': 'GND', '2': '+3V3', '3': 'SDA', '4': 'SCL'}))
fixed(add_fp(LIB, 'WQFN-24-1EP_4x4mm_P0.5mm_EP2.45mm_TI_RTW0024B', 'U1', 'TCA8418RTWR', kx(31.5), 48.5, padnets=TCA))
text(pcbnew.F_SilkS, 'QWIIC', kx(16), 6.6, 1.0, 0.15)
obst[False].append(('r', kx(16) - 2.2, 5.9, kx(16) + 2.2, 7.3))
for p in board.FindFootprintByReference('U1').Pads(): # unnumbered paste segments of the EP
	if p.GetNumber() == '':
		p.SetNet(net('GND'))

# TCA passives beside the unused pins (ROW4-7 / COL6-9) so the I2C, row and column pins can escape
auto(f'{KFP}/Capacitor_SMD.pretty', 'C_0603_1608Metric', 'C1', '100nF', kx(35.6), 45.8, {'1': '+3V3', '2': 'GND'}, rots=(90,))
auto(f'{KFP}/Capacitor_SMD.pretty', 'C_0603_1608Metric', 'C2', '10uF', kx(35.6), 42.9, {'1': '+3V3', '2': 'GND'}, rots=(90,))
auto(f'{KFP}/Resistor_SMD.pretty', 'R_0603_1608Metric', 'R3', '10k', kx(29.0), 44.0, {'1': '+3V3', '2': 'RST'}, rots=(90,))
auto(f'{KFP}/Resistor_SMD.pretty', 'R_0603_1608Metric', 'R4', '10k', kx(35.6), 40.4, {'1': '+3V3', '2': 'INT'}, rots=(90,))

# I2C pull-ups through 3-pad jumper (center = 3V3, bridged by default; cut both bars to disable)
auto(f'{KFP}/Jumper.pretty', 'SolderJumper-3_P1.3mm_Bridged2Bar123_RoundedPad1.0x1.5mm', 'JP1', 'I2C_PU', kx(16), 8.2, {'1': 'PU_SDA', '2': '+3V3', '3': 'PU_SCL'}, rots=(0,))
auto(f'{KFP}/Resistor_SMD.pretty', 'R_0603_1608Metric', 'R1', '4.7k', kx(12.5), 8.2, {'1': 'SDA', '2': 'PU_SDA'})
auto(f'{KFP}/Resistor_SMD.pretty', 'R_0603_1608Metric', 'R2', '4.7k', kx(19.5), 8.2, {'1': 'SCL', '2': 'PU_SCL'})


text(pcbnew.F_SilkS, 'T9KB v0.2  acidvegas', 24, 29.0, 1.0, 0.15)
text(pcbnew.F_SilkS, '3.3V ONLY  TCA8418 @ 0x34', 24, 30.8, 1.0, 0.15)

for fp in board.GetFootprints():
	for g in fp.GraphicalItems(): # library silk lines are 0.12mm, JLC minimum is 0.15mm
		if g.GetLayer() in (pcbnew.F_SilkS, pcbnew.B_SilkS) and g.GetClass() == 'PCB_SHAPE' and g.GetWidth() < pcbnew.FromMM(0.15):
			g.SetWidth(pcbnew.FromMM(0.15))
	if fp.GetReference() != 'U1':
		fp.Reference().SetVisible(False)
	else:
		fp.Reference().SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(1.0), pcbnew.FromMM(1.0)))
		fp.Reference().SetTextThickness(pcbnew.FromMM(0.15))

board.Save(OUT)
print('saved', OUT, len(board.GetFootprints()), 'footprints', len(nets), 'nets')
