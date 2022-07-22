# Involute Gears Generation Script
# by Marcin Wanczyk (dj_who)
# (c) 2011 LGPL


import FreeCAD
import FreeCADGui
import Part
import Draft
import MeshPart
import Mesh
import math
from PySide import QtGui, QtCore

App = FreeCAD
Gui = FreeCADGui


def proceed():
    try:
        compute()
    except Exception:
        hide()
        QtGui.QApplication.restoreOverrideCursor()


def compute():
    QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)

    if FreeCAD.ActiveDocument is None:
        FreeCAD.newDocument("Gear")

    oldDocumentObjects = App.ActiveDocument.Objects

    try:
        N = int(l1.text())
        p = float(l2.text())
        alfa = int(l3.text())
        y = float(l4.text())       # standard value y<1 for gear drives y>1 for Gear pumps
        m = p/math.pi              # standard value 0.06, 0.12, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 60 (polish norm)
        c = float(l5.text())*m     # standard value 0,1*m - 0,3*m
        j = float(l6.text())*m     # standard value 0,015 - 0,04*m
        width = float(l7.text())   # gear width
    except ValueError:
        FreeCAD.Console.PrintError("Wrong input! Only numbers allowed...\n")


    # tooth height
    h = 2*y*m+c

    # pitch diameter
    d = N*m

    # root diameter
    df = d - 2*y*m - 2*c              # df=d-2hf where and hf=y*m+c

    # addendum diameter
    da = d + 2*y*m                    # da=d+2ha where ha=y*m

    # base diameter for involute
    db = d * math.cos(math.radians(alfa))


    #Base circle
    baseCircle = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "BaseCircle")
    Draft._Circle(baseCircle)
    Draft._ViewProviderDraft(baseCircle.ViewObject)
    baseCircle.Radius = db/2
    baseCircle.FirstAngle = 0.0
    baseCircle.LastAngle = 0.0

    # Root circle
    rootCircle = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "RootCircle")
    Draft._Circle(rootCircle)
    Draft._ViewProviderDraft(rootCircle.ViewObject)
    rootCircle.Radius = df/2
    rootCircle.FirstAngle = 0.0
    rootCircle.LastAngle = 0.0

    # Addendum circle
    addendumCircle = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "AddendumCircle")
    Draft._Circle(addendumCircle)
    Draft._ViewProviderDraft(addendumCircle.ViewObject)
    addendumCircle.Radius = da/2
    addendumCircle.FirstAngle = 0.0
    addendumCircle.LastAngle = 0.0

    # Pitch circle
    pitchCircle = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "PitchCircle")
    Draft._Circle(pitchCircle)
    Draft._ViewProviderDraft(pitchCircle.ViewObject)
    pitchCircle.Radius = d/2
    pitchCircle.FirstAngle = 0.0
    pitchCircle.LastAngle = 0.0

#************ Calculating right sides of teeth
    # Involute of base circle
    involute = []
    involutee = []
    involutesav = []

    for t in range(0, 60, 1):
        x = db/2*(math.cos(math.radians(t))+math.radians(t)*math.sin(math.radians(t)))
        y = db/2*(math.sin(math.radians(t))-math.radians(t)*math.cos(math.radians(t)))
        involute.append(Part.Vertex(x, y, 0).Point)

#************ Drawing right sides of teeth
    involutesav.extend(involute)
    involutee.extend(involute)

    for angle in range(1, N+1, 1):
        involuteobj = FreeCAD.ActiveDocument.addObject("Part::Feature", "InvoluteL" + str(angle))
        involutee.insert(0, (0, 0, 0))
        involuteshape = Part.makePolygon(involutee)
        involuteobj.Shape=involuteshape
        involutee = []
        for num in range(0, 60, 1):
            point = involute.pop()
            pointt = Part.Vertex(point.x*math.cos(math.radians(angle*360/N)) - point.y*math.sin(math.radians(angle*360/N)),point.x*math.sin(math.radians(angle*360/N)) + point.y*math.cos(math.radians(angle*360/N)),0).Point
            involutee.insert(0,pointt)
        involute.extend(involutesav)
    involutee = []

