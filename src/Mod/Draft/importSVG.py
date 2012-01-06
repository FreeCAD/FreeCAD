
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@gmx.fr>                     * 
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__="FreeCAD Draft Workbench - SVG importer/exporter"
__author__ = "Yorik van Havre, Sebastian Hoogen"
__url__ = ["http://free-cad.sourceforge.net"]

'''
This script imports SVG files in FreeCAD. Currently only reads the following entities:
paths, lines, arcs and rects.
'''

import xml.sax, string, FreeCAD, os, math, re, Draft
from draftlibs import fcvec
from FreeCAD import Vector

try: import FreeCADGui
except: gui = False
else: gui = True
try: draftui = FreeCADGui.draftToolBar
except: draftui = None

pythonopen = open

svgcolors = {
	  'Pink': [255, 192, 203], 
	  'Blue': [0, 0, 255], 
          'Honeydew': [240, 255, 240],
          'Purple': [128, 0, 128],
          'Fuchsia': [255, 0, 255],
          'LawnGreen': [124, 252, 0],
          'Amethyst': [153, 102, 204],
          'Crimson': [220, 20, 60],
          'White': [255, 255, 255],
          'NavajoWhite': [255, 222, 173],
          'Cornsilk': [255, 248, 220],
          'Bisque': [255, 228, 196],
          'PaleGreen': [152, 251, 152],
          'Brown': [165, 42, 42],
          'DarkTurquoise': [0, 206, 209],
          'DarkGreen': [0, 100, 0],
          'MediumOrchid': [186, 85, 211],
          'Chocolate': [210, 105, 30],
          'PapayaWhip': [255, 239, 213],
          'Olive': [128, 128, 0],
          'Silver': [192, 192, 192],
          'PeachPuff': [255, 218, 185],
          'Plum': [221, 160, 221],
          'DarkGoldenrod': [184, 134, 11],
          'SlateGrey': [112, 128, 144],
          'MintCream': [245, 255, 250],
          'CornflowerBlue': [100, 149, 237],
          'Gold': [255, 215, 0],
          'HotPink': [255, 105, 180],
          'DarkBlue': [0, 0, 139],
          'LimeGreen': [50, 205, 50],
          'DeepSkyBlue': [0, 191, 255],
          'DarkKhaki': [189, 183, 107],
          'LightGrey': [211, 211, 211],
          'Yellow': [255, 255, 0],
          'Gainsboro': [220, 220, 220],
          'MistyRose': [255, 228, 225],
          'SandyBrown': [244, 164, 96],
          'DeepPink': [255, 20, 147],
          'Magenta': [255, 0, 255],
          'AliceBlue': [240, 248, 255],
          'DarkCyan': [0, 139, 139],
          'DarkSlateGrey': [47, 79, 79],
          'GreenYellow': [173, 255, 47],
          'DarkOrchid': [153, 50, 204],
          'OliveDrab': [107, 142, 35],
          'Chartreuse': [127, 255, 0],
          'Peru': [205, 133, 63],
          'Orange': [255, 165, 0],
          'Red': [255, 0, 0],
          'Wheat': [245, 222, 179],
          'LightCyan': [224, 255, 255],
          'LightSeaGreen': [32, 178, 170],
          'BlueViolet': [138, 43, 226],
          'LightSlateGrey': [119, 136, 153],
          'Cyan': [0, 255, 255],
          'MediumPurple': [147, 112, 219],
          'MidnightBlue': [25, 25, 112],
          'FireBrick': [178, 34, 34],
          'PaleTurquoise': [175, 238, 238],
          'PaleGoldenrod': [238, 232, 170],
          'Gray': [128, 128, 128],
          'MediumSeaGreen': [60, 179, 113],
          'Moccasin': [255, 228, 181],
          'Ivory': [255, 255, 240],
          'DarkSlateBlue': [72, 61, 139],
          'Beige': [245, 245, 220],
          'Green': [0, 128, 0],
          'SlateBlue': [106, 90, 205],
          'Teal': [0, 128, 128],
          'Azure': [240, 255, 255],
          'LightSteelBlue': [176, 196, 222],
          'DimGrey': [105, 105, 105],
          'Tan': [210, 180, 140],
          'AntiqueWhite': [250, 235, 215],
          'SkyBlue': [135, 206, 235],
          'GhostWhite': [248, 248, 255],
          'MediumTurquoise': [72, 209, 204],
          'FloralWhite': [255, 250, 240],
          'LavenderBlush': [255, 240, 245],
          'SeaGreen': [46, 139, 87],
          'Lavender': [230, 230, 250],
          'BlanchedAlmond': [255, 235, 205],
          'DarkOliveGreen': [85, 107, 47],
          'DarkSeaGreen': [143, 188, 143],
          'SpringGreen': [0, 255, 127],
          'Navy': [0, 0, 128],
          'Orchid': [218, 112, 214],
          'SaddleBrown': [139, 69, 19],
          'IndianRed': [205, 92, 92],
          'Snow': [255, 250, 250],
          'SteelBlue': [70, 130, 180],
          'MediumSlateBlue': [123, 104, 238],
          'Black': [0, 0, 0],
          'LightBlue': [173, 216, 230],
          'Turquoise': [64, 224, 208],
          'MediumVioletRed': [199, 21, 133],
          'DarkViolet': [148, 0, 211],
          'DarkGray': [169, 169, 169],
          'Salmon': [250, 128, 114],
          'DarkMagenta': [139, 0, 139],
          'Tomato': [255, 99, 71],
          'WhiteSmoke': [245, 245, 245],
          'Goldenrod': [218, 165, 32],
          'MediumSpringGreen': [0, 250, 154],
          'DodgerBlue': [30, 144, 255],
          'Aqua': [0, 255, 255],
          'ForestGreen': [34, 139, 34],
          'LemonChiffon': [255, 250, 205],
          'LightSlateGray': [119, 136, 153],
          'SlateGray': [112, 128, 144],
          'LightGray': [211, 211, 211],
          'Indigo': [75, 0, 130],
          'CadetBlue': [95, 158, 160],
          'LightYellow': [255, 255, 224],
          'DarkOrange': [255, 140, 0],
          'PowderBlue': [176, 224, 230],
          'RoyalBlue': [65, 105, 225],
          'Sienna': [160, 82, 45],
          'Thistle': [216, 191, 216],
          'Lime': [0, 255, 0],
          'Seashell': [255, 245, 238],
          'DarkRed': [139, 0, 0],
          'LightSkyBlue': [135, 206, 250],
          'YellowGreen': [154, 205, 50],
          'Aquamarine': [127, 255, 212],
          'LightCoral': [240, 128, 128],
          'DarkSlateGray': [47, 79, 79],
          'Khaki': [240, 230, 140],
          'DarkGrey': [169, 169, 169],
          'BurlyWood': [222, 184, 135],
          'LightGoldenrodYellow': [250, 250, 210],
          'MediumBlue': [0, 0, 205],
          'DarkSalmon': [233, 150, 122],
          'RosyBrown': [188, 143, 143],
          'LightSalmon': [255, 160, 122],
          'PaleVioletRed': [219, 112, 147],
          'Coral': [255, 127, 80],
          'Violet': [238, 130, 238],
          'Grey': [128, 128, 128],
          'LightGreen': [144, 238, 144],
          'Linen': [250, 240, 230],
          'OrangeRed': [255, 69, 0],
          'DimGray': [105, 105, 105],
          'Maroon': [128, 0, 0],
          'LightPink': [255, 182, 193],
          'MediumAquamarine': [102, 205, 170],
          'OldLace': [253, 245, 230]
          }

