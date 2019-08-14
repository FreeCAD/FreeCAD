# -*- coding: utf-8 -*-
## \package DraftLine
# \ingroup DRAFT
# \brief This module contains everything related to Drat Line tool.
"""\package DraftLine
\ingroup DRAFT
\brief This module contains everything related to Drat Line tool.
"""
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2009, 2010                                              *
# *   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
from FreeCAD import Console as FCC
from Draft import typecheck
from Draft import getParam
from Draft import dimSymbol
from Draft import formatObject
from Draft import select
from Draft import getType
from DraftGui import todo
from Draft import _DraftObject
from Draft import _ViewProviderDraft
from Draft import arrowtypes
from DraftTools import Line
import DraftVecUtils

if FreeCAD.GuiUp:
    import FreeCADGui
    import Draft_rc
    from PySide.QtCore import QT_TRANSLATE_NOOP
    from DraftTools import translate
    gui = True
else:
    def QT_TRANSLATE_NOOP(context, txt):
        return txt

    def translate(context, txt):
        return txt
    gui = False
    _msg = ("Interface not present. "
            "Draft workbench will have some features disabled.")
    FCC.PrintWarning(translate("draft", _msg) + "\n")


def makeWire(pointslist,
             closed=False, placement=None, face=None, support=None):
    """Create a straight wire that passes through various points.

    Parameters
    ----------
    pointslist : list of Base::Vector3 or Part.Edge
        A list of points or and edge.
    closed : bool, optional
        It defaults to `False`. If it is `True`, or if the first
        and last points in the list are identical, the wire will
        be closed, joining the first point with the last one.
    placement : bool, optional
        It defaults to `None`. It is a `Base::Placement`
        that modifies where to the wire is placed.
    face : bool, optional
        It defaults to `None`. If it is `True` and the wire is closed
        the wire will form a filled face.
    support : App::PropertyLinkSubList, optional
        An object that acts as a group to support the object,
        and adjust the placement.
        This parameter is not normally used in Draft,
        but is useful in PartDesign to attach sketches to flat faces.

    Returns
    -------
    Part::Part2DObjectPython
        The returned line object.
    """
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import DraftGeomUtils, Part
    if not isinstance(pointslist, list):
        e = pointslist.Wires[0].Edges
        pointslist = Part.Wire(Part.__sortEdges__(e))
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        if DraftGeomUtils.isReallyClosed(pointslist):
            closed = True
        pointslist = nlist
    if len(pointslist) == 0:
        print("Invalid input points: ", pointslist)
    # print(pointslist)
    # print(closed)
    if placement:
        typecheck([(placement, FreeCAD.Placement)], "makeWire")
        ipl = placement.inverse()
        pointslist = [ipl.multVec(p) for p in pointslist]
    if len(pointslist) == 2:
        fname = "Line"
    else:
        fname = "Wire"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython", fname)
    _Wire(obj)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support
    if face is not None:
        obj.MakeFace = face
    if placement:
        obj.Placement = placement
    if gui:
        _ViewProviderWire(obj.ViewObject)
        formatObject(obj)
        select(obj)

    return obj


