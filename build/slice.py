#!/usr/bin/env python3
# T9KB print files: flattens OrcaSlicer's Bambu A1 mini system profiles and slices every part into a .gcode.3mf
# Usage: python3 build/slice.py [job ...] (after build/export.sh)

import json
import os
import subprocess
import sys

ORCA = os.path.expanduser('~/dev/tools/orca/squashfs-root')
BBL  = os.path.join(ORCA, 'resources/profiles/BBL')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT  = os.path.join(ROOT, 'build', 'out')
TMP  = os.path.join(ROOT, 'build', 'profiles')

MACHINE  = 'Bambu Lab A1 mini 0.4 nozzle'
PLA      = 'Bambu PLA Basic @BBL A1M'
FINE     = '0.12mm Fine @BBL A1M'
STANDARD = '0.20mm Standard @BBL A1M'

# manual swap: park over the purge chute, pause for the swap on the screen, purge 100 mm of the new color, wipe
MANUAL_CHANGE = ''';===== MANUAL FILAMENT SWAP (A1 mini) =====
M400
G1 Z{max_layer_z + 3.0} F1200
M106 P1 S0
M106 P2 S0
G1 X-13.5 F18000
M400
G92 E0
G1 E-2 F1800
M400
M104 S{nozzle_temperature[next_extruder]}
; >>> SWAP NOW: on the screen Unload, then Load the next color, then Resume <<<
M400 U1
M109 S{nozzle_temperature[next_extruder]}
G1 X-13.5 F3000
M106 P1 S60
G92 E0
G1 E100 F{filament_max_volumetric_speed[next_extruder]/2.4053*60*0.8}
M400
G92 E0
G1 E-[new_retract_length_toolchange] F1800
M400
M106 P1 S178
M400 S3
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
M400
G1 Z{max_layer_z + 3.0} F3000
M106 P1 S0
{if layer_z <= (initial_layer_print_height + 0.001)}
M204 S[initial_layer_acceleration]
{else}
M204 S[default_acceleration]
{endif}
;===== MANUAL FILAMENT SWAP END =====
'''


def index():
	'''Map every profile name to its json path'''

	names = {}
	for kind in ('machine', 'process', 'filament'):
		for root, _, files in os.walk(os.path.join(BBL, kind)):
			for f in files:
				if f.endswith('.json'):
					path = os.path.join(root, f)
					data = json.load(open(path))
					if 'name' in data:
						names[data['name']] = path
	return names


def flatten(names, name):
	'''Resolve a profile's inherits chain into one dict'''

	data   = json.load(open(names[name]))
	parent = data.get('inherits')
	merged = flatten(names, parent) if parent else {}
	merged.update(data)
	merged.pop('inherits', None)
	return merged


def write(name, data):
	path = os.path.join(TMP, name + '.json')
	json.dump(data, open(path, 'w'), indent=1)
	return path


def slice_job(job, machine, process, filaments):
	'''Slice one plate described by an assemble list and export it as a .gcode.3mf'''

	os.makedirs(OUT, exist_ok=True)
	assemble = write(job + '_assemble', {'plates': [{'plate_name': job, 'need_arrange': False, 'objects': job_objects[job]}]})
	cmd = ['xvfb-run', '-a', os.path.join(ORCA, 'AppRun'),
		'--load-settings', machine + ';' + process,
		'--load-filaments', ';'.join(filaments),
		'--load-assemble-list', assemble,
		'--slice', '0', '--outputdir', OUT, '--export-3mf', job + '.gcode.3mf', '--debug', '2']
	print('slicing', job)
	res = subprocess.run(cmd, capture_output=True, text=True, timeout=600, cwd=OUT, env=dict(os.environ, MESA_GL_VERSION_OVERRIDE='4.5', MESA_GLSL_VERSION_OVERRIDE='450'))
	log = res.stdout + res.stderr
	open(os.path.join(OUT, job + '.log'), 'w').write(log)
	if res.returncode != 0 or not os.path.exists(os.path.join(OUT, job + '.gcode.3mf')):
		print(log[-3000:])
		sys.exit('slice failed: ' + job)