#************ Calculating difference between tooth spacing on BaseCircle and PitchCircle

    pc = App.ActiveDocument.getObject("PitchCircle")
    inv = App.ActiveDocument.getObject("InvoluteL1")
    cut = inv.Shape.cut(pc.Shape)
#    FreeCAD.ActiveDocument.addObject("Part::Feature","CutInv").Shape=cut
    invPoint = cut.Vertexes[0].Point

    diff = invPoint.y*2       # instead of making axial symmetry and calculating point distance.
    anglediff = 2*math.asin(diff/d)

#************ Calculating left sides of teeth

#************ Inversing Involute
    for num in range(0, 60, 1):
        point = involute.pop()
        pointt = Part.Vertex(point.x, point.y*-1, 0).Point
        involutee.insert(0, pointt)
    involute.extend(involutee)
    involutee = []

#Normal tooth size calculated as: 0,5*  p    -      j                       j = m * 0,1 below are calculations
#                                 0,5*  p    -      m      * 0,1
#                                 0,5*  p    -     p   /pi * 0,1
#                                 0,5*360/N  - ((360/N)/pi)* 0,1
#                                 0,5*360/N  - (360/N)*((1/pi)*0,1)         j = (p/pi)*0,1
#                                 0,5*360/N  - (360/N)*((p/pi)*0,1)/p
#                                 0,5*360/N  - (360/N)*(    j     )/p
    for num in range(0, 60, 1):
        point = involute.pop()
        pointt = Part.Vertex(point.x*math.cos(math.radians(180/N-(360/N)*(j/p))+anglediff) - point.y*math.sin(math.radians(180/N-(360/N)*(j/p))+anglediff),point.x*math.sin(math.radians(180/N-(360/N)*(j/p))+anglediff) + point.y*math.cos(math.radians(180/N-(360/N)*(j/p))+anglediff),0).Point
        involutee.insert(0, pointt)
    involute.extend(involutee)
    involutesav = []
    involutesav.extend(involute)

#************ Drawing left sides of teeth
    for angle in range(1, N+1, 1):
        involuteobj = FreeCAD.ActiveDocument.addObject("Part::Feature", "InvoluteR" + str(angle))
        involutee.insert(0, (0, 0, 0))
        involuteshape = Part.makePolygon(involutee)
        involuteobj.Shape = involuteshape
        involutee = []
        for num in range(0,60,1):
            point = involute.pop()
            pointt = Part.Vertex(point.x*math.cos(math.radians(angle*360/N)) - point.y*math.sin(math.radians(angle*360/N)),point.x*math.sin(math.radians(angle*360/N)) + point.y*math.cos(math.radians(angle*360/N)),0).Point
            involutee.insert(0,pointt)
        involute.extend(involutesav)

    Gui.SendMsgToActiveView("ViewFit")

#************ Forming teeth

    cutCircle = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "CutCircle")
    Draft._Circle(cutCircle)
    Draft._ViewProviderDraft(cutCircle.ViewObject)
    cutCircle.Radius = da    # da because must be bigger than addendumCircle and bigger than whole construction da is right for this but it not has to be.
    cutCircle.FirstAngle = 0.0
    cutCircle.LastAngle = 0.0


    cutTool = cutCircle.Shape.cut(addendumCircle.Shape)
    # cutshape = Part.show(cutTool)

    gearShape = rootCircle.Shape

    for invNum in range(1, N+1, 1):
        invL = App.ActiveDocument.getObject("InvoluteL" + str(invNum))
        invR = App.ActiveDocument.getObject("InvoluteR" + str(invNum))
        cutL = invL.Shape.cut(cutTool)
        cutR = invR.Shape.cut(cutTool)
        pointL = cutL.Vertexes.pop().Point
        pointR = cutR.Vertexes.pop().Point
        faceEdge = Part.makeLine(pointL, pointR)

        toothWhole = cutL.fuse(cutR)
        toothWhole = toothWhole.fuse(faceEdge)
        toothWire = Part.Wire(toothWhole.Edges)
        toothShape = Part.Face(toothWire)