class _Wire(_DraftObject):
    """The Wire object."""

    def __init__(self, obj):
        _DraftObject.__init__(self, obj, "Wire")
        obj.addProperty("App::PropertyVectorList", "Points", "Draft", QT_TRANSLATE_NOOP("App::Property", "The vertices of the wire"))
        obj.addProperty("App::PropertyBool", "Closed", "Draft", QT_TRANSLATE_NOOP("App::Property", "If the wire is closed or not"))
        obj.addProperty("App::PropertyLink", "Base", "Draft", QT_TRANSLATE_NOOP("App::Property", "The base object is the wire, it's formed from 2 objects"))
        obj.addProperty("App::PropertyLink", "Tool", "Draft", QT_TRANSLATE_NOOP("App::Property", "The tool object is the wire, it's formed from 2 objects"))
        obj.addProperty("App::PropertyVectorDistance", "Start", "Draft", QT_TRANSLATE_NOOP("App::Property", "The start point of this line"))
        obj.addProperty("App::PropertyVectorDistance", "End", "Draft", QT_TRANSLATE_NOOP("App::Property", "The end point of this line"))
        obj.addProperty("App::PropertyLength", "Length", "Draft", QT_TRANSLATE_NOOP("App::Property", "The length of this line"))
        obj.addProperty("App::PropertyLength", "FilletRadius", "Draft", QT_TRANSLATE_NOOP("App::Property", "Radius to use to fillet the corners"))
        obj.addProperty("App::PropertyLength", "ChamferSize", "Draft", QT_TRANSLATE_NOOP("App::Property", "Size of the chamfer to give to the corners"))
        obj.addProperty("App::PropertyBool", "MakeFace", "Draft", QT_TRANSLATE_NOOP("App::Property", "Create a face if this object is closed"))
        obj.addProperty("App::PropertyInteger", "Subdivisions", "Draft", QT_TRANSLATE_NOOP("App::Property", "The number of subdivisions of each edge"))
        obj.addProperty("App::PropertyArea", "Area", "Draft", QT_TRANSLATE_NOOP("App::Property", "The area of this object"))
        obj.MakeFace = getParam("fillmode", True)
        obj.Closed = False

    def execute(self, obj):
        import Part, DraftGeomUtils
        plm = obj.Placement
        if obj.Base and (not obj.Tool):
            if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                shape = obj.Base.Shape.copy()
                if obj.Base.Shape.isClosed():
                    if hasattr(obj, "MakeFace"):
                        if obj.MakeFace:
                            shape = Part.Face(shape)
                    else:
                        shape = Part.Face(shape)
                obj.Shape = shape
        elif obj.Base and obj.Tool:
            if obj.Base.isDerivedFrom("Part::Feature") and obj.Tool.isDerivedFrom("Part::Feature"):
                if (not obj.Base.Shape.isNull()) and (not obj.Tool.Shape.isNull()):
                    sh1 = obj.Base.Shape.copy()
                    sh2 = obj.Tool.Shape.copy()
                    shape = sh1.fuse(sh2)
                    if DraftGeomUtils.isCoplanar(shape.Faces):
                        shape = DraftGeomUtils.concatenate(shape)
                        obj.Shape = shape
                        p = []
                        for v in shape.Vertexes:
                            p.append(v.Point)
                        if obj.Points != p:
                            obj.Points = p
        elif obj.Points:
            if obj.Points[0] == obj.Points[-1]:
                if not obj.Closed:
                    obj.Closed = True
                obj.Points.pop()
            if obj.Closed and (len(obj.Points) > 2):
                pts = obj.Points
                if hasattr(obj, "Subdivisions"):
                    if obj.Subdivisions > 0:
                        npts = []
                        for i in range(len(pts)):
                            p1 = pts[i]
                            npts.append(pts[i])
                            if i == len(pts)-1:
                                p2 = pts[0]
                            else:
                                p2 = pts[i+1]
                            v = p2.sub(p1)
                            v = DraftVecUtils.scaleTo(v, v.Length/(obj.Subdivisions+1))
                            for j in range(obj.Subdivisions):
                                npts.append(p1.add(FreeCAD.Vector(v).multiply(j+1)))
                        pts = npts
                shape = Part.makePolygon(pts+[pts[0]])
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,
                                                      obj.ChamferSize.Value,
                                                      chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,
                                                      obj.FilletRadius.Value)
                        if w:
                            shape = w
                try:
                    if hasattr(obj, "MakeFace"):
                        if obj.MakeFace:
                            shape = Part.Face(shape)
                    else:
                        shape = Part.Face(shape)
                except Part.OCCError:
                    pass
            else:
                edges = []
                pts = obj.Points[1:]
                lp = obj.Points[0]
                for p in pts:
                    if not DraftVecUtils.equals(lp, p):
                        if hasattr(obj, "Subdivisions"):
                            if obj.Subdivisions > 0:
                                npts = []
                                v = p.sub(lp)
                                v = DraftVecUtils.scaleTo(v, v.Length/(obj.Subdivisions+1))
                                edges.append(Part.LineSegment(lp, lp.add(v)).toShape())
                                lv = lp.add(v)
                                for j in range(obj.Subdivisions):
                                    edges.append(Part.LineSegment(lv, lv.add(v)).toShape())
                                    lv = lv.add(v)
                            else:
                                edges.append(Part.LineSegment(lp, p).toShape())
                        else:
                            edges.append(Part.LineSegment(lp, p).toShape())
                        lp = p
                try:
                    shape = Part.Wire(edges)
                except Part.OCCError:
                    print("Error wiring edges")
                    shape = None
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape, obj.ChamferSize.Value, chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape, obj.FilletRadius.Value)
                        if w:
                            shape = w
            if shape:
                obj.Shape = shape
                if hasattr(obj, "Area") and hasattr(shape, "Area"):
                    obj.Area = shape.Area
                if hasattr(obj, "Length"):
                    obj.Length = shape.Length
        obj.Placement = plm
        obj.positionBySupport()
        self.onChanged(obj, "Placement")

    def onChanged(self, obj, prop):
        if prop == "Start":
            pts = obj.Points
            invpl = FreeCAD.Placement(obj.Placement).inverse()
            realfpstart = invpl.multVec(obj.Start)
            if pts:
                if pts[0] != realfpstart:
                    pts[0] = realfpstart
                    obj.Points = pts
        elif prop == "End":
            pts = obj.Points
            invpl = FreeCAD.Placement(obj.Placement).inverse()
            realfpend = invpl.multVec(obj.End)
            if len(pts) > 1:
                if pts[-1] != realfpend:
                    pts[-1] = realfpend
                    obj.Points = pts
        elif prop == "Length":
            if obj.Shape and not obj.Shape.isNull():
                if obj.Length.Value != obj.Shape.Length:
                    if len(obj.Points) == 2:
                        v = obj.Points[-1].sub(obj.Points[0])
                        v = DraftVecUtils.scaleTo(v, obj.Length.Value)
                        obj.Points = [obj.Points[0], obj.Points[0].add(v)]

        elif prop == "Placement":
            pl = FreeCAD.Placement(obj.Placement)
            if len(obj.Points) >= 2:
                displayfpstart = pl.multVec(obj.Points[0])
                displayfpend = pl.multVec(obj.Points[-1])
                if obj.Start != displayfpstart:
                    obj.Start = displayfpstart
                if obj.End != displayfpend:
                    obj.End = displayfpend


