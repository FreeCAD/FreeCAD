import FreeCAD, Part, Drawing

startSVG = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
 
<svg xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:ev="http://www.w3.org/2001/xml-events"
     version="1.1" baseProfile="full"
     width="297mm" height="420mm">
"""


def createSVG(Part):

	# projecting with HLR
	[Visibly,Hidden] = Drawing.project(Part)
	buffer = startSVG

	for i in Hidden.Edges:
		buffer += '<line x1="' +`i.Vertexes[0].X` +'" y1="' +`i.Vertexes[0].Y` +'" x2="'+ `i.Vertexes[1].X`+ '" y2="'+ `i.Vertexes[1].Y`+ '" stroke="#fff000" stroke-width="1px" />\n'
	for i in Visibly.Edges:
		buffer += '<line x1="' +`i.Vertexes[0].X` +'" y1="' +`i.Vertexes[0].Y` +'" x2="'+ `i.Vertexes[1].X`+ '" y2="'+ `i.Vertexes[1].Y`+ '" stroke="#000000" stroke-width="1px" />\n'
		
	buffer += '</svg>'

	return buffer

