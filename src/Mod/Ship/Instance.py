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
from math import *

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

class Ship:
	def __init__(self, obj, solids):
		""" Creates a new ship on active document.
		@param obj Part::FeaturePython created object.
		@param faces Ship solids components.
		"""
		# Add uniqueness property to identify Ship instances
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship instance",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyBool","IsShip","Ship", tooltip).IsShip=True
		# Add main dimensions
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship instance",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyLength","Length","Ship", tooltip).Length=0.0
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship instance",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyLength","Beam","Ship", tooltip).Beam=0.0
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship instance",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyLength","Draft","Ship", tooltip).Draft=0.0
		# Add shapes
		obj.Shape = Part.makeCompound(solids)
		tooltip = str(QtGui.QApplication.translate("Ship","True if is a valid ship instance",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("Part::PropertyPartShape","ExternalFaces","Ship", tooltip)
		obj.Proxy = self

	def onChanged(self, fp, prop):
		""" Called when a ship property is modified 
		@param fp Part::FeaturePython object.
		@param prop Property changed.
		"""
		if prop == "Length" or prop == "Beam" or prop == "Draft":
			pass

	def execute(self, fp):
		""" Called when a recomputation is needed. 
		@param fp Part::FeaturePython object.
		"""
		fp.Shape = Part.makeCompound(fp.Shape.Solids)

class ViewProviderShip:
	def __init__(self, obj):
		"Set this object to the proxy object of the actual view provider"
		obj.Proxy = self

	def attach(self, obj):
		''' Setup the scene sub-graph of the view provider, this method is mandatory '''
		return

	def updateData(self, fp, prop):
		''' If a property of the handled feature has changed we have the chance to handle this here '''
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
		static char * Ship_xpm[] = {
		"32 32 396 2",
		"  	c None",
		". 	c #2C2C2C",
		"+ 	c #3A3A3A",
		"@ 	c #585857",
		"# 	c #161616",
		"$ 	c #000000",
		"% 	c #363636",
		"& 	c #333333",
		"* 	c #B3B3B3",
		"= 	c #B4B4B4",
		"- 	c #949494",
		"; 	c #565653",
		"> 	c #141414",
		", 	c #080807",
		"' 	c #585858",
		") 	c #878787",
		"! 	c #9F9E9F",
		"~ 	c #9F9F9E",
		"{ 	c #8F8F90",
		"] 	c #6B6B6B",
		"^ 	c #101010",
		"/ 	c #737373",
		"( 	c #4C4C4C",
		"_ 	c #B1B1B7",
		": 	c #9090C0",
		"< 	c #A7A7B2",
		"[ 	c #87878E",
		"} 	c #4F4F52",
		"| 	c #191919",
		"1 	c #656565",
		"2 	c #D1D1D2",
		"3 	c #D1D1D1",
		"4 	c #CECECE",
		"5 	c #CDCCCC",
		"6 	c #CCCCCC",
		"7 	c #CCCCCB",
		"8 	c #CDCECD",
		"9 	c #BDBDBD",
		"0 	c #424242",
		"a 	c #373737",
		"b 	c #0A0A0A",
		"c 	c #241414",
		"d 	c #0E0C0C",
		"e 	c #929393",
		"f 	c #383738",
		"g 	c #9B9B9A",
		"h 	c #A0A0AF",
		"i 	c #2929E4",
		"j 	c #2525E5",
		"k 	c #3F3FD7",
		"l 	c #5B5BC8",
		"m 	c #535368",
		"n 	c #686866",
		"o 	c #C8C8C8",
		"p 	c #C8C8C7",
		"q 	c #C7C6C7",
		"r 	c #C6C6C6",
		"s 	c #C5C5C5",
		"t 	c #C4C5C5",
		"u 	c #C3C4C3",
		"v 	c #C3C3C2",
		"w 	c #BCBCBC",
		"x 	c #595959",
		"y 	c #A6A6A6",
		"z 	c #969696",
		"A 	c #0B0B0B",
		"B 	c #0D0707",
		"C 	c #894646",
		"D 	c #1C1A1A",
		"E 	c #525252",
		"F 	c #6C6D6C",
		"G 	c #A3A3A2",
		"H 	c #A3A296",
		"I 	c #8E8F98",
		"J 	c #6F6EA5",
		"K 	c #5354AF",
		"L 	c #373753",
		"M 	c #8D8D8B",
		"N 	c #C5C5C4",
		"O 	c #C2C2C2",
		"P 	c #C1C1C1",
		"Q 	c #C0C0C0",
		"R 	c #C0BFBF",
		"S 	c #BFBFBF",
		"T 	c #BEBEBE",
		"U 	c #B1B2B2",
		"V 	c #404040",
		"W 	c #ABAAAA",
		"X 	c #797979",
		"Y 	c #2A1212",
		"Z 	c #662828",
		"` 	c #3D403F",
		" .	c #B5B5B5",
		"..	c #6B6A6B",
		"+.	c #4A4A4A",
		"@.	c #9A9A9A",
		"#.	c #909090",
		"$.	c #8B8B8A",
		"%.	c #898A86",
		"&.	c #84837F",
		"*.	c #3D3D3C",
		"=.	c #9E9E9E",
		"-.	c #BFBFBE",
		";.	c #BDBEBD",
		">.	c #BBBBBB",
		",.	c #BABABA",
		"'.	c #B9B9B9",
		").	c #B8B8B8",
		"!.	c #999999",
		"~.	c #BABAB9",
		"{.	c #ABABAB",
		"].	c #292929",
		"^.	c #381212",
		"/.	c #4C1514",
		"(.	c #535656",
		"_.	c #717171",
		":.	c #919090",
		"<.	c #818181",
		"[.	c #4E4E4E",
		"}.	c #4B4B4B",
		"|.	c #B1B1B1",
		"1.	c #B8B7B8",
		"2.	c #B6B6B6",
		"3.	c #B6B5B5",
		"4.	c #B4B5B4",
		"5.	c #B2B3B2",
		"6.	c #5C5D5C",
		"7.	c #AFAFAF",
		"8.	c #ADACAC",
		"9.	c #5B5B5B",
		"0.	c #410C0C",
		"a.	c #3E0707",
		"b.	c #525555",
		"c.	c #9C9C9C",
		"d.	c #2D2D2D",
		"e.	c #757575",
		"f.	c #474747",
		"g.	c #484848",
		"h.	c #9F9F9F",
		"i.	c #B3B3B4",
		"j.	c #B2B2B2",
		"k.	c #B0B0B0",
		"l.	c #ADAEAD",
		"m.	c #ADADAD",
		"n.	c #B0B1B0",
		"o.	c #1E1E1E",
		"p.	c #ACABAC",
		"q.	c #AAA9A9",
		"r.	c #A8A8A8",
		"s.	c #5D5D5D",
		"t.	c #290202",
		"u.	c #281010",
		"v.	c #272828",
		"w.	c #767777",
		"x.	c #505050",
		"y.	c #1F1F1F",
		"z.	c #5E5E5D",
		"A.	c #A4A5A5",
		"B.	c #B1B2B1",
		"C.	c #AEAEAE",
		"D.	c #AEADAD",
		"E.	c #ABACAC",
		"F.	c #AAAAAA",
		"G.	c #A9A8A8",
		"H.	c #ABABAC",
		"I.	c #7B7B7B",
		"J.	c #2B2B2B",
		"K.	c #A4A4A4",
		"L.	c #A6A5A6",
		"M.	c #888888",
		"N.	c #0E0E0E",
		"O.	c #101312",
		"P.	c #7E8080",
		"Q.	c #5E5E5E",
		"R.	c #242424",
		"S.	c #555555",
		"T.	c #7F7F7F",
		"U.	c #A4A3A4",
		"V.	c #B3B3B2",
		"W.	c #ACACAC",
		"X.	c #A9A9A9",
		"Y.	c #A8A7A7",
		"Z.	c #A7A6A7",
		"`.	c #A7A7A7",
		" +	c #A8A8A7",
		".+	c #A5A5A5",
		"++	c #A2A2A2",
		"@+	c #222122",
		"#+	c #7E7E7E",
		"$+	c #A3A3A3",
		"%+	c #9B9B9B",
		"&+	c #050505",
		"*+	c #6E6E6E",
		"=+	c #A7A7A6",
		"-+	c #989898",
		";+	c #A5A4A4",
		">+	c #A7A7A8",
		",+	c #A5A6A7",
		"'+	c #979A99",
		")+	c #818383",
		"!+	c #757878",
		"~+	c #757979",
		"{+	c #878A8A",
		"]+	c #A3A5A5",
		"^+	c #828282",
		"/+	c #A0A0A0",
		"(+	c #232323",
		"_+	c #939393",
		":+	c #A5A6A5",
		"<+	c #A2A3A2",
		"[+	c #A2A1A1",
		"}+	c #A1A0A1",
		"|+	c #939292",
		"1+	c #636262",
		"2+	c #554D4D",
		"3+	c #634C4C",
		"4+	c #755555",
		"5+	c #936464",
		"6+	c #9F6868",
		"7+	c #9B6060",
		"8+	c #804A4A",
		"9+	c #5C3737",
		"0+	c #1D1616",
		"a+	c #A1A1A1",
		"b+	c #010101",
		"c+	c #151516",
		"d+	c #707070",
		"e+	c #9D9E9E",
		"f+	c #8C8D8D",
		"g+	c #8B8888",
		"h+	c #726A6A",
		"i+	c #6D5959",
		"j+	c #866261",
		"k+	c #C18B8B",
		"l+	c #D79696",
		"m+	c #D18C8C",
		"n+	c #CB8180",
		"o+	c #C57575",
		"p+	c #BF6B6A",
		"q+	c #BB6161",
		"r+	c #B95958",
		"s+	c #9C4544",
		"t+	c #2E1212",
		"u+	c #6F6C6C",
		"v+	c #A0A1A1",
		"w+	c #575757",
		"x+	c #0C0C0C",
		"y+	c #9C9D9D",
		"z+	c #7A7272",
		"A+	c #876F6F",
		"B+	c #977070",
		"C+	c #C28C8C",
		"D+	c #D59595",
		"E+	c #D08A8A",
		"F+	c #C67D7D",
		"G+	c #C07272",
		"H+	c #BC6969",
		"I+	c #B85F5F",
		"J+	c #B35656",
		"K+	c #B04C4C",
		"L+	c #AB4243",
		"M+	c #A63939",
		"N+	c #591B1B",
		"O+	c #6A2121",
		"P+	c #542323",
		"Q+	c #585A5A",
		"R+	c #191515",
		"S+	c #706262",
		"T+	c #A58080",
		"U+	c #B58383",
		"V+	c #CE8F8F",
		"W+	c #CD8989",
		"X+	c #C17372",
		"Y+	c #B45656",
		"Z+	c #AF4C4C",
		"`+	c #AB4242",
		" @	c #A73A39",
		".@	c #A3302F",
		"+@	c #9F2626",
		"@@	c #8E1A1A",
		"#@	c #2C0808",
		"$@	c #91191A",
		"%@	c #2F0200",
		"&@	c #90C6FB",
		"*@	c #8BBFFB",
		"=@	c #94CBFC",
		"-@	c #AFEFFB",
		";@	c #7DABA0",
		">@	c #3C2521",
		",@	c #C88484",
		"'@	c #C57C7D",
		")@	c #C17273",
		"!@	c #B86060",
		"~@	c #AB4343",
		"{@	c #A73939",
		"]@	c #A32F2F",
		"^@	c #9B1C1D",
		"/@	c #961313",
		"(@	c #96090A",
		"_@	c #3C0202",
		":@	c #4E0202",
		"<@	c #300000",
		"[@	c #3E5378",
		"}@	c #7EABF9",
		"|@	c #84B5FC",
		"1@	c #96CDFB",
		"2@	c #B2F2FA",
		"3@	c #C4FFFA",
		"4@	c #2E3FFD",
		"5@	c #3346FD",
		"6@	c #2A3AFD",
		"7@	c #161EFE",
		"8@	c #1B25FD",
		"9@	c #1F25B4",
		"0@	c #7C6196",
		"a@	c #AA6075",
		"b@	c #AC5763",
		"c@	c #AD5155",
		"d@	c #AD4645",
		"e@	c #A83938",
		"f@	c #A3302E",
		"g@	c #A02624",
		"h@	c #9B1C1B",
		"i@	c #971311",
		"j@	c #930A09",
		"k@	c #900300",
		"l@	c #900505",
		"m@	c #660007",
		"n@	c #00000D",
		"o@	c #200112",
		"p@	c #597F88",
		"q@	c #6E97FD",
		"r@	c #384CFD",
		"s@	c #394EFD",
		"t@	c #2D3EFD",
		"u@	c #151DFE",
		"v@	c #1821FE",
		"w@	c #3C52FD",
		"x@	c #6388FC",
		"y@	c #9CD6FB",
		"z@	c #D0FFFA",
		"A@	c #AEEEFB",
		"B@	c #749FFF",
		"C@	c #3F5DFF",
		"D@	c #4165FF",
		"E@	c #525AE3",
		"F@	c #6153C4",
		"G@	c #672D8D",
		"H@	c #6C1B6A",
		"I@	c #722164",
		"J@	c #75225E",
		"K@	c #731D57",
		"L@	c #701653",
		"M@	c #690E52",
		"N@	c #5F0050",
		"O@	c #562086",
		"P@	c #11108D",
		"Q@	c #2330BE",
		"R@	c #344AE1",
		"S@	c #4E6BFF",
		"T@	c #4E6BFD",
		"U@	c #597AFC",
		"V@	c #6184FC",
		"W@	c #7099FC",
		"X@	c #8BBEFB",
		"Y@	c #95CCFB",
		"Z@	c #5B7CFC",
		"`@	c #1C26FD",
		" #	c #121AFE",
		".#	c #9ED7FB",
		"+#	c #81B4FF",
		"@#	c #6893FF",
		"##	c #6997FF",
		"$#	c #6695FF",
		"%#	c #6390FF",
		"&#	c #618DFF",
		"*#	c #608DFF",
		"=#	c #618EFF",
		"-#	c #6391FF",
		";#	c #6898FF",
		">#	c #6B9AFF",
		",#	c #5171ED",
		"'#	c #90C4FF",
		")#	c #7EABFC",
		"!#	c #729CFC",
		"~#	c #6287FC",
		"{#	c #4761FD",
		"]#	c #070AFE",
		"^#	c #6084FC",
		"/#	c #9AD2FB",
		"(#	c #A2DDFB",
		"_#	c #8ABDFB",
		":#	c #2B3AFD",
		"<#	c #A9E8FB",
		"[#	c #B9FCFA",
		"}#	c #BAFEFA",
		"                                                                ",
		"                                                                ",
		"                                                                ",
		"                                                                ",
		"                                                                ",
		"                                                                ",
		"                                                                ",
		"        . + @ # $                   $ $ $ $                     ",
		"        % & * = - ; > $       $ , ' ) ! ~ { ] & $ $ $           ",
		"        ^ / ( = _ : < [ } | $ 1 2 3 4 5 6 7 6 8 9 0 a b         ",
		"      c d e f g h i j k l m n 4 o p q r s t u v w x y z A       ",
		"    B C D * E F G H I J K L M N O O P Q R S T 9 U V W O X $     ",
		"    Y Z `  ...+.@.#.$.%.&.*.=.-.;.w >.>.,.'.).).!.+ ~. .{.].    ",
		"    ^./.(.y _.f :.) <.[.^ }.|.).1.2.3. .4.* 5. .6.1 4.7.8.9.    ",
		"    0.a.b.c./ d.e.f.| g.h.9 i.* j.|.k.7.7.l.m.n.o.@.p.q.r.s.    ",
		"    t.u.v.w.x.y.% z.A.).B.C.D.m.E.{.F.F.G.r.H.I.J.k.K.L.M.N.    ",
		"    O.P.Q.R.S.T.U.V.W.q.X.r.Y.Z.`.Y. +`..+++{.@+#+$+++%+y.      ",
		"  &+*+W.=+-+;+X.>+y .+K.K.y y ,+'+)+!+~+{+]+^+].$+/+$+J.        ",
		"  (+_+:+U.$+<+[+}+/+h.=.|+1+2+3+4+5+6+7+8+9+0+_.a+/+0 b+        ",
		"  c+d+h.a+/+++e+f+g+h+i+j+k+l+m+n+o+p+q+r+s+t+u+v+w+$           ",
		"    x+f.- y+w.z+A+B+C+D+E+F+G+H+I+J+K+L+M+N+O+P+Q+$             ",
		"        R+S+T+U+V+W+F+X+H+I+Y+Z+`+ @.@+@@@#@$@%@$               ",
		"&@*@=@-@;@>@,@'@)@H+!@Y+K+~@{@]@+@^@/@(@_@:@<@[@}@|@1@2@3@      ",
		"4@5@6@7@8@9@0@a@b@c@d@e@f@g@h@i@j@k@l@m@n@o@p@q@r@s@t@u@v@w@x@y@",
		"      z@A@B@C@D@E@F@G@H@I@J@K@L@M@N@O@P@Q@R@S@T@U@V@W@X@Y@Z@`@ #",
		"              .#+#@###$#%#&#*#=#-#;#>#,#'#        )#!#~#{#]#^#/#",
		"                                                        (#_#:#<#",
		"                                                            [#}#",
		"                                                                ",
		"                                                                ",
		"                                                                ",
		"                                                                "};
		"""

def weights(obj):
	""" Returns Ship weights list. If weights has not been sets, 
	this tool creates it.
	@param obj Ship object
	@return Weights list. None if errors
	"""
	# Test if is a ship instance
	props = obj.PropertiesList
	try:
		props.index("IsShip")
	except ValueError:
		return None
	if not obj.IsShip:
		return None
	# Test if properties already exist
	try:
		props.index("WeightNames")
	except ValueError:
		tooltip = str(QtGui.QApplication.translate("Ship","Ship Weights names",None,QtGui.QApplication.UnicodeUTF8))
		lighweight = str(QtGui.QApplication.translate("Ship","Lightweight",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyStringList","WeightNames","Ship", tooltip).WeightNames=[lighweight]
	try:
		props.index("WeightMass")
	except ValueError:
		# Compute mass aproximation
		from shipHydrostatics import Tools
		disp = Tools.displacement(obj,obj.Draft)
		tooltip = str(QtGui.QApplication.translate("Ship","Ship Weights masses",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloatList","WeightMass","Ship", tooltip).WeightMass=[1000.0 * disp[0]]
	try:
		props.index("WeightPos")
	except ValueError:
		# Compute mass aproximation
		from shipHydrostatics import Tools
		disp = Tools.displacement(obj,obj.Draft)
		tooltip = str(QtGui.QApplication.translate("Ship","Ship Weights centers of gravity",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyVectorList","WeightPos","Ship", tooltip).WeightPos=[Vector(disp[1].x,0.0,obj.Draft)]
	# Setup list
	weights = []
	for i in range(0,len(obj.WeightNames)):
		weights.append([obj.WeightNames[i], obj.WeightMass[i], obj.WeightPos[i]])
	return weights
