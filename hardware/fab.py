#!/usr/bin/env python3
# JLCPCB outputs: gerbers+drill zip, BOM.csv, CPL.csv.  usage: fab.py
import csv
import os
import shutil
import subprocess
import pcbnew

LCSC = {
	'TCA8418RTWR': 'C138713',
	'100nF':       'C14663',
	'10uF':        'C19702',
	'4.7k':        'C23162',
	'10k':         'C25804',
	'Qwiic':       'C160404',  # JST SM04B-SRSS-TB(LF)(SN)
}

here = os.path.dirname(os.path.abspath(__file__))
pcb  = os.path.join(here, 't9kb.kicad_pcb')
out  = os.path.join(here, 'fab')
shutil.rmtree(out, ignore_errors=True)
os.makedirs(out + '/gerber')

subprocess.run(['kicad-cli', 'pcb', 'export', 'gerbers', '--no-protel-ext', '--layers', 'F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts', '-o', out + '/gerber/', pcb], check=True, capture_output=True)
subprocess.run(['kicad-cli', 'pcb', 'export', 'drill', '--excellon-separate-th', '-o', out + '/gerber/', pcb], check=True, capture_output=True)
shutil.make_archive(os.path.join(out, 't9kb_gerbers'), 'zip', out + '/gerber')
shutil.rmtree(out + '/gerber')

board = pcbnew.LoadBoard(pcb)
bom, cpl = {}, []
for fp in board.GetFootprints():
	ref, val = fp.GetReference(), fp.GetValue()
	if val not in LCSC:
		continue
	bom.setdefault((val, str(fp.GetFPID().GetLibItemName()), LCSC[val]), []).append(ref)
	pos = fp.GetPosition()
	cpl.append([ref, f'{pcbnew.ToMM(pos.x):.3f}mm', f'{-pcbnew.ToMM(pos.y):.3f}mm', # same origin as the gerbers
		'Bottom' if fp.IsFlipped() else 'Top', f'{fp.GetOrientationDegrees() % 360:.0f}'])

with open(os.path.join(out, 'BOM.csv'), 'w', newline='') as f:
	w = csv.writer(f)
	w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC'])
	for (val, fpn, lcsc), refs in sorted(bom.items()):
		w.writerow([val, ','.join(sorted(refs, key=lambda r: (r[0], int(''.join(c for c in r if c.isdigit()) or 0)))), fpn, lcsc])
with open(os.path.join(out, 'CPL.csv'), 'w', newline='') as f:
	w = csv.writer(f)
	w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
	w.writerows(sorted(cpl))
print('fab ->', out, f'{len(cpl)} placed parts')
