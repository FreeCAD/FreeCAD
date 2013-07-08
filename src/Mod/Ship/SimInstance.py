#***************************************************************************
#*																		 *
#*   Copyright (c) 2011, 2012											  *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>							*  
#*																		 *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)	*
#*   as published by the Free Software Foundation; either version 2 of	 *
#*   the License, or (at your option) any later version.				   *
#*   for detail see the LICENCE text file.								 *
#*																		 *
#*   This program is distributed in the hope that it will be useful,	   *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of		*
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		 *
#*   GNU Library General Public License for more details.				  *
#*																		 *
#*   You should have received a copy of the GNU Library General Public	 *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA																   *
#*																		 *
#***************************************************************************

from math import *
import threading

# Qt library
from PyQt4 import QtGui,QtCore

# COIN
from pivy.coin import *
from pivy import coin

# FreeCAD
import FreeCAD,FreeCADGui
from FreeCAD import Base, Vector
import Part

# Ship design module
from shipUtils import Paths, Math

class FreeSurfaceFace:
	def __init__(self, pos, normal, l, b):
		""" Face storage.
		@param pos Face position.
		@param normal Face normal.
		@param l Element length (distance between elements at x direction)
		@param b Element beam (distance between elements at y direction)
		"""
		self.pos	= pos
		self.normal = normal
		self.area   = l*b

	def __init__(self, pos, normal, area):
		""" Face storage.
		@param pos Face position.
		@param normal Face normal.
		@param area Element area
		"""
		self.pos	= pos
		self.normal = normal
		self.area   = area