#        tooth = App.ActiveDocument.addObject("Part::Feature", "Tooth" +str(invNum))
#        tooth.Shape=toothShape
        gearShape = gearShape.fuse(toothShape)


    for o in App.ActiveDocument.Objects:
        if oldDocumentObjects.count(o) == 0:
            App.ActiveDocument.removeObject(o.Name)

    gearFlat = App.ActiveDocument.addObject("Part::Feature", "GearFlat")
    gearFlat.Shape = gearShape
    Gui.ActiveDocument.getObject(gearFlat.Name).Visibility = False

    gear = App.ActiveDocument.addObject("Part::Extrusion", "Gear3D")
    gear.Base = gearFlat
    gear.Dir = (0, 0, width)
    App.ActiveDocument.recompute()


    if c1.isChecked():
        gearMesh = App.ActiveDocument.addObject("Mesh::Feature", "Gear3D-mesh")

        faces = []
        triangles = gear.Shape.tessellate(1)  # the number represents the precision of the tessellation)
        for tri in triangles[1]:
            face = []
            for i in range(3):
                vindex = tri[i]
                face.append(triangles[0][vindex])
            faces.append(face)
        mesh = Mesh.Mesh(faces)

        gearMesh.Mesh = mesh
        App.ActiveDocument.removeObject(gear.Name)
        App.ActiveDocument.removeObject(gearFlat.Name)


    App.ActiveDocument.recompute()
    Gui.SendMsgToActiveView("ViewFit")

    QtGui.QApplication.restoreOverrideCursor()


    hide()

def hide():
    dialog.hide()

dialog = QtGui.QDialog()
dialog.resize(200,450)
dialog.setWindowTitle("Gear")
la = QtGui.QVBoxLayout(dialog)
t1 = QtGui.QLabel("Number of teeth (N)")
la.addWidget(t1)
l1 = QtGui.QLineEdit()
l1.setText("16")
la.addWidget(l1)
t2 = QtGui.QLabel("Circular pitch (p)")
la.addWidget(t2)
l2 = QtGui.QLineEdit()
l2.setText("1.65")
la.addWidget(l2)
t3 = QtGui.QLabel("Pressure angle (alfa)")
la.addWidget(t3)
l3 = QtGui.QLineEdit()
l3.setText("20")
la.addWidget(l3)
t4 = QtGui.QLabel("Tooth height factor (y)")
la.addWidget(t4)
l4 = QtGui.QLineEdit()
l4.setText("1.0")
la.addWidget(l4)
t5 = QtGui.QLabel("Tooth clearance (c)")
la.addWidget(t5)
l5 = QtGui.QLineEdit()
l5.setText("0.1")
la.addWidget(l5)
t6 = QtGui.QLabel("Tooth lateral clearance (j)")
la.addWidget(t6)
l6 = QtGui.QLineEdit()
l6.setText("0.04")
la.addWidget(l6)
t7 = QtGui.QLabel("Gear width")
la.addWidget(t7)
l7 = QtGui.QLineEdit()
l7.setText("6.0")
la.addWidget(l7)
c1 = QtGui.QCheckBox("Create as a Mesh")
la.addWidget(c1)
e1 = QtGui.QLabel("(for faster rendering)")
commentFont = QtGui.QFont("Times", 8, True)
e1.setFont(commentFont)
la.addWidget(e1)

okbox = QtGui.QDialogButtonBox(dialog)
okbox.setOrientation(QtCore.Qt.Horizontal)
okbox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
la.addWidget(okbox)
QtCore.QObject.connect(okbox, QtCore.SIGNAL("accepted()"), proceed)
QtCore.QObject.connect(okbox, QtCore.SIGNAL("rejected()"), hide)
QtCore.QMetaObject.connectSlotsByName(dialog)
dialog.show()