class _ViewProviderWire(_ViewProviderDraft):
    """A View Provider for the Wire object"""
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self, obj)
        obj.addProperty("App::PropertyBool", "EndArrow", "Draft", QT_TRANSLATE_NOOP("App::Property", "Displays a Dimension symbol at the end of the wire"))
        obj.addProperty("App::PropertyLength", "ArrowSize", "Draft", QT_TRANSLATE_NOOP("App::Property", "Arrow size"))
        obj.addProperty("App::PropertyEnumeration", "ArrowType", "Draft", QT_TRANSLATE_NOOP("App::Property", "Arrow type"))
        obj.ArrowSize = getParam("arrowsize", 0.1)
        obj.ArrowType = arrowtypes
        obj.ArrowType = arrowtypes[getParam("dimsymbol", 0)]

    def attach(self, obj):
        from pivy import coin
        self.Object = obj.Object
        col = coin.SoBaseColor()
        col.rgb.setValue(obj.LineColor[0], obj.LineColor[1], obj.LineColor[2])
        self.coords = coin.SoTransform()
        self.pt = coin.SoSeparator()
        self.pt.addChild(col)
        self.pt.addChild(self.coords)
        self.symbol = dimSymbol()
        self.pt.addChild(self.symbol)
        _ViewProviderDraft.attach(self, obj)
        self.onChanged(obj, "EndArrow")

    def updateData(self, obj, prop):
        from pivy import coin
        if prop == "Points":
            if obj.Points:
                p = obj.Points[-1]
                if hasattr(self, "coords"):
                    self.coords.translation.setValue((p.x, p.y, p.z))
                    if len(obj.Points) >= 2:
                        v1 = obj.Points[-2].sub(obj.Points[-1])
                        if not DraftVecUtils.isNull(v1):
                            v1.normalize()
                            _rot = coin.SbRotation()
                            _rot.setValue(coin.SbVec3f(1, 0, 0), coin.SbVec3f(v1[0], v1[1], v1[2]))
                            self.coords.rotation.setValue(_rot)
        return

    def onChanged(self, vobj, prop):
        from pivy import coin
        if prop in ["EndArrow", "ArrowSize", "ArrowType", "Visibility"]:
            rn = vobj.RootNode
            if hasattr(self, "pt") and hasattr(vobj, "EndArrow"):
                if vobj.EndArrow and vobj.Visibility:
                    self.pt.removeChild(self.symbol)
                    s = arrowtypes.index(vobj.ArrowType)
                    self.symbol = dimSymbol(s)
                    self.pt.addChild(self.symbol)
                    self.updateData(vobj.Object, "Points")
                    if hasattr(vobj, "ArrowSize"):
                        s = vobj.ArrowSize
                    else:
                        s = getParam("arrowsize", 0.1)
                    self.coords.scaleFactor.setValue((s, s, s))
                    rn.addChild(self.pt)
                else:
                    if self.symbol:
                        if self.pt.findChild(self.symbol) != -1:
                            self.pt.removeChild(self.symbol)
                        if rn.findChild(self.pt) != -1:
                            rn.removeChild(self.pt)
        if prop in ["LineColor"]:
            if hasattr(self, "pt"):
                self.pt[0].rgb.setValue(vobj.LineColor[0],
                                        vobj.LineColor[1],
                                        vobj.LineColor[2])
        _ViewProviderDraft.onChanged(self, vobj, prop)
        return

    def claimChildren(self):
        if hasattr(self.Object, "Base"):
            return [self.Object.Base, self.Object.Tool]
        return []

    def setupContextMenu(self, vobj, menu):
        from PySide import QtCore, QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Edit.svg"), "Flatten this wire", menu)
        QtCore.QObject.connect(action1, QtCore.SIGNAL("triggered()"), self.flatten)
        menu.addAction(action1)

    def flatten(self):
        if hasattr(self, "Object"):
            if len(self.Object.Shape.Wires) == 1:
                import DraftGeomUtils
                fw = DraftGeomUtils.flattenWire(self.Object.Shape.Wires[0])
                points = [v.Point for v in fw.Vertexes]
                if len(points) == len(self.Object.Points):
                    if points != self.Object.Points:
                        FreeCAD.ActiveDocument.openTransaction("Flatten wire")
                        FreeCADGui.doCommand("FreeCAD.ActiveDocument."+self.Object.Name+".Points="+str(points).replace("Vector", "FreeCAD.Vector").replace(" ", ""))
                        FreeCAD.ActiveDocument.commitTransaction()

                    else:
                        from DraftTools import translate
                        FCC.PrintMessage(translate("Draft", "This Wire is already flat")+"\n")