class ShipSimulation:
	def __init__(self, obj, fsMeshData, waves, error):
		""" Creates a new simulation instance on active document.
		@param obj Created Part::FeaturePython object.
		@param h Sea water level.
		@param fsMeshData [L,B,N] Free surface mesh data, with lenght 
		(x), Beam (y) and desired number of points.
		@param waves [[A,T,phi,heading],] Waves involved.
		@param error Relation between the minimum and the maximum Green's function values.
		"""
		# Add uniqueness property to identify Tank instances
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship simulation instance",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyBool","IsShipSimulation","ShipSimulation", tooltip).IsShipSimulation=True
		# Store general data
		tooltip = str(QtGui.QApplication.translate("Ship","Free surface length in the x direction",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloat","L","ShipSimulation", tooltip).L=fsMeshData[0]
		tooltip = str(QtGui.QApplication.translate("Ship","Free surface length in the y direction",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloat","B","ShipSimulation", tooltip).B=fsMeshData[1]
		tooltip = str(QtGui.QApplication.translate("Ship","Free surface number of elements at x direction",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyInteger","FS_Nx","ShipSimulation", tooltip).FS_Nx=fsMeshData[2]
		tooltip = str(QtGui.QApplication.translate("Ship","Free surface number of elements at y direction",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyInteger","FS_Ny","ShipSimulation", tooltip).FS_Ny=fsMeshData[3]
		tooltip = str(QtGui.QApplication.translate("Ship","Relative error of the Green's function",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloat","error","ShipSimulation", tooltip).error=error
		# Compute free surface mesh
		self.createFSMesh(obj,fsMeshData)
		self.createVirtualFS(obj,fsMeshData,error)
		self.computeWaves(obj,waves)
		# Store waves
		tooltip = str(QtGui.QApplication.translate("Ship","Waves (Amplitude,period,phase)",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyVectorList","Waves","ShipSimulation", tooltip).Waves=[]
		tooltip = str(QtGui.QApplication.translate("Ship","Waves direction (0 deg to stern waves)",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloatList","Waves_Dir","ShipSimulation", tooltip).Waves_Dir=[]
		w = []
		d = []
		for i in range(0,len(waves)):
			w.append(Vector(waves[i][0], waves[i][1], waves[i][2]))
			d.append(waves[i][3])
		obj.Waves = w
		obj.Waves_Dir = d
		# Add shapes
		shape = self.computeShape(obj)
		if not shape:
			obj.IsShipSimulation=False
			return
		obj.Shape = shape
		obj.Proxy = self

	def onChanged(self, fp, prop):
		""" Property changed, tank must be recomputed """
		if prop == "IsShipSimulation":
			msg = QtGui.QApplication.translate("ship_console", "Ussually you don't want to modify manually this option",
										   None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintWarning(msg + "\n")

	def execute(self, obj):
		""" Shape recomputation called """
		obj.Shape = self.computeShape(obj)

	def createFSMesh(self, obj, fsMeshData):
		""" Create or modify free surface mesh.
		@param obj Created Part::FeaturePython object.
		@param fsMeshData [L,B,Nx,Ny] Free surface mesh data, with lenght 
		(x), Breath (y) and desired number of points at each direction.
		"""
		# Study input object
		try:
			props = obj.PropertiesList
			props.index("IsShipSimulation")
			if not obj.IsShipSimulation:
				msg = QtGui.QApplication.translate("ship_console", "Object is not a valid ship simulation",
										   None,QtGui.QApplication.UnicodeUTF8)
				FreeCAD.Console.PrintError(msg + '\n')
				return
		except ValueError:
			msg = QtGui.QApplication.translate("ship_console", "Object is not a ship simulation",
									   None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintError(msg + '\n')
			return
		# Get areas and number of elements per direction
		L    = fsMeshData[0]
		B    = fsMeshData[1]
		nx   = fsMeshData[2]
		ny   = fsMeshData[3]
		N    = nx*ny
		A	 = L*B
		area = A/N
		l	 = L/nx
		b	 = B/ny
		# Start data fields if not already exist
		props = obj.PropertiesList
		try:
			props.index("FS_Position")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("Ship","Free surface elements position",
													   None,QtGui.QApplication.UnicodeUTF8))
			obj.addProperty("App::PropertyVectorList","FS_Position","ShipSimulation", tooltip).FS_Position=[]
		try:
			props.index("FS_Area")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("Ship","Free surface elements area",
													   None,QtGui.QApplication.UnicodeUTF8))
			obj.addProperty("App::PropertyFloatList","FS_Area","ShipSimulation", tooltip).FS_Area=[]
		try:
			props.index("FS_Normal")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("Ship","Free surface elements normal",
													   None,QtGui.QApplication.UnicodeUTF8))
			obj.addProperty("App::PropertyVectorList","FS_Normal","ShipSimulation", tooltip).FS_Normal=[]
		# Fill data
		obj.L = L
		obj.B = B
		obj.FS_Nx = nx
		obj.FS_Ny = ny
		pos	= []
		areas  = []
		normal = []
		for i in range(0,nx):
			for j in range(0,ny):
				pos.append(Vector(-0.5*L + (i+0.5)*l,-0.5*B + (j+0.5)*b,0.0))
				areas.append(l*b)
				normal.append(Vector(0.0,0.0,1.0))
		obj.FS_Position = pos[:]
		obj.FS_Area	 = areas[:]
		obj.FS_Normal   = normal[:]

	def createVirtualFS(self, obj, fsMeshData, error):
		""" Computes the number of required extended free surfaces.
		@param obj Created Part::FeaturePython object.
		@param fsMeshData [L,B,Nx,Ny] Free surface mesh data, with lenght 
		(x), Breath (y) and desired number of points at each direction.
		@param error Relation between the minimum and the maximum Green's function values.
		"""
		# Study input object
		try:
			props = obj.PropertiesList
			props.index("IsShipSimulation")
			if not obj.IsShipSimulation:
				msg = QtGui.QApplication.translate("ship_console", "Object is not a valid ship simulation",
										   None,QtGui.QApplication.UnicodeUTF8)
				FreeCAD.Console.PrintError(msg + '\n')
				return
		except ValueError:
			msg = QtGui.QApplication.translate("ship_console", "Object is not a ship simulation",
									   None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintError(msg + '\n')
			return
		# Get dimensions of the elements
		L    = fsMeshData[0]
		B    = fsMeshData[1]
		nx   = fsMeshData[2]
		ny   = fsMeshData[3]
		dx   = L / nx
		dy   = B / ny
		# Compute maximum Green's function considering flat free surface
		Gmax = dx*asinh(dy/dx) + dy*asinh(dx/dy)
		# Locate the distance (number of free surface) to get the minimum required value
		Gmin = error*Gmax
		x  = (L-dx)/2.0
		Nx = 0
		G  = Gmin + 1.0
		while(G > Gmin):
			x  = x + L
			Nx = Nx + 1
			G = 1.0 / (4.0*pi * x)
		y  = (B-dy)/2.0
		Ny = 0
		G  = Gmin + 1.0
		while(G > Gmin):
			y  = y + L
			Ny = Ny + 1
			G = 1.0 / (4.0*pi * y)
		# Register computed data
		try:
			props.index("Sea_Nx")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("Ship","Number of repetitions of the free surface at x direction",
													   None,QtGui.QApplication.UnicodeUTF8))
			obj.addProperty("App::PropertyInteger","Sea_Nx","ShipSimulation", tooltip).Sea_Nx=0
		try:
			props.index("Sea_Ny")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("Ship","Number of repetitions of the free surface at y direction",
													   None,QtGui.QApplication.UnicodeUTF8))
			obj.addProperty("App::PropertyInteger","Sea_Ny","ShipSimulation", tooltip).Sea_Ny=0
		obj.Sea_Nx = Nx
		obj.Sea_Ny = Ny

	def computeWaves(self, obj, waves):
		""" Add waves effect to free surface mesh positions.
		@param obj Created Part::FeaturePython object.
		@param waves waves data [A,T,phase, heading].
		"""
		grav = 9.81
		positions = obj.FS_Position[:]
		for i in range(0, len(positions)):
			for w in waves:
				A	   = w[0]
				T	   = w[1]
				phase   = w[2]
				heading = pi*w[3]/180.0
				wl	  = 0.5 * grav / pi * T*T
				k	   = 2.0*pi/wl
				frec	= 2.0*pi/T
				pos	 = obj.FS_Position[i]
				l	   = pos.x*cos(heading) + pos.y*sin(heading)
				amp	 = A*sin(k*l + phase)
				positions[i].z = positions[i].z + amp
		obj.FS_Position = positions[:]

	def computeShape(self, obj):
		""" Computes simulation involved shapes.
		@param obj Created Part::FeaturePython object.
		@return Shape
		"""
		nx	 = obj.FS_Nx
		ny	 = obj.FS_Ny
		mesh = FSMesh(obj)
		surf = Part.BSplineSurface()
		pos  = []
		for i in range(0,nx):
			pos.append([])
			for j in range(0,ny):
				pos[i].append(mesh[i][j].pos)
		surf.interpolate(pos)
		return surf.toShape()

class ViewProviderShipSimulation:
	def __init__(self, obj):
		""" Set this object to the proxy object of the actual view provider """
		obj.Proxy = self

	def attach(self, obj):
		""" Setup the scene sub-graph of the view provider, this method is mandatory """
		return

	def updateData(self, fp, prop):
		""" If a property of the handled feature has changed we have the chance to handle this here """
		return

	def getDisplayModes(self,obj):
		''' Return a list of display modes. '''
		modes=[]
		return modes

	def getDefaultDisplayMode(self):
		''' Return the name of the default display mode. It must be defined in getDisplayModes. '''
		return "Flat Lines"

	def setDisplayMode(self,mode):
		''' Map the display mode defined in attach with those defined in getDisplayModes.
		Since they have the same names nothing needs to be done. This method is optinal.
		'''
		return mode

	def onChanged(self, vp, prop):
		''' Print the name of the property that has changed '''
		# FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")

	def __getstate__(self):
		''' When saving the document this object gets stored using Python's cPickle module.
		Since we have some un-pickable here -- the Coin stuff -- we must define this method
		to return a tuple of all pickable objects or None.
		'''
		return None

	def __setstate__(self,state):
		''' When restoring the pickled object from document we have the chance to set some
		internals here. Since no data were pickled nothing needs to be done here.
		'''
		return None

	def getIcon(self):
		return """
		/* XPM */
		static char * Sim_xpm[] = {
		"32 32 301 2",
		"  	c None",
		". 	c #CCCCCC",
		"+ 	c #A9A9A9",
		"@ 	c #989898",
		"# 	c #A1A1A1",
		"$ 	c #C3C3C3",
		"% 	c #C1C0C1",
		"& 	c #BFBFBF",
		"* 	c #A7A7A7",
		"= 	c #808080",
		"- 	c #5C5C5C",
		"; 	c #565655",
		"> 	c #4E4E4E",
		", 	c #676767",
		"' 	c #898989",
		") 	c #B6B5B6",
		"! 	c #BABABA",
		"~ 	c #B9B9B9",
		"{ 	c #A5A5A5",
		"] 	c #7E7E7E",
		"^ 	c #595A59",
		"/ 	c #575656",
		"( 	c #535353",
		"_ 	c #505050",
		": 	c #4D4D4C",
		"< 	c #474747",
		"[ 	c #404040",
		"} 	c #4D4D4D",
		"| 	c #787878",
		"1 	c #B8B7B8",
		"2 	c #B6B6B6",
		"3 	c #888888",
		"4 	c #7C7C7C",
		"5 	c #575657",
		"6 	c #535354",
		"7 	c #4E4D4E",
		"8 	c #4A4A4A",
		"9 	c #444444",
		"0 	c #414141",
		"a 	c #3E3E3E",
		"b 	c #393938",
		"c 	c #313131",
		"d 	c #393939",
		"e 	c #636363",
		"f 	c #ABABAB",
		"g 	c #B3B3B3",
		"h 	c #848484",
		"i 	c #787979",
		"j 	c #545454",
		"k 	c #515151",
		"l 	c #4B4B4B",
		"m 	c #484748",
		"n 	c #3B3B3B",
		"o 	c #383838",
		"p 	c #353535",
		"q 	c #323232",
		"r 	c #2F2F2E",
		"s 	c #2A2A2A",
		"t 	c #222323",
		"u 	c #252625",
		"v 	c #AFAFAF",
		"w 	c #767676",
		"x 	c #484848",
		"y 	c #454545",
		"z 	c #424242",
		"A 	c #3F3F3E",
		"B 	c #3B3B3C",
		"C 	c #393838",
		"D 	c #2F2F2F",
		"E 	c #2C2C2C",
		"F 	c #292929",
		"G 	c #262626",
		"H 	c #222222",
		"I 	c #1F1F20",
		"J 	c #171716",
		"K 	c #959595",
		"L 	c #747474",
		"M 	c #4E4E4F",
		"N 	c #4C4B4C",
		"O 	c #484849",
		"P 	c #424243",
		"Q 	c #282828",
		"R 	c #525251",
		"S 	c #373737",
		"T 	c #353636",
		"U 	c #333233",
		"V 	c #30302F",
		"W 	c #2C2D2D",
		"X 	c #232323",
		"Y 	c #201F20",
		"Z 	c #1D1D1D",
		"` 	c #151414",
		" .	c #717272",
		"..	c #4C4C4C",
		"+.	c #484949",
		"@.	c #464545",
		"#.	c #424343",
		"$.	c #3A3A3A",
		"%.	c #5D4A49",
		"&.	c #7E7E86",
		"*.	c #56569F",
		"=.	c #3E3E41",
		"-.	c #757575",
		";.	c #575757",
		">.	c #222221",
		",.	c #262627",
		"'.	c #242423",
		").	c #212020",
		"!.	c #1A1A1A",
		"~.	c #121212",
		"{.	c #939493",
		"].	c #6F6F6F",
		"^.	c #494949",
		"/.	c #464646",
		"(.	c #434343",
		"_.	c #554545",
		":.	c #686863",
		"<.	c #939394",
		"[.	c #BDBDBD",
		"}.	c #202021",
		"|.	c #1E1E1E",
		"1.	c #171718",
		"2.	c #0F0F0F",
		"3.	c #929292",
		"4.	c #6C6D6D",
		"5.	c #464746",
		"6.	c #525F73",
		"7.	c #444648",
		"8.	c #3D3D3D",
		"9.	c #2D2C2A",
		"0.	c #A1A2A2",
		"a.	c #AAACAC",
		"b.	c #A6A7A7",
		"c.	c #A8AAAA",
		"d.	c #AFB0B0",
		"e.	c #777676",
		"f.	c #9A9A9A",
		"g.	c #1B1B1B",
		"h.	c #181818",
		"i.	c #0C0C0C",
		"j.	c #909090",
		"k.	c #6B6A6B",
		"l.	c #55657E",
		"m.	c #6990FB",
		"n.	c #6483CD",
		"o.	c #5871B2",
		"p.	c #434E7E",
		"q.	c #A97C76",
		"r.	c #AB7777",
		"s.	c #AC7070",
		"t.	c #A26565",
		"u.	c #805C5C",
		"v.	c #848686",
		"w.	c #424342",
		"x.	c #151515",
		"y.	c #0A0909",
		"z.	c #8F8F8F",
		"A.	c #676868",
		"B.	c #3B3A3A",
		"C.	c #383738",
		"D.	c #353534",
		"E.	c #45525F",
		"F.	c #6367AC",
		"G.	c #804682",
		"H.	c #942A39",
		"I.	c #991312",
		"J.	c #540901",
		"K.	c #393742",
		"L.	c #1C1C1C",
		"M.	c #191919",
		"N.	c #161515",
		"O.	c #121313",
		"P.	c #070707",
		"Q.	c #8D8E8D",
		"R.	c #656566",
		"S.	c #3E3F3F",
		"T.	c #2F2E2F",
		"U.	c #353838",
		"V.	c #35496A",
		"W.	c #3E4D88",
		"X.	c #354889",
		"Y.	c #5573D7",
		"Z.	c #5D80FB",
		"`.	c #374899",
		" +	c #293338",
		".+	c #101010",
		"++	c #0D0D0D",
		"@+	c #040404",
		"#+	c #8C8C8C",
		"$+	c #8B8B8B",
		"%+	c #4B4A4B",
		"&+	c #303030",
		"*+	c #333232",
		"=+	c #2F2F30",
		"-+	c #232223",
		";+	c #1A1919",
		">+	c #2E3949",
		",+	c #5C7BA3",
		"'+	c #36467D",
		")+	c #536F93",
		"!+	c #0A0A0A",
		"~+	c #010101",
		"{+	c #C1C1C1",
		"]+	c #B8B8B8",
		"^+	c #A0A0A0",
		"/+	c #3F3F3F",
		"(+	c #222122",
		"_+	c #202020",
		":+	c #161717",
		"<+	c #141414",
		"[+	c #111011",
		"}+	c #0D0E0E",
		"|+	c #0B0B0A",
		"1+	c #000000",
		"2+	c #525252",
		"3+	c #686868",
		"4+	c #ADADAD",
		"5+	c #9E9F9F",
		"6+	c #6D6D6D",
		"7+	c #3C3C3C",
		"8+	c #131414",
		"9+	c #111111",
		"0+	c #0E0E0E",
		"a+	c #0B0B0B",
		"b+	c #080708",
		"c+	c #050504",
		"d+	c #4C4D4C",
		"e+	c #4D4C4D",
		"f+	c #494A4A",
		"g+	c #454444",
		"h+	c #9D9D9D",
		"i+	c #9E9E9E",
		"j+	c #AEAEAE",
		"k+	c #BEBEBF",
		"l+	c #BEBDBD",
		"m+	c #979797",
		"n+	c #6A6B6A",
		"o+	c #3F3F40",
		"p+	c #020202",
		"q+	c #030303",
		"r+	c #878787",
		"s+	c #69696A",
		"t+	c #868685",
		"u+	c #646464",
		"v+	c #474647",
		"w+	c #656565",
		"x+	c #9E9F9E",
		"y+	c #A8A8A8",
		"z+	c #AFAFAE",
		"A+	c #A4A4A4",
		"B+	c #7A7A7A",
		"C+	c #969696",
		"D+	c #363636",
		"E+	c #777776",
		"F+	c #8C8D8D",
		"G+	c #7D7D7D",
		"H+	c #5E5E5E",
		"I+	c #4F4F50",
		"J+	c #808181",
		"K+	c #707070",
		"L+	c #909191",
		"M+	c #9C9C9C",
		"N+	c #787877",
		"O+	c #696969",
		"P+	c #616161",
		"Q+	c #6E6E6E",
		"R+	c #7C7B7C",
		"S+	c #777677",
		"T+	c #6F6E6E",
		"U+	c #595959",
		"V+	c #717171",
		"W+	c #8D8D8D",
		"X+	c #515051",
		"Y+	c #49494A",
		"Z+	c #4B4A4A",
		"`+	c #606060",
		" @	c #6A6A6A",
		".@	c #616162",
		"+@	c #6C6D6C",
		"@@	c #767777",
		"#@	c #727272",
		"$@	c #6B6B6B",
		"%@	c #828283",
		"&@	c #757475",
		"*@	c #444545",
		"=@	c #565656",
		"-@	c #5A595A",
		";@	c #666666",
		">@	c #878687",
		",@	c #8A8A8A",
		"'@	c #797979",
		")@	c #444344",
		"!@	c #7F8080",
		"~@	c #737373",
		"{@	c #484747",
		"]@	c #707170",
		"^@	c #7F7F7F",
		"/@	c #676867",
		"(@	c #4D4C4C",
		"_@	c #5F5F5F",
		":@	c #434444",
		"                                                                ",
		"                                                                ",
		"            . +                                                 ",
		"            @ # $ % & *                                         ",
		"            = - ; > , ' ) ! ~ {                                 ",
		"            ] ^ / ( _ : < [ } | # 1 2 # 3                       ",
		"            4 5 6 _ 7 8 < 9 0 a b c d e ' f g + h               ",
		"            i j k 7 l m 9 0 a n o p q r s t u < | v             ",
		"            w k > l x y z A B C p q D E F G H I J K             ",
		"            L M N O y P Q R S T U V W F G X Y Z ` K             ",
		"             ...+.@.#.$.%.&.*.=.-.;.>.,.'.).Z !.~.{.            ",
		"            ].^./.(.[ c _._ :.<.[.$ ' /.}.|.!.1.2.3.            ",
		"            4.5.6.7.8.9.# 0.a.b.c.d.e.f.g.g.h.` i.j.            ",
		"            k.9 l.m.n.o.p.q.r.s.t.u.v.w.g.h.x.~.y.z.            ",
		"            A.0 a B.C.D.E.F.G.H.I.J.K.L.M.N.O.2.P.Q.            ",
		"            R.S.n o p q T.E U.V.W.X.Y.Z.`. +.+++@+#+            ",
		"            $+%+&+q *+=+E F G -+I Z ;+>+,+'+)+!+~+$+            ",
		"              {+]+^+w /+H (+X _+Z !.:+<+[+}+|+P.1+'             ",
		"            k 2+_ > 3+z.4+5+6+7+x.~.8+9+0+a+b+c+1+3             ",
		"        %+..d+e+..f+< g+h+i+j+k+l+m+n+o+P.p+q+p+1+r+            ",
		"    s+t+u+< (.< v+y 9 (.w+x+y+z+y+h+A+B+C+K ].D+1+h             ",
		"    E+i+F+f.j.G+H+9 [ (.z I+J+m+f.j.K+z 9 9 9 K+L+r+/.9 (.      ",
		"    L M+N+O+u+P+Q+R+S+T+U+y 8 - ;...9 9 9 9 9 9 9 9 (.(.k w+    ",
		"    V+m+' W+r+] , X+Y+(.: r+L P+k 9 z (.9 9 9 9 (.(.Z+;.- `+    ",
		"    ].C+w  @u+.@+@@@#@$@j %@B+&@#@L $@H+2+/.0 (.*@+.} 2+=@-@    ",
		"      ;@| >@,@'@u+k 8 )@..!@| ~@V+#@#@#@#@L 6+..(.9 {@.._ (     ",
		"              e ]@^@] /@k G+w #@#@#@#@#@V+ @$@_ 9 9 9 /.Y+(@    ",
		"                      - R.T+L ~@#@#@#@#@]._ _@_ 9 9 9 (.9 x     ",
		"                              =@_@O+L ~@#@~@L _ 9 9 :@          ",
		"                                      ;.H+ @-._ (.              ",
		"                                                                ",
		"                                                                "};
		"""

def FSMesh(obj, recompute=False):
	""" Get free surface mesh in matrix mode.
	@param obj Created Part::FeaturePython object.
	@param recompute True if mesh must be recomputed, False otherwise.
	@return Faces matrix
	"""
	nx = obj.FS_Nx
	ny = obj.FS_Ny
	if not recompute:
		faces = []
		for i in range(0,nx):
			faces.append([])
			for j in range(0,ny):
				faces[i].append(FreeSurfaceFace(obj.FS_Position[j + i*ny],
												obj.FS_Normal[j + i*ny],
												obj.FS_Area[j + i*ny]))
		return faces
	# Transform positions into a mesh
	pos = []
	for i in range(0,nx):
		pos.append([])
		for j in range(0,ny):
			pos[i].append(obj.FS_Position[j + i*ny])
	# Recompute normals and dimensions
	normal = []
	l	  = []
	b	  = []
	for i in range(0,nx):
		normal.append([])
		l.append([])
		b.append([])
		for j in range(0,ny):
			i0 = i-1
			i1 = i+1
			fi = 1.0
			j0 = j-1
			j1 = j+1
			fj = 1.0
			if i == 0:
				i0 = i
				i1 = i+1
				fi = 2.0
			if i == nx-1:
				i0 = i-1
				i1 = i
				fi = 2.0
			if j == 0:
				j0 = j
				j1 = j+1
				fj = 2.0
			if j == ny-1:
				j0 = j-1
				j1 = j
				fj = 2.0
			l[i].append(fi*(obj.FS_Position[j + i1*ny].x - obj.FS_Position[j + i0*ny].x))
			b[i].append(fj*(obj.FS_Position[j1 + i*ny].y - obj.FS_Position[j0 + i*ny].y))
			xvec = Vector(obj.FS_Position[j + i1*ny].x - obj.FS_Position[j + i0*ny].x,
						  obj.FS_Position[j + i1*ny].y - obj.FS_Position[j + i0*ny].y,
						  obj.FS_Position[j + i1*ny].z - obj.FS_Position[j + i0*ny].z)
			yvec = Vector(obj.FS_Position[j1 + i*ny].x - obj.FS_Position[j0 + i*ny].x,
						  obj.FS_Position[j1 + i*ny].y - obj.FS_Position[j0 + i*ny].y,
						  obj.FS_Position[j1 + i*ny].z - obj.FS_Position[j0 + i*ny].z)
			n = Vector(xvec.cross(yvec))	# Z positive
			normal[i].append(n.normalize())
	# Create faces
	faces = []
	for i in range(0,nx):
		faces.append([])
		for j in range(0,ny):
			faces[i].append(FreeSurfaceFace(pos[i][j], normal[i][j], l[i][j], b[i][j]))
	# Reconstruct mesh data
	for i in range(0,nx):
		for j in range(0,ny):
			obj.FS_Position[j + i*ny] = faces[i][j].pos
			obj.FS_Normal[j + i*ny]   = faces[i][j].normal
			obj.FS_Area[j + i*ny]	 = faces[i][j].area
	return faces

