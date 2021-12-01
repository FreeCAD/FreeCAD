#! python
# -*- coding: utf-8 -*-
# (c) 2008 Werner Mayer LGPL


import Part, math
import FreeCAD, FreeCADGui
App=FreeCAD
Gui=FreeCADGui
from FreeCAD import Base

def makeBottle(myWidth=50.0, myHeight=70.0, myThickness=30.0):
	aPnt1=Base.Vector(-myWidth/2.,0,0)
	aPnt2=Base.Vector(-myWidth/2.,-myThickness/4.,0)
	aPnt3=Base.Vector(0,-myThickness/2.,0)
	aPnt4=Base.Vector(myWidth/2.,-myThickness/4.,0)
	aPnt5=Base.Vector(myWidth/2.,0,0)

	aArcOfCircle = Part.Arc(aPnt2,aPnt3,aPnt4)
	aSegment1=Part.LineSegment(aPnt1,aPnt2)
	aSegment2=Part.LineSegment(aPnt4,aPnt5)

	aEdge1=aSegment1.toShape()
	aEdge2=aArcOfCircle.toShape()
	aEdge3=aSegment2.toShape()
	aWire=Part.Wire([aEdge1,aEdge2,aEdge3])

	aTrsf=Base.Matrix()
	aTrsf.rotateZ(math.pi) # rotate around the z-axis

	aMirroredWire=aWire.copy()
	aMirroredWire.transformShape(aTrsf)
	myWireProfile=Part.Wire([aWire,aMirroredWire])

	myFaceProfile=Part.Face(myWireProfile)
	aPrismVec=Base.Vector(0,0,myHeight)
	myBody=myFaceProfile.extrude(aPrismVec)

	myBody=myBody.makeFillet(myThickness/12.0,myBody.Edges)

	neckLocation=Base.Vector(0,0,myHeight)
	neckNormal=Base.Vector(0,0,1)

	myNeckRadius = myThickness / 4.
	myNeckHeight = myHeight / 10
	myNeck = Part.makeCylinder(myNeckRadius,myNeckHeight,neckLocation,neckNormal)
	myBody = myBody.fuse(myNeck)

	faceToRemove = 0
	zMax = -1.0

	for xp in myBody.Faces:
		surf = xp.Surface
		if type(surf) == Part.Plane:
			z = surf.Position.z
			if z > zMax:
				zMax = z
				faceToRemove = xp

	# This doesn't work for any reason
	myBody = myBody.makeThickness([faceToRemove],-myThickness/50 , 1.e-3)
	myThreading = Part.makeThread(myNeckHeight/10, myNeckRadius*0.06, myHeight/10, myNeckRadius*0.99)
	myThreading.translate(Base.Vector(0,0,myHeight))
	myCompound = Part.Compound([myBody, myThreading])

	return myCompound

def makeBoreHole():
	# create a document if needed
	if App.ActiveDocument is None:
		App.newDocument("Solid")

	Group = App.ActiveDocument.addObject("App::DocumentObjectGroup","Group")
	Group.Label="Bore hole"

	V1 = Base.Vector(0,10,0)
	V2 = Base.Vector(30,10,0)
	V3 = Base.Vector(30,-10,0)
	V4 = Base.Vector(0,-10,0)
	VC1 = Base.Vector(-10,0,0)
	C1 = Part.Arc(V1,VC1,V4)
	# and the second one
	VC2 = Base.Vector(40,0,0)
	C2 = Part.Arc(V2,VC2,V3)
	L1 = Part.LineSegment(V1,V2)
	# and the second one
	L2 = Part.LineSegment(V4,V3)
	S1 = Part.Shape([C1,C2,L1,L2])

	W=Part.Wire(S1.Edges)
	F=Part.Face(W)
	P=F.extrude(Base.Vector(0,0,5))

	# add objects with the shape
	Wire=Group.newObject("Part::Feature","Wire")
	Wire.Shape=W
	Face=Group.newObject("Part::Feature","Face")
	Face.Shape=F
	Prism=Group.newObject("Part::Feature","Extrude")
	Prism.Shape=P

	c=Part.Circle(Base.Vector(0,0,-1),Base.Vector(0,0,1),2.0)
	w=Part.Wire(c.toShape())
	f=Part.Face(w)
	p=f.extrude(Base.Vector(0,0,7))
	P=P.cut(p)

	# add first borer
	Bore1=Group.newObject("Part::Feature","Borer_1")
	Bore1.Shape=p
	Hole1=Group.newObject("Part::Feature","Borer_Hole1")
	Hole1.Shape=P

	c=Part.Circle(Base.Vector(0,-11,2.5),Base.Vector(0,1,0),1.0)
	w=Part.Wire(c.toShape())
	f=Part.Face(w)
	p=f.extrude(Base.Vector(0,22,0))
	P=P.cut(p)

	# add second borer
	Bore2=Group.newObject("Part::Feature","Borer_2")
	Bore2.Shape=p
	Hole2=Group.newObject("Part::Feature","Borer_Hole2")
	Hole2.Shape=P

	App.ActiveDocument.recompute()

	# hide all objects except of the final one
	Gui.ActiveDocument.getObject(Wire.Name).hide()
	Gui.ActiveDocument.getObject(Face.Name).hide()
	Gui.ActiveDocument.getObject(Prism.Name).hide()
	Gui.ActiveDocument.getObject(Bore1.Name).hide()
	Gui.ActiveDocument.getObject(Hole1.Name).hide()
	Gui.ActiveDocument.getObject(Bore2.Name).hide()
	Gui.ActiveDocument.ActiveView.fitAll()