def getcolor(color):
	"checks if the given string is a RGB value, or if it is a named color. returns 1-based RGBA tuple."
	if (color[:1] == "#"):
		r = float(int(color[1:3],16)/255.0)
		g = float(int(color[3:5],16)/255.0)
		b = float(int(color[5:],16)/255.0)
		return (r,g,b,0.0)
	else:
		for k,v in svgcolors.iteritems():
			if (k.lower() == color.lower()):
				r = float(v[0]/255.0)
				g = float(v[1]/255.0)
				b = float(v[2]/255.0)
				return (r,g,b,0.0)

def getsize(width):
	"extracts a number from the given string (removes suffixes)"
	if width[-1] == "%":
		return float(width[:-1])
	elif len(width) > 1:
		for s in ['pt','pc','mm','cm','in','px']:
			if width[-2:] == s:
				return float(width[:-2])
	try:
		s = float(width)
		return s
	except ValueError:
		return width

def getrgb(color):
	"returns a rgb value #000000 from a freecad color"
	r = str(hex(int(color[0]*255)))[2:].zfill(2)
	g = str(hex(int(color[1]*255)))[2:].zfill(2)
	b = str(hex(int(color[2]*255)))[2:].zfill(2)
	return "#"+r+g+b

class svgHandler(xml.sax.ContentHandler):
	"this handler parses the svg files and creates freecad objects"

	def __init__(self):
		"retrieving Draft parameters"
		params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
		self.style = params.GetInt("svgstyle")
                self.count = 0
                self.transform = None
                self.grouptransform = []
                self.lastdim = None

                global Part
                import Part
	
		if gui and draftui:
			r = float(draftui.color.red()/255.0)
			g = float(draftui.color.green()/255.0)
			b = float(draftui.color.blue()/255.0)
			self.lw = float(draftui.widthButton.value())
		else:
			self.lw = float(params.GetInt("linewidth"))
			c = params.GetUnsigned("color")
			r = float(((c>>24)&0xFF)/255)
			g = float(((c>>16)&0xFF)/255)
			b = float(((c>>8)&0xFF)/255)
		self.col = (r,g,b,0.0)

	def format(self,obj):
		"applies styles to passed object"
		if self.style and gui:
			v = obj.ViewObject
			if self.color: v.LineColor = self.color
			if self.width: v.LineWidth = self.width
			if self.fill: v.ShapeColor = self.fill
	
	def startElement(self, name, attrs):

		# reorganizing data into a nice clean dictionary

                self.count += 1

                print "processing element ",self.count,": ",name
                print "existing group transform: ", self.grouptransform
		
		data = {}
		for (keyword,content) in attrs.items():
			content = content.replace(',',' ')
			content = content.split()
			data[keyword]=content

		if 'style' in data:
			content = data['style'][0].replace(' ','')
			content = content.split(';')
			for i in content:
				pair = i.split(':')
				if len(pair)>1:	data[pair[0]]=pair[1]

		for k in ['x','y','x1','y1','x2','y2','width','height']:
			if k in data:
				data[k] = getsize(data[k][0])

		for k in ['fill','stroke','stroke-width','font-size']:
			if k in data:
				if isinstance(data[k],list):
					data[k]=data[k][0]

		# extracting style info
			
		self.fill = None
		self.color = None
		self.width = None
		self.text = None
		
		if 'fill' in data:
			if data['fill'][0] != 'none':
				self.fill = getcolor(data['fill'])
		if 'stroke' in data:
			if data['stroke'][0] != 'none':
				self.color = getcolor(data['stroke'])
		if 'stroke-width' in data:
			if data['stroke-width'] != 'none':
				self.width = getsize(data['stroke-width'])
		if 'transform' in data:
                        m = self.getMatrix(data['transform'])
                        if name == "g":
                                self.grouptransform.append(m)
                        else:
                                self.transform = m
                else:
                        if name == "g":
                                self.grouptransform.append(FreeCAD.Matrix())
                                
                        '''
                        print "existing grouptransform: ",self.grouptransform
                        print "existing transform: ",self.transform
			if "translate" in tr:
                                i0 = tr.index("translate")
                                print "getting translate ",tr
                                if "translate" in self.transform:
                                        self.transform['translate'] = self.transform['translate'].add(Vector(float(tr[i0+1]),-float(tr[i0+2]),0))
                                else:
                                        self.transform['translate'] = Vector(float(tr[i0+1]),-float(tr[i0+2]),0)
                                if "translate" in self.grouptransform:
                                        print "adding to group ",self.grouptransform['translate']
                                        self.transform['translate'] = self.grouptransform['translate'].add(self.transform['translate'])
                        else:
                                if "translate" in self.grouptransform:
                                        print "adding to group ",self.grouptransform['translate']
                                        self.transform['translate'] = self.grouptransform['translate']
                        if "scale" in tr:
                                i0 = tr.index("scale")
                                if "scale" in self.transform:
                                        self.transform['scale'] = self.transform['scale'].add(Vector(float(tr[i0+1]),float(tr[i0+2]),0))
                                else:
                                        print tr
                                        self.transform['scale'] = Vector(float(tr[i0+1]),float(tr[i0+2]),0)
                                if "scale" in self.grouptransform:
                                        self.transform['scale'] = self.transform['scale'].add(self.grouptransform['scale'])
                        else:
                                if "scale" in self.grouptransform:
                                        self.transform['scale'] = self.grouptransform['scale']
                        '''
 
		if (self.style == 1):
			self.color = self.col
			self.width = self.lw

                pathname = None
                if 'id' in data:
                        pathname = data['id'][0]
                        print "name: ",pathname
                        
		# processing paths
                        
		if name == "path":
                        print data
                        
                        if not pathname: pathname = 'Path'

			path = []
			point = []
			lastvec = Vector(0,0,0)
			lastpole = None
			command = None
			relative = False
			firstvec = None

			pathdata = []
			for d in data['d']:
				if (len(d) == 1) and (d in ['m','M','l','L','h','H','v','V','a','A','c','C','q','Q','s','S','t','T']):
					pathdata.append(d)
				else:
					try:
						f = float(d)
						pathdata.append(f)
					except ValueError:
						if d[0].isdigit():
							pathdata.append(d[:-1])
							pathdata.append(d[-1])
						else:
							pathdata.append(d[0])
							pathdata.append(d[1:])

                        # print "debug: pathdata:",pathdata

                        if "freecad:basepoint1" in data:
                                p1 = data["freecad:basepoint1"]
                                p1 = Vector(float(p1[0]),-float(p1[1]),0)
                                p2 = data["freecad:basepoint2"]
                                p2 = Vector(float(p2[0]),-float(p2[1]),0)
                                p3 = data["freecad:dimpoint"]
                                p3 = Vector(float(p3[0]),-float(p3[1]),0)
                                obj = Draft.makeDimension(p1,p2,p3)
                                self.applyTrans(obj)
                                self.format(obj)
                                pathdata = []
                                self.lastdim = obj

			for d in pathdata:
				if (d == "M"):
					command = "move"
					relative = False
					point = []
				elif (d == "m"):
					command = "move"
					relative = True
					point = []
				elif (d == "L"):
					command = "line"
					relative = False
					point = []
				elif (d == "l"):
					command = "line"
					relative = True
					point = []
				elif (d == "H"):
					command = "horizontal"
					relative = False
					point = []
				elif (d == "h"):
					command = "horizontal"
					relative = True
					point = []
				elif (d == "V"):
					command = "vertical"
					relative = False
					point = []
				elif (d == "v"):
					command = "vertical"
					relative = True
					point = []
				elif (d == "A"):
					command = "arc"
					relative = False
					point = []
				elif (d == "a"):
					command = "arc"
					relative = True
					point = []
				elif (d == "Z") or (d == "z"):
					command = "close"
					point = []
				elif (d == "C"):
					command = "cubic"
					relative = False
					smooth = False
					point = []
				elif (d == "c"):
					command = "cubic"
					relative = True
					smooth = False
					point = []
				elif (d == "Q"):
					command = "quadratic"
					relative = False
					smooth = False
					point = []
				elif (d == "q"):
					command = "quadratic"
					relative = True
					smooth = False
					point = []
				elif (d == "S"):
					command = "cubic"
					relative = False
					smooth = True
					point = []
				elif (d == "s"):
					command = "cubic"
					relative = True
					smooth = True
					point = []
				elif (d == "T"):
					command = "quadratic"
					relative = False
					smooth = True
					point = []
				elif (d == "t"):
					command = "quadratic"
					relative = True
					smooth = True
					point = []
				else:
					try:
						point.append(float(d))
					except ValueError:
						pass

				print "command: ",command, ' point: ',point

				if (len(point)==2) and (command=="move"):
					if path:
						sh = Part.Wire(path)
						if self.fill: sh = Part.Face(sh)
                                                sh = self.applyTrans(sh)
						obj = self.doc.addObject("Part::Feature",pathname)
						obj.Shape = sh
						self.format(obj)
						path = []
					if relative:
						lastvec = lastvec.add(Vector(point[0],-point[1],0))
						command="line"
					else:
						lastvec = Vector(point[0],-point[1],0)
					firstvec = lastvec
					print "move ",lastvec
					command = "line"
					lastpole = None
					point = []
				elif (len(point)==2) and (command=="line"):
					if relative:
						currentvec = lastvec.add(Vector(point[0],-point[1],0))
					else:
						currentvec = Vector(point[0],-point[1],0)
                                        if not fcvec.equals(lastvec,currentvec):
                                                seg = Part.Line(lastvec,currentvec).toShape()
                                                print "line ",lastvec,currentvec
                                                lastvec = currentvec
                                                path.append(seg)
					lastpole = None
					point = []
				elif (len(point)==1) and (command=="horizontal"):
					if relative:
						currentvec = lastvec.add(Vector(point[0],0,0))
					else:
						lasty = path[-1].y
						currentvec = Vector(point[0],lasty,0)
					seg = Part.Line(lastvec,currentvec).toShape()
					lastvec = currentvec
					lastpole = None
					path.append(seg)
					point = []
				elif (len(point)==1) and (command=="vertical"):
					if relative:
						currentvec = lastvec.add(Vector(0,-point[0],0))
					else:
						lastx = path[-1].x
						currentvec = Vector(lastx,-point[0],0)
					seg = Part.Line(lastvec,currentvec).toShape()
					lastvec = currentvec
					lastpole = None
					path.append(seg)
					point = []
				elif (len(point)==7) and (command=="arc"):
					if relative:
						currentvec = lastvec.add(Vector(point[-2],-point[-1],0))
					else:
						currentvec = Vector(point[-2],-point[-1],0)
					chord = currentvec.sub(lastvec)
					# perp = chord.cross(Vector(0,0,-1))
                                        # here is a better way to find the perpendicular
                                        if point[4] == 1:
                                                # clockwise
                                                perp = fcvec.rotate2D(chord,-math.pi/2)
                                        else:
                                                # anticlockwise
                                                perp = fcvec.rotate2D(chord,math.pi/2)
					chord = fcvec.scale(chord,.5)
					if chord.Length > point[0]: a = 0
					else: a = math.sqrt(point[0]**2-chord.Length**2)
					s = point[0] - a
					perp = fcvec.scale(perp,s/perp.Length)
					midpoint = lastvec.add(chord.add(perp))
					seg = Part.Arc(lastvec,midpoint,currentvec).toShape()
					lastvec = currentvec
					lastpole = None
					path.append(seg)
					point = []
				elif (command == "close"):
					if not fcvec.equals(lastvec,firstvec):
						seg = Part.Line(lastvec,firstvec).toShape()
						path.append(seg)
					if path:
						sh = Part.Wire(path)
						if self.fill: sh = Part.Face(sh)
                                                sh = self.applyTrans(sh)
						obj = self.doc.addObject("Part::Feature",pathname)
						obj.Shape = sh
						self.format(obj)
						path = []
					point = []
					command = None
				#elif (len(point)==6) and (command=="cubic"):
				elif (command=="cubic") and (((smooth==False) and (len(point)==6)) or (smooth==True and (len(point)==4))) :
					if smooth:
						if relative:
							currentvec = lastvec.add(Vector(point[2],-point[3],0))
							pole2 = lastvec.add(Vector(point[0],-point[1],0))
						else:
							currentvec = Vector(point[2],-point[3],0)
							pole2 = Vector(point[0],-point[1],0)
						if lastpole:
							pole1 = lastvec.sub(lastpole).add(lastvec)
						else:
							pole1 = lastvec
					else: #not smooth
						if relative:
							currentvec = lastvec.add(Vector(point[4],-point[5],0))
							pole1 = lastvec.add(Vector(point[0],-point[1],0))
							pole2 = lastvec.add(Vector(point[2],-point[3],0))
						else:
							currentvec = Vector(point[4],-point[5],0)
							pole1 = Vector(point[0],-point[1],0)
							pole2 = Vector(point[2],-point[3],0)

					if not fcvec.equals(currentvec,lastvec):
                                                mainv = currentvec.sub(lastvec)
                                                pole1v = lastvec.add(pole1)
                                                pole2v = currentvec.add(pole2)
                                                print "cubic curve data:",mainv.normalize(),pole1v.normalize(),pole2v.normalize()
                                                if pole1.distanceToLine(lastvec,currentvec) < 10**(-1*Draft.precision()) and pole2.distanceToLine(lastvec,currentvec) < 10**(-1*Draft.precision()):
                                                        print "straight segment"
                                                        seg = Part.Line(lastvec,currentvec).toShape()
                                                else:
                                                        print "cubic bezier segment"
                                                        b = Part.BezierCurve()
                                                        b.setPoles([lastvec,pole1,pole2,currentvec])
                                                        seg = b.toShape()
						print "connect ",lastvec,currentvec
						lastvec = currentvec
						lastpole = pole2
						path.append(seg)
					point = []

				elif (command=="quadratic") and (((smooth==False) and (len(point)==4)) or (smooth==True and (len(point)==2))) :
					if smooth:
						if relative:
							currentvec = lastvec.add(Vector(point[0],-point[1],0))
						else:
							currentvec = Vector(point[0],-point[1],0)
						if lastpole:
							pole1 = lastvec.sub(lastpole).add(lastvec)
						else:
							pole1 = lastvec
					else: #not smooth
						if relative:
							currentvec = lastvec.add(Vector(point[2],-point[3],0))
							pole1 = lastvec.add(Vector(point[0],-point[1],0))
						else:
							currentvec = Vector(point[2],-point[3],0)
							pole1 = Vector(point[0],-point[1],0)

					if not fcvec.equals(currentvec,lastvec):
                                                if pole1.distanceToLine(lastvec,currentvec) < 10**(-1*Draft.precision()):
                                                        print "straight segment"
                                                        seg = Part.Line(lastvec,currentvec).toShape()
                                                else:
                                                        print "quadratic bezier segment"
                                                        b = Part.BezierCurve()
                                                        b.setPoles([lastvec,pole1,currentvec])
                                                        seg = b.toShape()
						print "connect ",lastvec,currentvec
						lastvec = currentvec
						lastpole = pole1
						path.append(seg)
					point = []

			if path:
				sh = Part.Wire(path)
				if self.fill: sh = Part.Face(sh)
                                sh = self.applyTrans(sh)
				obj = self.doc.addObject("Part::Feature",pathname)
				obj.Shape = sh
				self.format(obj)

		# processing rects

		if name == "rect":
                        if not pathname: pathname = 'Rectangle'
			p1 = Vector(data['x'],-data['y'],0)
			p2 = Vector(data['x']+data['width'],-data['y'],0)
			p3 = Vector(data['x']+data['width'],-data['y']-data['height'],0)
			p4 = Vector(data['x'],-data['y']-data['height'],0)
			edges = []
			edges.append(Part.Line(p1,p2).toShape())
			edges.append(Part.Line(p2,p3).toShape())
			edges.append(Part.Line(p3,p4).toShape())
			edges.append(Part.Line(p4,p1).toShape())
			sh = Part.Wire(edges)
			if self.fill: sh = Part.Face(sh)
                        sh = self.applyTrans(sh)
			obj = self.doc.addObject("Part::Feature",pathname)
			obj.Shape = sh
			self.format(obj)
				     
                # processing lines

		if name == "line":
                        if not pathname: pathname = 'Line'
			p1 = Vector(float(data['x1'][0]),-float(data['y1'][0]),0)
			p2 = Vector(float(data['x2'][0]),-float(data['y2'][0]),0)
			sh = Part.Line(p1,p2).toShape()
                        sh = self.applyTrans(sh)
			obj = self.doc.addObject("Part::Feature",pathname)
			obj.Shape = sh
			self.format(obj)

                # processing circles

                if (name == "circle") and (not ("freecad:skip" in data)) :
                        if not pathname: pathname = 'Circle'
                        c = Vector(float(data['cx'][0]),-float(data['cy'][0]),0)
                        r = float(data['r'][0])
                        sh = Part.makeCircle(r)
                        if self.fill:
                                sh = Part.Wire([sh])
                                sh = Part.Face(sh)
                        sh.translate(c)
                        sh = self.applyTrans(sh)
			obj = self.doc.addObject("Part::Feature",pathname)
			obj.Shape = sh
			self.format(obj)

                # processing texts

		if name in ["text","tspan"]:
                        if not("freecad:skip" in data):
                                print "processing a text"
                                if 'x' in data:
                                        self.x = data['x']
                                else:
                                        self.x = 0
                                if 'y' in data:
                                        self.y = data['y']
                                else:
                                        self.y = 0
                                if 'font-size' in data:
                                        if data['font-size'] != 'none':
                                                self.text = getsize(data['font-size'])
                                else:
                                        self.text = 1
                        else:
                                if self.lastdim:
                                        self.lastdim.ViewObject.FontSize = int(getsize(data['font-size']))

                print "done processing element ",self.count
                
	def characters(self,content):
		if self.text:
                        print "reading characters", str(content)
			obj=self.doc.addObject("App::Annotation",'Text')
			obj.LabelText = content.encode('latin1')
			vec = Vector(self.x,-self.y,0)
                        if self.transform:
                                vec = self.translateVec(vec,self.transform)
                                print "own transform: ",self.transform, vec
                        for i in range(len(self.grouptransform)):
                                #vec = self.translateVec(vec,self.grouptransform[-i-1])
                                vec = self.grouptransform[-i-1].multiply(vec)
                        print "applying vector: ",vec
                        obj.Position = vec
			if gui:
				obj.ViewObject.FontSize = int(self.text)
				if self.fill: obj.ViewObject.TextColor = self.fill
				else: obj.ViewObject.TextColor = (0.0,0.0,0.0,0.0)

        def endElement(self, name):
                if not name in ["tspan"]:
                        self.transform = None
                        self.text = None
                if name == "g":
                        print "closing group"
                        self.grouptransform.pop()

        def applyTrans(self,sh):
                if isinstance(sh,Part.Shape):
                        if self.transform:
                                print "applying object transform: ",self.transform
                                sh = sh.transformGeometry(self.transform)
                        for i in range(len(self.grouptransform)):
                                print "applying group transform: ",self.grouptransform[-i-1]
                                sh = sh.transformGeometry(self.grouptransform[-i-1])
                        return sh
                elif Draft.getType(sh) == "Dimension":
                        pts = []
                        for p in [sh.Start,sh.End,sh.Dimline]:
                                cp = Vector(p)
                                if self.transform:
                                        print "applying object transform: ",self.transform
                                        cp = self.transform.multiply(cp)
                                for i in range(len(self.grouptransform)):
                                        print "applying group transform: ",self.grouptransform[-i-1]
                                        cp = self.grouptransform[-i-1].multiply(cp)
                                pts.append(cp)
                        sh.Start = pts[0]
                        sh.End = pts[1]
                        sh.Dimline = pts[2]

        def translateVec(self,vec,mat):
                v = Vector(mat.A14,mat.A24,mat.A34)
                return vec.add(v)

        def getMatrix(self,tr):
                "returns a FreeCAD matrix from a svg transform attribute"
                s = ""
                for l in tr:
                        s += l
                        s += " "
                s=s.replace("("," ")
                s=s.replace(")"," ")
                s = s.strip()
                tr = s.split()
                m = FreeCAD.Matrix()
                for i in range(len(tr)):
                        if tr[i] == 'translate':
                                vec = Vector(float(tr[i+1]),-float(tr[i+2]),0)
                                m.move(vec)
                        elif tr[i] == 'scale':
                                vec = Vector(float(tr[i+1]),float(tr[i+2]),0)
                                m.scale(vec)
                        #elif tr[i] == 'rotate':
                        #        m.rotateZ(float(tr[i+1]))
                print "generating transformation: ",m
                return m
			
