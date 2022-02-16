#! python
# -*- coding: utf-8 -*-
# SkipGui.py
# 2022, by Mark Ganson <TheMarkster>
# LGPL 2.1 or later

# this file is called from c++ in Mod/Part/Gui/Command.cpp
# from various functions when these commands are activated:
#   Part::Loft
#   Part::Sweep
#   Part::Extrude
#   Part::Revolve
# Then if the user has preselected certain objects and subobjects
# the Gui can be skipped and the document objects directly created

# If False is returned, then the Gui dialog should be shown
# Else if True is returned, then the dialog can be skipped

import FreeCAD as App
import FreeCADGui as Gui

#for testing only to see how the calling cpp code handles runtime exceptions
#raise Exception("testing")

#these get functions return command strings for use with doCommand()
def getDocStr(document):
    """returns a command string to get the document"""
    return f"App.getDocument('{document.Name}')"

def getActStr(document):
    """returns a command string to get the active object of the document"""
    return f"{getDocStr(document)}.ActiveObject"

def getObjStr(document, object):
    """returns a command string to get the object in the document"""
    return f"{getDocStr(document)}.getObject('{object.Name}')"

def setHiddenStr(document, objs):
    """returns command strings to hide these objects"""
    ret = ""
    for obj in objs:
        ret += f"{getObjStr(document, obj)}.ViewObject.Visibility = False\n"
    return ret

def getSetPropListStr(document, propName, objList):
    """returns a command string to set propName of document's active object
    to the objects in objList.  propName is of type App::PropertyLinkList"""
    ret = f"{getActStr(document)}.{propName} = ["
    for obj in objList:
        ret += f"{getObjStr(document,obj)}, "
    ret += "]\n"
    return ret

def getAddObjStr(document, typeId, name):
    """returns a command string to add a new object of typeid to document"""
    return f"{getDocStr(document)}.addObject('{typeId}','{name}')\n"


def makeExtrude():

    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        App.Console.PrintLog(f"makeExtrude() nothing selected, not skipping dialog\n")
        return False

    selected = [sel.Object for sel in selx if bool(sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::Face") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1) or
                sel.Object.isDerivedFrom("Sketcher::SketchObject")) and
                sel.HasSubObjects == False]
    if len(selected) != 1:
        App.Console.PrintLog(f"makeExtrude() need exactly 1 selected profile to skip dialog, not skipping\n")
        return False

    solid = "True"
    base = selected[0]
    if hasattr(base.Shape, "Wire1"):
        if not base.Shape.Wire1.isClosed():
            solid = "False"
    else:
        if not bool(hasattr(base,"Shape") and len(base.Shape.Vertexes) == 1):
            App.Console.PrintLog(f"makeExtrude() no wire in shape and not a vertex (not skip dialog)\n")
            return False #no Wire1
        else:
            solid = "False"

    spines = [sel for sel in selx if sel.HasSubObjects]
    if len(spines) != 1:
        App.Console.PrintLog(f"makeExtrude() all edges not from same object or no edges selected, not skipping dialog\n")
        return False

    spineEdges = spines[0].SubElementNames
    subNames = [name for name in spineEdges if "Edge" in name]
    if len(subNames) != 1:
        App.Console.PrintLog(f"makeExtrude() only 1 edge supported for DirLink, not skipping dialog\n")
        return False

    dirLink = spines[0].Object.getSubObject(subNames[0])
    if dirLink.Curve.TypeId != "Part::GeomLine":
        App.Console.PrintLog(f"makeExtrude() selected edge must be of TypeId Part::GeomLine, is TypeId = {dirLink.Curve.TypeId}, not skipping dialog\n")
        return False

    if solid == "False":
        App.Console.PrintWarning("Solid = False\n")

    doc = selected[0].Document
    cmd = getAddObjStr(doc, "Part::Extrusion", "Extrude")
    actStr = getActStr(doc)
    cmd += actStr + ".Base = " + getObjStr(doc, selected[0]) + "\n"
    cmd += actStr + ".Solid = " + solid + "\n"
    cmd += actStr + ".DirMode = 'Edge'\n"
    cmd += actStr + ".Reversed = False\n"
    cmd += actStr + ".Symmetric = False\n"
    cmd += setHiddenStr(doc, selected)
    cmd += actStr + ".DirLink = (" + getObjStr(doc, spines[0].Object) + ", '" + subNames[0] + "')\n"
    Gui.doCommand(cmd)
    doc.recompute()
    return True


