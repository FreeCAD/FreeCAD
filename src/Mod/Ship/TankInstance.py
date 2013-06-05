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

import time

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

class ShipTank:
	def __init__(self, obj, solid, level=0, density=998.0):
		""" Creates a new tank on active document.
		@param obj Created Part::FeaturePython object.
		@param solid Solid shape that represent the tank.
		@param level Tank filling level.
		@param density Fluid density.
		"""
		# Add uniqueness property to identify Tank instances
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship tank instance",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyBool","IsShipTank","ShipTank", tooltip).IsShipTank=True
		# Add general options
		tooltip = str(QtGui.QApplication.translate("Ship","Fluid filling level percentage",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloat","Level","ShipTank", tooltip).Level=level
		tooltip = str(QtGui.QApplication.translate("Ship","Inside fluid density",
												   None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloat","Density","ShipTank", tooltip).Density=density
		# Add shapes
		shape = self.computeShape(solid)
		if not shape:
			obj.IsShipTank=False
			return
		obj.Shape = shape
		obj.Proxy = self

	def onChanged(self, fp, prop):
		""" Property changed, tank must be recomputed """
		if prop == "IsShipTank":
			FreeCAD.Console.PrintWarning("Ussually you don't want to modify manually this option.\n")
		elif prop == "Level":
			if fp.Level > 100.0:
				fp.Level = 100.0
			elif fp.Level < 0.0:
				fp.Level = 0.0

	def execute(self, obj):
		""" Shape recomputation called """
		obj.Shape = self.computeShape(obj.Shape)

	def computeShape(self, solid):
		""" Create faces shape. This method also calls to generate boxes.
		@param solid Solid shape that represent the tank.
		@return Computed solid shape. None if can't build it.
		"""
		# Study input to try to build a solid
		if solid.isDerivedFrom('Part::Feature'):
			# Get shape
			shape = solid.Shape
			if not shape:
				return None
			solid = shape
		if not solid.isDerivedFrom('Part::TopoShape'):
			return None
		# Get shells
		shells = solid.Shells
		if not shells:
			return None
		# Build solids
		solids = []
		for s in shells:
			solid = Part.Solid(s)
			if solid.Volume < 0.0:
				solid.reverse()
			solids.append(solid)
		# Create compound
		shape = Part.CompSolid(solids)
		return shape

class ViewProviderShipTank:
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
		return "Shaded"

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
		static char * TankS_xpm[] = {
		"32 32 516 2",
		"  	c None",
		". 	c #2A2A2A",
		"+ 	c #6D6C6D",
		"@ 	c #434343",
		"# 	c #000000",
		"$ 	c #3C3C3C",
		"% 	c #878787",
		"& 	c #DADADA",
		"* 	c #D1D2D1",
		"= 	c #DBDBDC",
		"- 	c #DDDDDD",
		"; 	c #777677",
		"> 	c #636363",
		", 	c #3D3D3D",
		"' 	c #D2D2D1",
		") 	c #CBCBCB",
		"! 	c #CECECE",
		"~ 	c #D2D2D2",
		"{ 	c #D5D5D5",
		"] 	c #D8D9D9",
		"^ 	c #DCDCDC",
		"/ 	c #E6E6E6",
		"( 	c #ACADAC",
		"_ 	c #949494",
		": 	c #2B2A2B",
		"< 	c #202020",
		"[ 	c #838383",
		"} 	c #D0D0D1",
		"| 	c #C4C3C3",
		"1 	c #C7C7C7",
		"2 	c #CACACA",
		"3 	c #D1D1D1",
		"4 	c #D5D4D5",
		"5 	c #D8D8D8",
		"6 	c #DEDEDE",
		"7 	c #E2E2E2",
		"8 	c #E5E5E5",
		"9 	c #F5F5F6",
		"0 	c #B5B5B5",
		"a 	c #747474",
		"b 	c #393939",
		"c 	c #7E7E7E",
		"d 	c #7B7B7B",
		"e 	c #7C7C7C",
		"f 	c #D3D3D3",
		"g 	c #CCCCCC",
		"h 	c #CDCDCD",
		"i 	c #D4D4D4",
		"j 	c #DBDBDA",
		"k 	c #DEDFDF",
		"l 	c #E2E2E1",
		"m 	c #E5E4E5",
		"n 	c #E9E8E8",
		"o 	c #ECECEC",
		"p 	c #F2F2F1",
		"q 	c #FEFEFE",
		"r 	c #989898",
		"s 	c #737373",
		"t 	c #ADADAD",
		"u 	c #AAAAAA",
		"v 	c #5A5A5A",
		"w 	c #717171",
		"x 	c #9D9D9D",
		"y 	c #D7D7D7",
		"z 	c #DBDBDB",
		"A 	c #DEDDDD",
		"B 	c #E1E1E1",
		"C 	c #E4E5E4",
		"D 	c #E8E8E8",
		"E 	c #EBEBEC",
		"F 	c #EFEFEF",
		"G 	c #F1F1F1",
		"H 	c #F0F0F0",
		"I 	c #FFFFFF",
		"J 	c #666666",
		"K 	c #5E5E5E",
		"L 	c #4D4D4D",
		"M 	c #D8D7D8",
		"N 	c #D7D7D6",
		"O 	c #E3E3E3",
		"P 	c #797979",
		"Q 	c #656565",
		"R 	c #A9AAAA",
		"S 	c #707070",
		"T 	c #949493",
		"U 	c #C3C3C3",
		"V 	c #E0E0E0",
		"W 	c #E1E0E0",
		"X 	c #E4E4E4",
		"Y 	c #E7E7E7",
		"Z 	c #EBEBEB",
		"` 	c #EEEDED",
		" .	c #585858",
		"..	c #5B5B5B",
		"+.	c #C3C4C3",
		"@.	c #D3D3D2",
		"#.	c #D3D3D4",
		"$.	c #D5D5D4",
		"%.	c #DFDFDF",
		"&.	c #474747",
		"*.	c #858585",
		"=.	c #505050",
		"-.	c #606060",
		";.	c #848485",
		">.	c #F4F4F5",
		",.	c #6F6F6F",
		"'.	c #909090",
		").	c #D9DAD9",
		"!.	c #D4D3D4",
		"~.	c #D6D6D5",
		"{.	c #525252",
		"].	c #ADACAD",
		"^.	c #323232",
		"/.	c #ABABAA",
		"(.	c #D0CFCF",
		"_.	c #D1D0D1",
		":.	c #D1D2D2",
		"<.	c #D3D4D3",
		"[.	c #D5D5D6",
		"}.	c #D6D6D6",
		"|.	c #1B1B1B",
		"1.	c #494949",
		"2.	c #919191",
		"3.	c #CFCFCF",
		"4.	c #D2D3D2",
		"5.	c #D4D5D4",
		"6.	c #848384",
		"7.	c #B2B2B2",
		"8.	c #B3B2B2",
		"9.	c #A7A7A7",
		"0.	c #444344",
		"a.	c #CCCFD1",
		"b.	c #ABC2D5",
		"c.	c #94BBD9",
		"d.	c #A9C4D7",
		"e.	c #C1CFD8",
		"f.	c #D0D1D1",
		"g.	c #2D2D2D",
		"h.	c #CDCECD",
		"i.	c #C5C5C5",
		"j.	c #C9C9C9",
		"k.	c #CBCBCA",
		"l.	c #CDCECE",
		"m.	c #CFCFD0",
		"n.	c #818181",
		"o.	c #B0B0B0",
		"p.	c #B1B1B1",
		"q.	c #9FA4AA",
		"r.	c #4D6782",
		"s.	c #557CA0",
		"t.	c #90B7DB",
		"u.	c #8DB5D7",
		"v.	c #8FB7D7",
		"w.	c #91BAD9",
		"x.	c #93BCD9",
		"y.	c #BCD3E1",
		"z.	c #747C81",
		"A.	c #BDBCBC",
		"B.	c #BEBEBE",
		"C.	c #C0C0C0",
		"D.	c #C2C2C2",
		"E.	c #C4C4C4",
		"F.	c #C6C6C6",
		"G.	c #C7C8C7",
		"H.	c #CAC9C9",
		"I.	c #7D7D7E",
		"J.	c #828281",
		"K.	c #AEAEAD",
		"L.	c #ADADAE",
		"M.	c #A6AAB1",
		"N.	c #7C9ABF",
		"O.	c #7699C2",
		"P.	c #769BC3",
		"Q.	c #789DC3",
		"R.	c #6288AE",
		"S.	c #4F7799",
		"T.	c #5981A3",
		"U.	c #94BDDC",
		"V.	c #8FB9D8",
		"W.	c #537E9A",
		"X.	c #87B2CE",
		"Y.	c #8AB0C7",
		"Z.	c #A8B6BE",
		"`.	c #B7BABB",
		" +	c #BDBDBD",
		".+	c #BFBFBF",
		"++	c #C1C1C1",
		"@+	c #C2C3C3",
		"#+	c #C4C4C5",
		"$+	c #C7C7C6",
		"%+	c #7A7A7B",
		"&+	c #7F8080",
		"*+	c #A8AAAB",
		"=+	c #9EA6AF",
		"-+	c #7594BE",
		";+	c #7192C0",
		">+	c #7293C0",
		",+	c #7395C1",
		"'+	c #7498C1",
		")+	c #7599C1",
		"!+	c #779CC2",
		"~+	c #779EC3",
		"{+	c #769DC0",
		"]+	c #638BAD",
		"^+	c #4F7897",
		"/+	c #4F7D9F",
		"(+	c #7DA7C4",
		"_+	c #7FAAC6",
		":+	c #80ADC7",
		"<+	c #83B0C8",
		"[+	c #8EB4C6",
		"}+	c #B5B9BB",
		"|+	c #BABBBC",
		"1+	c #787878",
		"2+	c #717B89",
		"3+	c #6F90BD",
		"4+	c #6F91BD",
		"5+	c #6F90BE",
		"6+	c #7091BE",
		"7+	c #7192BF",
		"8+	c #7194BF",
		"9+	c #7396C0",
		"0+	c #7498C0",
		"a+	c #759AC1",
		"b+	c #769CC1",
		"c+	c #789FC2",
		"d+	c #79A1C3",
		"e+	c #4A7CA2",
		"f+	c #7AA3C2",
		"g+	c #7BA5C2",
		"h+	c #7DA8C4",
		"i+	c #7FABC5",
		"j+	c #81AEC6",
		"k+	c #83B1C8",
		"l+	c #85B4C9",
		"m+	c #98B7C4",
		"n+	c #BABABB",
		"o+	c #747576",
		"p+	c #5B71A4",
		"q+	c #6D89BE",
		"r+	c #678BBF",
		"s+	c #6D8EBD",
		"t+	c #6D8FBD",
		"u+	c #6E90BD",
		"v+	c #7091BD",
		"w+	c #7193BE",
		"x+	c #7296BF",
		"y+	c #7397C0",
		"z+	c #7499C0",
		"A+	c #759BC1",
		"B+	c #779EC1",
		"C+	c #4979A1",
		"D+	c #769FBF",
		"E+	c #78A0C0",
		"F+	c #79A4C1",
		"G+	c #7BA6C2",
		"H+	c #7DAAC3",
		"I+	c #80ACC5",
		"J+	c #82B0C6",
		"K+	c #84B2C7",
		"L+	c #89B4C6",
		"M+	c #95AFC1",
		"N+	c #59819C",
		"O+	c #5A6FA3",
		"P+	c #7388BC",
		"Q+	c #7489BC",
		"R+	c #7189BD",
		"S+	c #6B89BE",
		"T+	c #678BBE",
		"U+	c #6D8EBC",
		"V+	c #6E8FBC",
		"W+	c #6F92BE",
		"X+	c #7194BE",
		"Y+	c #7398BF",
		"Z+	c #759BBF",
		"`+	c #46759F",
		" @	c #729ABC",
		".@	c #749CBD",
		"+@	c #769FBE",
		"@@	c #77A2BF",
		"#@	c #79A5C0",
		"$@	c #7BA7C2",
		"%@	c #7CA9C2",
		"&@	c #7AA5C3",
		"*@	c #7CA6C4",
		"=@	c #7DA7C5",
		"-@	c #577E98",
		";@	c #586CA2",
		">@	c #7184BA",
		",@	c #7185BB",
		"'@	c #7286BB",
		")@	c #7287BB",
		"!@	c #7388BB",
		"~@	c #6C86BD",
		"{@	c #6486BF",
		"]@	c #6B8DBB",
		"^@	c #6D8FBB",
		"/@	c #6E91BC",
		"(@	c #7093BC",
		"_@	c #7195BD",
		":@	c #7297BD",
		"<@	c #43719D",
		"[@	c #6E95B8",
		"}@	c #7098BA",
		"|@	c #729BBB",
		"1@	c #749DBC",
		"2@	c #799EB9",
		"3@	c #719CBF",
		"4@	c #769EBF",
		"5@	c #77A0C0",
		"6@	c #78A2C1",
		"7@	c #7AA3C3",
		"8@	c #557B96",
		"9@	c #576AA1",
		"0@	c #6F82B9",
		"a@	c #7083BA",
		"b@	c #7084BA",
		"c@	c #7185BA",
		"d@	c #7285BB",
		"e@	c #7187BC",
		"f@	c #6985BF",
		"g@	c #6387C0",
		"h@	c #6E90BC",
		"i@	c #6F92BB",
		"j@	c #7095BC",
		"k@	c #416D9B",
		"l@	c #6A90B5",
		"m@	c #6A92B7",
		"n@	c #6393BD",
		"o@	c #6D95B9",
		"p@	c #7097BA",
		"q@	c #7199BA",
		"r@	c #729ABB",
		"s@	c #739BBD",
		"t@	c #759DBE",
		"u@	c #779FBE",
		"v@	c #527794",
		"w@	c #5567A0",
		"x@	c #6D7EB7",
		"y@	c #6D7FB7",
		"z@	c #6E81B8",
		"A@	c #6F82B8",
		"B@	c #6F83B9",
		"C@	c #7186BA",
		"D@	c #6F85BB",
		"E@	c #6183C2",
		"F@	c #698DBC",
		"G@	c #3D6A9A",
		"H@	c #5E8BB7",
		"I@	c #688EB3",
		"J@	c #6A90B4",
		"K@	c #6B91B5",
		"L@	c #6C93B6",
		"M@	c #6F97B8",
		"N@	c #7199BB",
		"O@	c #739BBC",
		"P@	c #507492",
		"Q@	c #52639E",
		"R@	c #6A7BB5",
		"S@	c #6A7CB6",
		"T@	c #6B7DB6",
		"U@	c #6C7EB7",
		"V@	c #6D7FB8",
		"W@	c #6D80B7",
		"X@	c #6F83BA",
		"Y@	c #436587",
		"Z@	c #6388AF",
		"`@	c #658AB0",
		" #	c #668CB1",
		".#	c #688DB3",
		"+#	c #698FB4",
		"@#	c #6C92B6",
		"##	c #6D94B7",
		"$#	c #6F96B9",
		"%#	c #4E7190",
		"&#	c #50619D",
		"*#	c #6778B4",
		"=#	c #6878B4",
		"-#	c #6979B4",
		";#	c #697BB5",
		">#	c #6A7CB7",
		",#	c #6A7DB7",
		"'#	c #6B7EB7",
		")#	c #6C7FB7",
		"!#	c #6E80B8",
		"~#	c #416285",
		"{#	c #6084AC",
		"]#	c #6186AD",
		"^#	c #6389B0",
		"/#	c #668BB0",
		"(#	c #678CB1",
		"_#	c #6B91B4",
		":#	c #4B6E8E",
		"<#	c #4F5F9D",
		"[#	c #6575B2",
		"}#	c #6675B2",
		"|#	c #6676B3",
		"1#	c #6776B3",
		"2#	c #6777B4",
		"3#	c #6879B4",
		"4#	c #697AB5",
		"5#	c #6A7DB6",
		"6#	c #6B7DB7",
		"7#	c #6C7EB6",
		"8#	c #3E5E83",
		"9#	c #5C80A9",
		"0#	c #5D81AA",
		"a#	c #5F83AB",
		"b#	c #6085AC",
		"c#	c #6388AE",
		"d#	c #648AAF",
		"e#	c #668BB1",
		"f#	c #678CB2",
		"g#	c #688EB4",
		"h#	c #486B8C",
		"i#	c #233077",
		"j#	c #314293",
		"k#	c #6574B5",
		"l#	c #6573B2",
		"m#	c #6473B2",
		"n#	c #6575B3",
		"o#	c #6676B2",
		"p#	c #697AB4",
		"q#	c #3C5B81",
		"r#	c #587CA5",
		"s#	c #5A7DA7",
		"t#	c #5B7FA8",
		"u#	c #5D80A9",
		"v#	c #5D82AA",
		"w#	c #6185AD",
		"x#	c #6287AE",
		"y#	c #6389AF",
		"z#	c #678CB3",
		"A#	c #385C80",
		"B#	c #2A3576",
		"C#	c #35418C",
		"D#	c #253584",
		"E#	c #4F5FA4",
		"F#	c #6574B3",
		"G#	c #6372B1",
		"H#	c #6473B1",
		"I#	c #6474B2",
		"J#	c #6576B3",
		"K#	c #6777B3",
		"L#	c #3A587F",
		"M#	c #5578A3",
		"N#	c #5779A3",
		"O#	c #577BA5",
		"P#	c #597CA6",
		"Q#	c #5B7EA7",
		"R#	c #5C7FA8",
		"S#	c #5E82AA",
		"T#	c #4D7299",
		"U#	c #0C1A71",
		"V#	c #1E2A74",
		"W#	c #38448D",
		"X#	c #2E3D90",
		"Y#	c #33428C",
		"Z#	c #5D6AAC",
		"`#	c #6371B2",
		" $	c #6270B0",
		".$	c #6372B0",
		"+$	c #37557D",
		"@$	c #52739F",
		"#$	c #5274A1",
		"$$	c #5577A3",
		"%$	c #577AA4",
		"&$	c #587CA6",
		"*$	c #597DA6",
		"=$	c #5C7FA9",
		"-$	c #5478A1",
		";$	c #21466F",
		">$	c #111F72",
		",$	c #2B377D",
		"'$	c #34428F",
		")$	c #303F8D",
		"!$	c #455293",
		"~$	c #6572B3",
		"{$	c #616FB0",
		"]$	c #626FB0",
		"^$	c #34537B",
		"/$	c #4E709D",
		"($	c #50719E",
		"_$	c #50729F",
		":$	c #5174A0",
		"<$	c #5375A1",
		"[$	c #577AA5",
		"}$	c #3F618C",
		"|$	c #2F5786",
		"1$	c #0C315B",
		"2$	c #182678",
		"3$	c #1C2872",
		"4$	c #354087",
		"5$	c #2C3A88",
		"6$	c #36448A",
		"7$	c #5562A4",
		"8$	c #6370B1",
		"9$	c #34527A",
		"0$	c #4C6D9B",
		"a$	c #4D6E9C",
		"b$	c #51739F",
		"c$	c #385A87",
		"d$	c #254672",
		"e$	c #1F2A6F",
		"f$	c #2A367A",
		"g$	c #334088",
		"h$	c #33428D",
		"i$	c #254673",
		"j$	c #4F709E",
		"k$	c #375886",
		"l$	c #224473",
		"m$	c #09155A",
		"n$	c #2A3577",
		"o$	c #1E275A",
		"p$	c #244676",
		"q$	c #1F4373",
		"                                                                ",
		"                      . + @ #                                   ",
		"                  $ % & * = - ; >                               ",
		"              , % ' ) ! ~ { ] ^ / ( _ :                         ",
		"          < [ } | 1 2 ! 3 4 5 = 6 7 8 9 0 a #                   ",
		"        b c d e e f g h 3 i 5 j k l m n o p q r s #             ",
		"      # t 7 u v _ w w x & f y z A B C D E F G H I J K           ",
		"      L M f i N O P Q R S T U V 6 W X Y Z ` I d _ - V  .        ",
		"      ..+.3 3 @.#.$.5 %.&.*.=.-.;.8 Z Y >.,.'.).!.~.5 Y {.      ",
		"      % ].^./.(._.:.<.!.[.}.V |.# # 1.Q 2.} g h 3._.4.5.6.      ",
		"      *.7.8.t 9.0.N ' a.b.c.d.e.f.'.g.h.U i.1 j.k.g l.m.n.      ",
		"      [ o.o.p.p.7.q.r.s.t.u.v.w.x.y.z.A.B.C.D.E.F.G.H.k.I.      ",
		"      J.t K.L.M.N.O.P.Q.R.S.T.U.V.W.X.Y.Z.`. +.+++@+#+$+%+      ",
		"      &+*+=+-+;+>+,+'+)+!+~+{+]+^+/+(+_+:+<+[+}+|+B.C.++1+      ",
		"      2+3+4+5+6+6+7+8+9+0+a+b+c+d+e+f+g+h+i+j+k+l+m+n+A.o+      ",
		"      p+q+r+s+t+u+u+v+w+x+y+z+A+B+C+D+E+F+G+H+I+J+K+L+M+N+      ",
		"      O+P+Q+R+S+T+U+V+3+W+X+x+Y+Z+`+ @.@+@@@#@$@%@&@*@=@-@      ",
		"      ;@>@,@'@)@!@~@{@]@^@/@(@_@:@<@[@}@|@1@2@3@4@5@6@7@8@      ",
		"      9@0@a@b@c@d@'@'@e@f@g@h@i@j@k@l@m@n@o@p@q@r@s@t@u@v@      ",
		"      w@x@y@z@z@A@B@b@b@,@C@D@E@F@G@H@I@J@K@L@[@M@p@N@O@P@      ",
		"      Q@R@S@T@U@U@V@W@z@z@0@X@b@c@Y@Z@`@ #.#+#J@@###[@$#%#      ",
		"      &#*#=#-#;#R@>#,#'#U@)#W@!#z@~#{#]#R.^#/#(#I@+#_#@#:#      ",
		"      <#[#}#|#1#2#=#3#4#;#S@5#6#7#8#9#0#a#b#]#c#d#e#f#g#h#      ",
		"      i#j#k#l#m#[#n#o#|#2#*#3#p#4#q#r#s#t#u#v#a#w#x#y#z#A#      ",
		"        B#C#D#E#F#G#G#H#I#[#J#|#K#L#M#N#O#P#Q#R#0#S#{#T#        ",
		"          U#V#W#X#Y#Z#`# $.$G#H#I#+$@$#$$$M#%$&$*$=$-$;$        ",
		"                >$,$'$)$!$~${$]$ $^$/$($_$:$<$[$}$|$1$          ",
		"                    2$3$4$5$6$7$8$9$0$a$a$b$c$d$                ",
		"                          e$f$g$h$i$_$j$k$l$                    ",
		"                              m$n$o$p$q$                        ",
		"                                                                ",
		"                                                                "};
		"""

def tankWeight(obj, angles=Vector(0.0,0.0,0.0), cor=Vector(0.0,0.0,0.0)):
	""" Compute tank fluid weight and their center of gravity.
	@param obj Tank object.
	@param angles Tank angles, Roll, Pitch and Yaw.
	@param cor Center or rotation.
	@return Weight and center of gravity. None if errors detected
	"""
	# Test if is a tank instance
	props = obj.PropertiesList
	try:
		props.index("IsShipTank")
	except ValueError:
		return None
	if not obj.IsShipTank:
		return None
	# Get object solids
	Solids = obj.Shape.Solids
	W = [0.0, 0.0, 0.0, 0.0]
	for s in Solids:
		# Get fluid volume
		bbox  = s.BoundBox
		z0	= bbox.ZMin
		z1	= bbox.ZMax
		dz	= obj.Level/100.0 * (z1-z0)
		z	 = z0 + dz
		dx	= bbox.XMax-bbox.XMin
		dy	= bbox.YMax-bbox.YMin
		try:
			box   = Part.makeBox(3.0*(dx), 3.0*(dy), (z1-z0)+dz, Vector(bbox.XMin-dx, bbox.YMin-dy, bbox.ZMin-(z1-z0)))
			fluid = s.common(box)
			vol   = fluid.Volume		
		except:
			vol   = 0.0
		W[0]  = W[0] + vol*obj.Density
		# Compute fluid solid in rotated position (non linear rotation
		# are ussually computed as Roll -> Pitch -> Yaw).
		s.rotate(cor, Vector(1.0,0.0,0.0), angles.x)
		s.rotate(cor, Vector(0.0,1.0,0.0), angles.y)
		s.rotate(cor, Vector(0.0,0.0,1.0), angles.z)
		bbox  = s.BoundBox
		z0	= bbox.ZMin
		z1	= bbox.ZMax
		dx	= bbox.XMax-bbox.XMin
		dy	= bbox.YMax-bbox.YMin
		Error = 0.01*vol
		z	 = 0.0
		v	 = 0.0
		while(abs(vol - v) > Error):
			z  = z + (vol - v) / (dx*dy)
			dz = z - z0
			try:
				box   = Part.makeBox(3.0*(dx), 3.0*(dy), (z1-z0)+dz, Vector(bbox.XMin-dx, bbox.YMin-dy, bbox.ZMin-(z1-z0)))
				fluid = s.common(box)
				v	  = fluid.Volume
			except:
				v     = 0.0
			if(abs(vol - v) / (dx*dy) <= 0.000001):
				break
		# Add fluid moments
		for f in fluid.Solids:
			cog  = f.CenterOfMass
			W[1] = W[1] + f.Volume*obj.Density*cog.x
			W[2] = W[2] + f.Volume*obj.Density*cog.y
			W[3] = W[3] + f.Volume*obj.Density*cog.z
	return [W[0], W[1]/W[0], W[2]/W[0], W[3]/W[0]]