def decodeName(name):
	"decodes encoded strings"
	try:
		decodedName = (name.decode("utf8"))
	except UnicodeDecodeError:
		try:
			decodedName = (name.decode("latin1"))
		except UnicodeDecodeError:
			print "svg: error: couldn't determine character encoding"
			decodedName = name
	return decodedName

def getContents(filename,tag,stringmode=False):
        "gets the contents of all the occurences of the given tag in the given file"
        result = {}
        if stringmode:
                contents = filename
        else:
                f = pythonopen(filename)
                contents = ''
                for line in f: contents += line
                f.close()
        contents = contents.replace('\n','_linebreak')
        searchpat = '<'+tag+'.*?</'+tag+'>'
        tags = re.findall(searchpat,contents)
        for t in tags:
                tagid = re.findall('id="(.*?)"',t)
                if tagid:
                        tagid = tagid[0]
                else:
                        tagid = 'none'
                res = t.replace('_linebreak','\n')
                result[tagid] = res
        return result

def open(filename):
	docname=os.path.split(filename)[1]
	doc=FreeCAD.newDocument(docname)
	doc.Label = decodeName(docname[:-4])
	parser = xml.sax.make_parser()
	parser.setContentHandler(svgHandler())
	parser._cont_handler.doc = doc
        f = pythonopen(filename)
	parser.parse(f)
        f.close()
	doc.recompute()
        return doc

