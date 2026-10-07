#!/usr/bin/env python3
# Export DSN / import SES + GND pour.  usage: route.py <export|import>
import os
import sys
import pcbnew

step  = sys.argv[1]
here  = os.path.dirname(os.path.abspath(__file__))
pcb   = os.path.join(here, 't9kb.kicad_pcb')
board = pcbnew.LoadBoard(pcb)

if step == 'export':
	# Specctra mangles the custom dome ring pad; route to the ring's via instead and keep the ring as a netless obstacle
	for fp in board.GetFootprints():
		if fp.GetFPID().GetLibItemName() == 'MetalDome_5mm':
			for p in fp.Pads():
				if p.GetShape() == pcbnew.PAD_SHAPE_CUSTOM:
					p.SetNetCode(0)
	# route with a little margin over the 0.15mm rule (rounded pads are approximated in DSN)
	board.GetDesignSettings().m_NetSettings.GetDefaultNetclass().SetClearance(pcbnew.FromMM(0.18))
	pcbnew.ExportSpecctraDSN(board, os.path.join(here, 'route.dsn'))
else:
	pcbnew.ImportSpecctraSES(board, os.path.join(here, 'route.ses'))
	# GND pour on the parts side, 0.3mm clearance, solid pad connections
	gnd  = board.FindNet('GND')
	zone = pcbnew.ZONE(board)
	zone.SetLayer(pcbnew.F_Cu)
	zone.SetNet(gnd)
	zone.SetLocalClearance(pcbnew.FromMM(0.3))
	zone.SetMinThickness(pcbnew.FromMM(0.2))
	zone.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
	bb = board.GetBoardEdgesBoundingBox()
	ol = zone.Outline()
	ol.NewOutline()
	for x, y in [(bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetTop()), (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom())]:
		ol.Append(x, y)
	board.Add(zone)
	pcbnew.ZONE_FILLER(board).Fill(board.Zones())
	board.Save(pcb)
	print('routed', len(board.GetTracks()), 'tracks/vias')