class Wire(Line):
    "a FreeCAD command for creating a wire"

    def __init__(self):
        Line.__init__(self, wiremode=True)

    def GetResources(self):
        return {'Pixmap': 'Draft_Wire',
                'Accel': "P, L",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Wire", "Polyline"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Wire", "Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain")}

    def Activated(self):

        # allow to convert several Draft Lines to a Wire
        if len(FreeCADGui.Selection.getSelection()) > 1:
            edges = []
            for o in FreeCADGui.Selection.getSelection():
                if getType(o) != "Wire":
                    edges = []
                    break
                edges.extend(o.Shape.Edges)
            if edges:
                try:
                    import Part
                    w = Part.Wire(edges)
                except:
                    FreeCAD.Console.PrintError(translate("draft", "Unable to create a Wire from selected objects")+"\n")
                else:
                    pts = ",".join([str(v.Point) for v in w.Vertexes])
                    pts = pts.replace("Vector", "FreeCAD.Vector")
                    rems = ["FreeCAD.ActiveDocument.removeObject(\""+o.Name+"\")" for o in FreeCADGui.Selection.getSelection()]
                    FreeCADGui.addModule("Draft")
                    todo.delayCommit([(translate("draft", "Convert to Wire"),
                            ['wire = Draft.makeWire(['+pts+'])']+rems+['Draft.autogroup(wire)',
                             'FreeCAD.ActiveDocument.recompute()'])])
                    return

        Line.Activated(self, name=translate("draft", "Polyline"))


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Draft_Wire', Wire())