def insert(filename,docname):
	try:
		doc=FreeCAD.getDocument(docname)
	except:
		doc=FreeCAD.newDocument(docname)
	parser = xml.sax.make_parser()
	parser.setContentHandler(svgHandler())
	parser._cont_handler.doc = doc
	parser.parse(pythonopen(filename))
	doc.recompute()

def export(exportList,filename):
	"called when freecad exports a file"

	# finding sheet size
	minx = 10000
	miny = 10000
	maxx = 0
	maxy = 0
	for ob in exportList:
		if ob.isDerivedFrom("Part::Feature"):
			for v in ob.Shape.Vertexes:
				if v.Point.x < minx: minx = v.Point.x
				if v.Point.x > maxx: maxx = v.Point.x
				if v.Point.y < miny: miny = v.Point.y
				if v.Point.y > maxy: maxy = v.Point.y
	margin = (maxx-minx)*.01
	minx -= margin 
	maxx += margin
	miny -= margin
	maxy += margin
	sizex = maxx-minx
	sizey = maxy-miny
	miny += margin
	boty = sizey+miny

	# writing header
	svg = pythonopen(filename,'wb')	
	svg.write('<?xml version="1.0"?>\n')
	svg.write('<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"')
	svg.write(' "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">\n')
	svg.write('<svg')
	svg.write(' width="' + str(sizex) + '" height="' + str(sizey) + '"')
	svg.write(' viewBox="0 0 ' + str(sizex) + ' ' + str(sizey) + '"')
	svg.write(' xmlns="http://www.w3.org/2000/svg" version="1.1"')
	svg.write('>\n')

	# writing paths
	for ob in exportList:
                svg.write('<g transform="translate('+str(-minx)+','+str(-miny+(2*margin))+') scale(1,-1)">\n')
                svg.write(Draft.getSVG(ob))
                svg.write('</g>\n')
                
	# closing
	svg.write('</svg>')
	svg.close()
	FreeCAD.Console.PrintMessage("successfully exported "+filename)