def stl(rel):
	return os.path.join(ROOT, rel)


# bed center of the A1 mini is 90, 90; parts are placed so their footprint lands around it
job_objects = {
	# legends first: black prints the first layer's legends, one swap, green does the rest
	'keys_2color_MANUAL': [
		{'path': stl('keys/t9kb_keys_legends.stl'), 'count': 1, 'filaments': [1], 'assemble_index': [1], 'pos_x': [66], 'pos_y': [56], 'pos_z': [0], 'object_name': 'legends'},
		{'path': stl('keys/t9kb_keys_caps.stl'),    'count': 1, 'filaments': [2], 'assemble_index': [1], 'pos_x': [66], 'pos_y': [56], 'pos_z': [0], 'object_name': 'keys'},
	],
	'case_top': [
		{'path': stl('case/t9kb_case_top.stl'), 'count': 1, 'filaments': [1], 'pos_x': [64], 'pos_y': [56], 'pos_z': [0], 'object_name': 'case_top'},
	],
	# candybar phone: keypad + 1.8in screen, XIAO and buzzer inside
	'phone': [
		{'path': stl('case/t9kb_phone_top.stl'), 'count': 1, 'filaments': [1], 'pos_x': [25], 'pos_y': [22], 'pos_z': [0], 'object_name': 'phone_top'},
		{'path': stl('case/t9kb_phone_lid.stl'), 'count': 1, 'filaments': [1], 'pos_x': [95], 'pos_y': [24], 'pos_z': [0], 'object_name': 'phone_lid'},
	],
	'case': [
		{'path': stl('case/t9kb_case_top.stl'), 'count': 1, 'filaments': [1], 'pos_x': [30], 'pos_y': [56], 'pos_z': [0], 'object_name': 'case_top'},
		{'path': stl('case/t9kb_case_lid.stl'), 'count': 1, 'filaments': [1], 'pos_x': [100], 'pos_y': [56], 'pos_z': [0], 'object_name': 'case_lid'},
	],
}


def main():
	os.makedirs(TMP, exist_ok=True)
	names = index()

	machine = flatten(names, MACHINE)
	pla     = flatten(names, PLA)
	fine    = flatten(names, FINE)
	std     = flatten(names, STANDARD)

	black = dict(pla, name='PLA black', filament_colour=['#000000'])
	green = dict(pla, name='PLA green', filament_colour=['#00AE42'])

	# keycaps: 0.12 mm layers after a 0.2 mm first layer, so the 0.2 mm legends are exactly the first layer.
	# Solid caps print with no supports; the 0.75 mm flange overhang is short enough to print bare.
	# The prime tower stays on: without it OrcaSlicer drops the swap pause.
	keys = dict(fine, name='T9KB keys', curr_bed_type='Textured PEI Plate',
		initial_layer_print_height='0.2', wall_loops='2', sparse_infill_density='100%', sparse_infill_pattern='zig-zag',
		top_shell_layers='5', bottom_shell_layers='5', enable_support='0', brim_type='no_brim',
		enable_prime_tower='1', wipe_tower_x=['130'], wipe_tower_y=['70'], ironing_type='no ironing', elefant_foot_compensation='0.1',
		wall_generator='arachne', min_feature_size='25%', min_bead_width='85%')

	# case: plate down, no supports
	case = dict(std, name='T9KB case', curr_bed_type='Textured PEI Plate', wall_loops='3', enable_support='0', brim_type='no_brim', enable_prime_tower='0')

	m_std    = write('machine', machine)
	m_manual = write('machine_manual', dict(machine, manual_filament_change='1', change_filament_gcode=MANUAL_CHANGE))

	f_black = write('pla_black', black)
	f_green = write('pla_green', green)

	jobs = sys.argv[1:] or ['keys_2color_MANUAL', 'case', 'case_top', 'phone']
	for job in jobs:
		if job == 'keys_2color_MANUAL':
			slice_job(job, m_manual, write('keys', keys), [f_black, f_green])
		elif job in ('case', 'case_top', 'phone'):
			slice_job(job, m_std, write('case', case), [f_black])


if __name__ == '__main__':
	main()
