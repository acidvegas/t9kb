#!/bin/bash
# exports the keycap and case STLs; run from the repo root
set -e
cd keys
openscad -D 'PART="caps"' -D PRINT=true -o t9kb_keys_caps.stl t9kb_keys.scad &
openscad -D 'PART="legends"' -D PRINT=true -o t9kb_keys_legends.stl t9kb_keys.scad &
cd ../case
openscad -D 'PART="top"' -D PRINT=true -o t9kb_case_top.stl t9kb_case.scad &
openscad -D 'PART="lid"' -D PRINT=true -o t9kb_case_lid.stl t9kb_case.scad &
openscad -D 'PART="top"' -D PRINT=true -o t9kb_phone_top.stl t9kb_phone.scad &
openscad -D 'PART="lid"' -D PRINT=true -o t9kb_phone_lid.stl t9kb_phone.scad &
wait
echo EXPORT DONE
