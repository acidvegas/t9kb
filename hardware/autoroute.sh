#!/bin/bash
# usage: ./autoroute.sh   regenerate board, freeroute it, pour GND, DRC (retries with different via costs)

cd "$(dirname "$0")"
for vc in 50 30 80 100 40 60 20 120; do
	python3 gen_pcb.py
	python3 route.py export 2>/dev/null
	rm -f route.ses
	timeout 400 java -jar ../tools/freerouting.jar -de route.dsn -do route.ses -mp 200 --router.via_costs=$vc --router.optimizer.enabled=false --gui.enabled=false > freerouting.log 2>&1
	python3 route.py import
	out=$(kicad-cli pcb drc --severity-error -o drc.rpt t9kb.kicad_pcb | head -2)
	echo "via_costs=$vc: $out"
	echo "$out" | grep -q "Found 0 violations" && echo "$out" | grep -q "Found 0 unconnected" && break
done