def makeRevolve():
    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        App.Console.PrintLog(f"makeRevolve() nothing selected, not skipping dialog\n")
        return False

    selected = [sel.Object for sel in selx if bool(sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::Face") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1) or
                sel.Object.isDerivedFrom("Sketcher::SketchObject")) and
                sel.HasSubObjects == False]
    if len(selected) != 1:
        App.Console.PrintLog(f"makeRevolve() only 1 selected profile needed, not skipping dialog\n")
        return False

    solid = "True"
    base = selected[0]
    if hasattr(base.Shape, "Wire1"):
        if not base.Shape.Wire1.isClosed():
            solid = "False"
    else:
        if not bool(hasattr(base,"Shape") and len(base.Shape.Vertexes) == 1):
            App.Console.PrintLog(f"makeRevolve() selected object has no wire and not a vertex, not skipping dialog\n")
            return False
        else:
            solid = "False"

    spines = [sel for sel in selx if sel.HasSubObjects]
    if len(spines) != 1:
        App.Console.PrintLog(f"makeRevolve() all edges must be from the same object, not skipping dialog\n")
        return False

    spineEdges = spines[0].SubElementNames
    subNames = [name for name in spineEdges if "Edge" in name]
    if len(subNames) != 1:
        App.Console.PrintLog(f"makeRevolve() only 1 edge may be selected, not skipping dialog\n")
        return False

    #check for proper curve types
    curveTypeIds = [spines[0].Object.getSubObject(elname).Curve.TypeId for elname in spineEdges if spines[0].Object.getSubObject(elname).Curve.TypeId in ["Part::GeomLine","Part::GeomCircle"]]
    if len(curveTypeIds) != 1:
        App.Console.PrintLog(f"makeRevolve() invalid edge type, must be line segment, circle, or arc.  Not skipping dialog\n")
        return False

    if solid == "False":
        App.Console.PrintWarning("Solid = False\n")

    doc = selected[0].Document
    doc.openTransaction("Revolve")
    cmd = getAddObjStr(doc, "Part::Revolution", "Revolve")
    cmd += getActStr(doc) + ".Source = " + getObjStr(doc, selected[0]) + "\n"
    cmd += getActStr(doc) + ".Solid = " + solid + "\n"
    cmd += getActStr(doc) + ".Symmetric = False\n"
    cmd += getActStr(doc) + ".Angle = 360.0\n"
    cmd += setHiddenStr(doc, selected)
    cmd += getActStr(doc) + ".AxisLink = (" + getObjStr(doc, spines[0].Object) + ", \'" + subNames[0] + "\')\n"
    Gui.doCommand(cmd)
    doc.commitTransaction()
    doc.recompute()
    return True


def makeLoft():
    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        App.Console.PrintLog(f"makeLoft() nothing selected, not skipping dialog\n")
        return False

    selected = [sel.Object for sel in selx if sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::Face") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                sel.Object.isDerivedFrom("Sketcher::SketchObject") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1)]
    if len(selected) < 2:
        App.Console.PrintLog(f"makeLoft() need at least 2 objects selected, not skipping dialog\n")
        return False

    #filter out sketches with multiple wires
    #should be removed if/when multiple wires are supported for Part::Loft
    filtered = [sel for sel in selected if len(sel.Shape.Wires) < 2]
    if len(filtered) < 2:
        App.Console.PrintLog(f"makeLoft() need at least 2 objects without multiple wires, not skipping dialog\n")
        return False

    if len(filtered) != len(selected):
        App.Console.PrintLog(f"makeLoft() some selected objects filtered out, not skipping dialog\n")
        return False

    doc = filtered[0].Document
    cmd = getAddObjStr(doc, "Part::Loft", "Loft")
    cmd += getSetPropListStr(doc, "Sections", filtered)
    cmd += getActStr(doc) + ".Solid = True\n"
    cmd += getActStr(doc) + ".Closed = False\n"
    cmd += getActStr(doc) + ".Ruled = False\n"
    cmd += setHiddenStr(doc, filtered)
    Gui.doCommand(cmd)
    doc.recompute()
    return True


def makeSweep():

    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        App.Console.PrintLog(f"makeSweep() nothing selected, not skipping dialog\n")
        return False

    selected = [sel.Object for sel in selx if bool(sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::Face") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1) or
                sel.Object.isDerivedFrom("Sketcher::SketchObject")) and
                sel.HasSubObjects == False]
    if len(selected) < 1:
        App.Console.PrintLog(f"makeSweep() need at least 1 selected profile, not skipping dialog\n")
        return False #need at least 1 profile for Sweep

    #filter out sketches with multiple wires
    #should be removed if/when multiple wires are supported for Part::Sweep
    filtered = [sel for sel in selected if len(sel.Shape.Wires) < 2]
    if len(filtered) < 1:
        App.Console.PrintLog(f"makeSweep() profiles may only have 1 wire, not skipping dialog\n")
        return False

    if len(filtered) != len(selected):
        App.Console.PrintLog(f"makeSweep() some selected profiles filtered out, not skipping dialog\n")
        return False

    solid = "True"
    base = filtered[0]
    if hasattr(base.Shape, "Wire1"):
        if not base.Shape.Wire1.isClosed():
            solid = "False"

    spines = [sel for sel in selx if sel.HasSubObjects]
    if len(spines) != 1:
        App.Console.PrintLog(f"makeSweep() all edges must be from the same object, not skipping dialog\n")
        return False

    spineEdges = spines[0].SubElementNames
    subNames = [name for name in spineEdges if "Edge" in name]
    if len(subNames) == 0:
        App.Console.PrintLog(f"makeSweep() must have selected at least 1 edge for path, not skipping dialog\n")
        return False

    if solid == "False":
        App.Console.PrintWarning("Solid = False\n")

    doc = filtered[0].Document
    cmd = getAddObjStr(doc, "Part::Sweep", "Sweep")
    cmd += getSetPropListStr(doc, "Sections", filtered)
    cmd += getActStr(doc) + ".Solid = " + solid + "\n"
    cmd += getActStr(doc) + ".Frenet = False\n"
    cmd += getActStr(doc) + ".Transition = \'Right corner\'\n"
    cmd += setHiddenStr(doc, filtered)
    cmd += getActStr(doc) + ".Spine = (" + getObjStr(doc, spines[0].Object) + ", " + repr(subNames) + ")\n"
    Gui.doCommand(cmd)
    doc.recompute()
    return True
