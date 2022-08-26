"""
Examples for a feature class and its view provider.
(c) 2009 Werner Mayer LGPL
"""

__author__ = "Werner Mayer <wmayer@users.sourceforge.net>"

import FreeCAD, Part, math
from FreeCAD import Base
from pivy import coin

class PartFeature:
	def __init__(self, obj):
		obj.Proxy = self

class Box(PartFeature):
	def __init__(self, obj):
		PartFeature.__init__(self, obj)
		''' Add some custom properties to our box feature '''
		obj.addProperty("App::PropertyLength","Length","Box","Length of the box").Length=1.0
		obj.addProperty("App::PropertyLength","Width","Box","Width of the box").Width=1.0
		obj.addProperty("App::PropertyLength","Height","Box", "Height of the box").Height=1.0

	def onChanged(self, fp, prop):
		''' Print the name of the property that has changed '''
		FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")

	def execute(self, fp):
		''' Print a short message when doing a recomputation, this method is mandatory '''
		FreeCAD.Console.PrintMessage("Recompute Python Box feature\n")
		fp.Shape = Part.makeBox(fp.Length,fp.Width,fp.Height)

class ViewProviderBox:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
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
		Since they have the same names nothing needs to be done. This method is optional.
		'''
		return mode

	def onChanged(self, vp, prop):
		''' Print the name of the property that has changed '''
		FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")

	def getIcon(self):
		''' Return the icon in XMP format which will appear in the tree view. This method is optional
		and if not defined a default icon is shown.
		'''
		return """
			/* XPM */
			static const char * ViewProviderBox_xpm[] = {
			"16 16 6 1",
			" 	c None",
			".	c #141010",
			"+	c #615BD2",
			"@	c #C39D55",
			"#	c #000000",
			"$	c #57C355",
			"        ........",
			"   ......++..+..",
			"   .@@@@.++..++.",
			"   .@@@@.++..++.",
			"   .@@  .++++++.",
			"  ..@@  .++..++.",
			"###@@@@ .++..++.",
			"##$.@@$#.++++++.",
			"#$#$.$$$........",
			"#$$#######      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			" #$#$$$$$#      ",
			"  ##$$$$$#      ",
			"   #######      "};
			"""

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


def makeBox():
	doc=FreeCAD.newDocument()
	a=FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Box")
	Box(a)
	ViewProviderBox(a.ViewObject)
	doc.recompute()

# -----------------------------------------------------------------------------

class Line:
	def __init__(self, obj):
		''' Add two point properties '''
		obj.addProperty("App::PropertyVector","p1","Line","Start point")
		obj.addProperty("App::PropertyVector","p2","Line","End point").p2=FreeCAD.Vector(1,0,0)
		obj.Proxy = self

	def execute(self, fp):
		''' Print a short message when doing a recomputation, this method is mandatory '''
		fp.Shape = Part.makeLine(fp.p1,fp.p2)

class ViewProviderLine:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
		obj.Proxy = self

	def getDefaultDisplayMode(self):
		''' Return the name of the default display mode. It must be defined in getDisplayModes. '''
		return "Flat Lines"

def makeLine():
	doc=FreeCAD.newDocument()
	a=FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Line")
	Line(a)
	#ViewProviderLine(a.ViewObject)
	a.ViewObject.Proxy=0 # just set it to something different from None
	doc.recompute()

# -----------------------------------------------------------------------------

class Octahedron:
	def __init__(self, obj):
		"Add some custom properties to our box feature"
		obj.addProperty("App::PropertyLength","Length","Octahedron","Length of the octahedron").Length=1.0
		obj.addProperty("App::PropertyLength","Width","Octahedron","Width of the octahedron").Width=1.0
		obj.addProperty("App::PropertyLength","Height","Octahedron", "Height of the octahedron").Height=1.0
		obj.addProperty("Part::PropertyPartShape","Shape","Octahedron", "Shape of the octahedron")
		obj.Proxy = self
 
	def execute(self, fp):
		# Define six vetices for the shape
		v1 = FreeCAD.Vector(0,0,0)
		v2 = FreeCAD.Vector(fp.Length,0,0)
		v3 = FreeCAD.Vector(0,fp.Width,0)
		v4 = FreeCAD.Vector(fp.Length,fp.Width,0)
		v5 = FreeCAD.Vector(fp.Length/2,fp.Width/2,fp.Height/2)
		v6 = FreeCAD.Vector(fp.Length/2,fp.Width/2,-fp.Height/2)
		
		# Make the wires/faces
		f1 = self.make_face(v2,v1,v5)
		f2 = self.make_face(v4,v2,v5)
		f3 = self.make_face(v3,v4,v5)
		f4 = self.make_face(v1,v3,v5)
		f5 = self.make_face(v1,v2,v6)
		f6 = self.make_face(v2,v4,v6)
		f7 = self.make_face(v4,v3,v6)
		f8 = self.make_face(v3,v1,v6)
		shell=Part.makeShell([f1,f2,f3,f4,f5,f6,f7,f8])
		solid=Part.makeSolid(shell)
		fp.Shape = solid
	# helper method to create the faces
	def make_face(self,v1,v2,v3):
		wire = Part.makePolygon([v1,v2,v3,v1])
		face = Part.Face(wire)
		return face

class ViewProviderOctahedron:
	def __init__(self, obj):
		"Set this object to the proxy object of the actual view provider"
		obj.addProperty("App::PropertyColor","Color","Octahedron","Color of the octahedron").Color=(1.0,0.0,0.0)
		obj.Proxy = self
 
	def attach(self, obj):
		"Setup the scene sub-graph of the view provider, this method is mandatory"
		self.shaded = coin.SoGroup()
		self.wireframe = coin.SoGroup()
		self.color = coin.SoBaseColor()

		self.data=coin.SoCoordinate3()
		self.face=coin.SoIndexedFaceSet()
 
		self.shaded.addChild(self.color)
		self.shaded.addChild(self.data)
		self.shaded.addChild(self.face)
		obj.addDisplayMode(self.shaded,"Shaded");
		style=coin.SoDrawStyle()
		style.style = coin.SoDrawStyle.LINES
		self.wireframe.addChild(style)
		self.wireframe.addChild(self.color)
		self.wireframe.addChild(self.data)
		self.wireframe.addChild(self.face)
		obj.addDisplayMode(self.wireframe,"Wireframe");
		self.onChanged(obj,"Color")
 
	def updateData(self, fp, prop):
		"If a property of the handled feature has changed we have the chance to handle this here"
		# fp is the handled feature, prop is the name of the property that has changed
		if prop == "Shape":
			s = fp.getPropertyByName("Shape")
			self.data.point.setNum(6)
			cnt=0
			for i in s.Vertexes:
				self.data.point.set1Value(cnt,i.X,i.Y,i.Z)
				cnt=cnt+1
			
			self.face.coordIndex.set1Value(0,0)
			self.face.coordIndex.set1Value(1,2)
			self.face.coordIndex.set1Value(2,1)
			self.face.coordIndex.set1Value(3,-1)

			self.face.coordIndex.set1Value(4,3)
			self.face.coordIndex.set1Value(5,2)
			self.face.coordIndex.set1Value(6,0)
			self.face.coordIndex.set1Value(7,-1)
 
			self.face.coordIndex.set1Value(8,4)
			self.face.coordIndex.set1Value(9,2)
			self.face.coordIndex.set1Value(10,3)
			self.face.coordIndex.set1Value(11,-1)
 
			self.face.coordIndex.set1Value(12,1)
			self.face.coordIndex.set1Value(13,2)
			self.face.coordIndex.set1Value(14,4)
			self.face.coordIndex.set1Value(15,-1)
 
			self.face.coordIndex.set1Value(16,1)
			self.face.coordIndex.set1Value(17,5)
			self.face.coordIndex.set1Value(18,0)
			self.face.coordIndex.set1Value(19,-1)
 
			self.face.coordIndex.set1Value(20,0)
			self.face.coordIndex.set1Value(21,5)
			self.face.coordIndex.set1Value(22,3)
			self.face.coordIndex.set1Value(23,-1)
 
			self.face.coordIndex.set1Value(24,3)
			self.face.coordIndex.set1Value(25,5)
			self.face.coordIndex.set1Value(26,4)
			self.face.coordIndex.set1Value(27,-1)
 
			self.face.coordIndex.set1Value(28,4)
			self.face.coordIndex.set1Value(29,5)
			self.face.coordIndex.set1Value(30,1)
			self.face.coordIndex.set1Value(31,-1)

	def getDisplayModes(self,obj):
		"Return a list of display modes."
		modes=[]
		modes.append("Shaded")
		modes.append("Wireframe")
		return modes
 
	def getDefaultDisplayMode(self):
		"Return the name of the default display mode. It must be defined in getDisplayModes."
		return "Shaded"
 
	def setDisplayMode(self,mode):
		return mode

	def onChanged(self, vp, prop):
		"Here we can do something when a single property got changed"
		FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")
		if prop == "Color":
			c = vp.getPropertyByName("Color")
			self.color.rgb.setValue(c[0],c[1],c[2])

	def getIcon(self):
		return """
			/* XPM */
			static const char * ViewProviderBox_xpm[] = {
			"16 16 6 1",
			" 	c None",
			".	c #141010",
			"+	c #615BD2",
			"@	c #C39D55",
			"#	c #000000",
			"$	c #57C355",
			"        ........",
			"   ......++..+..",
			"   .@@@@.++..++.",
			"   .@@@@.++..++.",
			"   .@@  .++++++.",
			"  ..@@  .++..++.",
			"###@@@@ .++..++.",
			"##$.@@$#.++++++.",
			"#$#$.$$$........",
			"#$$#######      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			" #$#$$$$$#      ",
			"  ##$$$$$#      ",
			"   #######      "};
			"""
 
	def __getstate__(self):
		return None
 
	def __setstate__(self,state):
		return None

def makeOctahedron():
	doc=FreeCAD.newDocument()
	a=FreeCAD.ActiveDocument.addObject("App::FeaturePython","Octahedron")
	Octahedron(a)
	ViewProviderOctahedron(a.ViewObject)
	doc.recompute()

# -----------------------------------------------------------------------------

class PointFeature:
	def __init__(self, obj):
		obj.Proxy = self

	def onChanged(self, fp, prop):
		''' Print the name of the property that has changed '''
		return

	def execute(self, fp):
		''' Print a short message when doing a recomputation, this method is mandatory '''
		return

class ViewProviderPoints:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
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
		return "Points"

	def setDisplayMode(self,mode):
		''' Map the display mode defined in attach with those defined in getDisplayModes.
		Since they have the same names nothing needs to be done. This method is optional.
		'''
		return mode

	def onChanged(self, vp, prop):
		''' Print the name of the property that has changed '''
		return

	def getIcon(self):
		''' Return the icon in XMP format which will appear in the tree view. This method is optional
		and if not defined a default icon is shown.
		'''
		return """
			/* XPM */
			static const char * ViewProviderBox_xpm[] = {
			"16 16 6 1",
			" 	c None",
			".	c #141010",
			"+	c #615BD2",
			"@	c #C39D55",
			"#	c #000000",
			"$	c #57C355",
			"        ........",
			"   ......++..+..",
			"   .@@@@.++..++.",
			"   .@@@@.++..++.",
			"   .@@  .++++++.",
			"  ..@@  .++..++.",
			"###@@@@ .++..++.",
			"##$.@@$#.++++++.",
			"#$#$.$$$........",
			"#$$#######      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			" #$#$$$$$#      ",
			"  ##$$$$$#      ",
			"   #######      "};
			"""

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


def makePoints():
	doc=FreeCAD.newDocument()
	import Mesh
	m=Mesh.createSphere(5.0).Points
	import Points
	p=Points.Points()

	l=[]
	for s in m:
		l.append(s.Vector)

	p.addPoints(l)


	a=FreeCAD.ActiveDocument.addObject("Points::FeaturePython","Points")
	a.Points=p
	PointFeature(a)
	ViewProviderPoints(a.ViewObject)
	doc.recompute()

# -----------------------------------------------------------------------------

class MeshFeature:
	def __init__(self, obj):
		obj.Proxy = self

	def onChanged(self, fp, prop):
		''' Print the name of the property that has changed '''
		return

	def execute(self, fp):
		''' Print a short message when doing a recomputation, this method is mandatory '''
		return

class ViewProviderMesh:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
		obj.Proxy = self

	def attach(self, obj):
		''' Setup the scene sub-graph of the view provider, this method is mandatory '''
		return

	def getDefaultDisplayMode(self):
		''' Return the name of the default display mode. It must be defined in getDisplayModes. '''
		return "Shaded"

	def getIcon(self):
		''' Return the icon in XMP format which will appear in the tree view. This method is optional
		and if not defined a default icon is shown.
		'''
		return """
			/* XPM */
			static const char * ViewProviderBox_xpm[] = {
			"16 16 6 1",
			" 	c None",
			".	c #141010",
			"+	c #615BD2",
			"@	c #C39D55",
			"#	c #000000",
			"$	c #57C355",
			"        ........",
			"   ......++..+..",
			"   .@@@@.++..++.",
			"   .@@@@.++..++.",
			"   .@@  .++++++.",
			"  ..@@  .++..++.",
			"###@@@@ .++..++.",
			"##$.@@$#.++++++.",
			"#$#$.$$$........",
			"#$$#######      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			"#$$#$$$$$#      ",
			" #$#$$$$$#      ",
			"  ##$$$$$#      ",
			"   #######      "};
			"""

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


def makeMesh():
	doc=FreeCAD.newDocument()
	import Mesh

	a=FreeCAD.ActiveDocument.addObject("Mesh::FeaturePython","Mesh")
	a.Mesh=Mesh.createSphere(5.0)
	MeshFeature(a)
	ViewProviderMesh(a.ViewObject)
	doc.recompute()

# -----------------------------------------------------------------------------

class Molecule:
	def __init__(self, obj):
		''' Add two point properties '''
		obj.addProperty("App::PropertyVector","p1","Line","Start point")
		obj.addProperty("App::PropertyVector","p2","Line","End point").p2=FreeCAD.Vector(5,0,0)

		obj.Proxy = self

	def execute(self, fp):
		''' Print a short message when doing a recomputation, this method is mandatory '''
		fp.Shape = Part.makeLine(fp.p1,fp.p2)

class ViewProviderMolecule:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
		sep1=coin.SoSeparator()
		self.trl1=coin.SoTranslation()
		sep1.addChild(self.trl1)
		sep1.addChild(coin.SoSphere())
		sep2=coin.SoSeparator()
		self.trl2=coin.SoTranslation()
		sep2.addChild(self.trl2)
		sep2.addChild(coin.SoSphere())
		obj.RootNode.addChild(sep1)
		obj.RootNode.addChild(sep2)
		# triggers an updateData call so the assignment at the end
		obj.Proxy = self

	def updateData(self, fp, prop):
		"If a property of the handled feature has changed we have the chance to handle this here"
		# fp is the handled feature, prop is the name of the property that has changed
		if prop == "p1":
			p = fp.getPropertyByName("p1")
			self.trl1.translation=(p.x,p.y,p.z)
		elif prop == "p2":
			p = fp.getPropertyByName("p2")
			self.trl2.translation=(p.x,p.y,p.z)

	def __getstate__(self):
		return None
 
	def __setstate__(self,state):
		return None

def makeMolecule():
	doc=FreeCAD.newDocument()
	a=FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Molecule")
	Molecule(a)
	ViewProviderMolecule(a.ViewObject)
	doc.recompute()

# -----------------------------------------------------------------------------

class CircleSet:
	def __init__(self, obj):
		obj.addProperty("Part::PropertyPartShape","Shape","Circle","Shape")
		obj.Proxy = self

	def execute(self, fp):
		pass


class ViewProviderCircleSet:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
		obj.Proxy = self

	def attach(self, obj):
		self.coords=coin.SoCoordinate3()
		self.lines=coin.SoLineSet()
		obj.RootNode.addChild(self.coords)
		obj.RootNode.addChild(self.lines)

	def updateData(self, fp, prop):
			if prop == "Shape":
				edges = fp.getPropertyByName("Shape").Edges
				pts=[]
				ver=[]
				for i in edges:
					length=i.Length
					ver.append(10)
					for j in range(10):
						v=i.valueAt(j/9.0*length)
						pts.append((v.x,v.y,v.z))
				
				self.coords.point.setValues(pts)
				self.lines.numVertices.setValues(ver)

	def __getstate__(self):
		return None
 
	def __setstate__(self,state):
		return None

def makeCircleSet():
	x=0.5
	comp=Part.Compound([])
	for j in range (630):
		y=0.5
		for i in range (630):
			c = Part.makeCircle(0.1, Base.Vector(x,y,0), Base.Vector(0,0,1))
			#Part.show(c)
			comp.add(c)
			y=y+0.5
		x=x+0.5

	doc=FreeCAD.newDocument()
	a=FreeCAD.ActiveDocument.addObject("App::FeaturePython","Circles")
	CircleSet(a)
	ViewProviderCircleSet(a.ViewObject)
	a.Shape=comp
	doc.recompute()

# -----------------------------------------------------------------------------

class EnumTest:
	def __init__(self, obj):
		''' Add enum properties '''
		obj.addProperty("App::PropertyEnumeration","Enum","","Enumeration").Enum=["One","Two","Three"]
		obj.addProperty("App::PropertyEnumeration","Enum2","","Enumeration2").Enum2=["One","Two","Three"]
		obj.Proxy = self

	def execute(self, fp):
		return

class ViewProviderEnumTest:
	def __init__(self, obj):
		''' Set this object to the proxy object of the actual view provider '''
		obj.addProperty("App::PropertyEnumeration","Enum3","","Enumeration3").Enum3=["One","Two","Three"]
		obj.addProperty("App::PropertyEnumeration","Enum4","","Enumeration4").Enum4=["One","Two","Three"]
		obj.Proxy = self

	def updateData(self, fp, prop):
		print("prop updated:",prop)

	def __getstate__(self):
		return None

	def __setstate__(self,state):
		return None

def makeEnumTest():
	FreeCAD.newDocument()
	a=FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Enum")
	EnumTest(a)
	ViewProviderEnumTest(a.ViewObject)

# -----------------------------------------------------------------------------

class DistanceBolt:
	def __init__(self, obj):
		''' Add the properties: Length, Edges, Radius, Height '''
		obj.addProperty("App::PropertyInteger","Edges","Bolt","Number of edges of the outline").Edges=6
		obj.addProperty("App::PropertyLength","Length","Bolt","Length of the edges of the outline").Length=10.0
		obj.addProperty("App::PropertyLength","Radius","Bolt","Radius of the inner circle").Radius=4.0
		obj.addProperty("App::PropertyLength","Height","Bolt","Height of the extrusion").Height=20.0
		obj.Proxy = self

	def onChanged(self, fp, prop):
		if prop == "Edges" or prop == "Length" or prop == "Radius" or prop == "Height":
			self.execute(fp)

	def execute(self, fp):
		edges = fp.Edges
		if edges < 3:
			edges = 3
		length = fp.Length
		radius = fp.Radius
		height = fp.Height

		m=Base.Matrix()
		m.rotateZ(math.radians(360.0/edges))

		# create polygon
		polygon = []
		v=Base.Vector(length,0,0)
		for i in range(edges):
			polygon.append(v)
			v = m.multiply(v)
		polygon.append(v)
		wire = Part.makePolygon(polygon)

		# create circle
		circ=Part.makeCircle(radius)

		# Create the face with the polygon as outline and the circle as hole
		face=Part.Face([wire,Part.Wire(circ)])

		# Extrude in z to create the final solid
		extrude=face.extrude(Base.Vector(0,0,height))
		fp.Shape = extrude

def makeDistanceBolt():
	doc=FreeCAD.newDocument()
	bolt=FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Distance_Bolt")
	bolt.Label = "Distance bolt"
	DistanceBolt(bolt)
	bolt.ViewObject.Proxy=0
	doc.recompute()
