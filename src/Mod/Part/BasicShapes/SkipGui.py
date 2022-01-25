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

def getActStr(document):
    return "App.getDocument(\'" + document.Name + "\').ActiveObject"

def getDocStr(document):
    return "App.getDocument(\'" + document.Name +"\')"

def getObjStr(document, object):
    return getDocStr(document) + ".getObject(\'" + object.Name + "\')"

def getVisStr(document, objs):
    ret = ""
    for obj in objs:
        ret += getDocStr(document) + ".getObject(\'" + obj.Name + "\').ViewObject.Visibility = False\n"
    return ret

def getSetPropListStr(document, propName, objList):
    ret = getActStr(document) + "." + propName + " = ["
    for obj in objList:
        ret += getDocStr(document) + ".getObject(\'" + obj.Name + "\'), "
    ret += "]\n"
    return ret

def getAddObjStr(document, typeId, name):
    ret = getDocStr(document)
    ret += ".addObject(\'" + typeId + "\', \'" + name + "\')\n"
    return ret

def getSubListStr(names):
    ret = "["
    for name in names:
        ret += "\'" + name + "\', "
    ret += "]"
    return ret


def makeExtrude():

    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        App.Console.PrintMessage(f"makeExtrude() len(selx) = {len(selx)} (must be > 0 to skip dialog)\n")
        return False

    selected = [sel.Object for sel in selx if bool(sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1) or
                sel.Object.isDerivedFrom("Sketcher::SketchObject")) and
                sel.HasSubObjects == False]
    if len(selected) != 1:
        App.Console.PrintMessage(f"makeExtrude() len(selected) = {len(selected)} (must be 1 to skip dialog)\n")
        return False #need exactly 1 profile for Extrude

    solid = "True"
    base = selected[0]
    if hasattr(base.Shape, "Wire1"):
        if not base.Shape.Wire1.isClosed():
            solid = "False"
    else:
        if not bool(hasattr(base,"Shape") and len(base.Shape.Vertexes) == 1):
            return False #no Wire1
        else:
            solid = "False"

    spines = [sel for sel in selx if sel.HasSubObjects]
    if len(spines) != 1:
        return False #all edges must be from the same object

    spineEdges = spines[0].SubElementNames
    subNames = [name for name in spineEdges if "Edge" in name]
    if len(subNames) != 1:
        return False

    dirLink = spines[0].Object.getSubObject(subNames[0])
    if dirLink.Curve.TypeId != "Part::GeomLine":
        return False

    if solid == "False":
        App.Console.PrintWarning("Solid = False\n")

    doc = selected[0].Document
    cmd = getAddObjStr(doc, "Part::Extrusion", "Extrude")
    cmd += getActStr(doc) + ".Base = " + getObjStr(doc, selected[0]) + "\n"
    cmd += getActStr(doc) + ".Solid = " + solid + "\n"
    cmd += getActStr(doc) + ".DirMode = \'Edge\'\n"
    cmd += getActStr(doc) + ".Reversed = False\n"
    cmd += getActStr(doc) + ".Symmetric = False\n"
    cmd += getVisStr(doc, selected)
    cmd += getActStr(doc) + ".DirLink = (" + getObjStr(doc, spines[0].Object) + ", \'" + subNames[0] + "\')\n"
    Gui.doCommand(cmd)
    doc.recompute()
    return True


def makeRevolve():
    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        return False #nothing selected, False = don't skip Gui

    selected = [sel.Object for sel in selx if bool(sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1) or
                sel.Object.isDerivedFrom("Sketcher::SketchObject")) and
                sel.HasSubObjects == False]
    if len(selected) != 1:
        return False #need exactly 1 profile for Revolve

    solid = "True"
    base = selected[0]
    if hasattr(base.Shape, "Wire1"):
        if not base.Shape.Wire1.isClosed():
            solid = "False"
    else:
        if not bool(hasattr(base,"Shape") and len(base.Shape.Vertexes) == 1):
            return False #no Wire1
        else:
            solid = "False"

    spines = [sel for sel in selx if sel.HasSubObjects]
    if len(spines) != 1:
        return False #all edges must be from the same objects

    spineEdges = spines[0].SubElementNames
    subNames = [name for name in spineEdges if "Edge" in name]
    if len(subNames) != 1:
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
    cmd += getVisStr(doc, selected)
    cmd += getActStr(doc) + ".AxisLink = (" + getObjStr(doc, spines[0].Object) + ", \'" + subNames[0] + "\')\n"
    Gui.doCommand(cmd)
    doc.commitTransaction()
    doc.recompute()
    return True


def makeLoft():

    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        return False #nothing selected, False = don't skip Gui

    selected = [sel.Object for sel in selx if sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                sel.Object.isDerivedFrom("Sketcher::SketchObject") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1)]
    if len(selected) < 2:
        return False #need at least 2 objects for Loft

    #filter out sketches with multiple wires
    #should be removed if/when multiple wires are supported for Part::Loft
    filtered = [sel for sel in selected if len(sel.Shape.Wires) < 2]
    if len(filtered) < 2:
        return False

    if len(filtered) != len(selected):
        return False #user will not get expected result since some profiles are not supported

    doc = filtered[0].Document
    cmd = getAddObjStr(doc, "Part::Loft", "Loft")
    cmd += getSetPropListStr(doc, "Sections", filtered)
    cmd += getActStr(doc) + ".Solid = True\n"
    cmd += getActStr(doc) + ".Closed = False\n"
    cmd += getActStr(doc) + ".Ruled = False\n"
    cmd += getVisStr(doc, filtered)
    Gui.doCommand(cmd)
    doc.recompute()
    return True


def makeSweep():

    selx = Gui.Selection.getSelectionEx()
    if len(selx) == 0:
        return False #nothing selected, False = don't skip Gui

    selected = [sel.Object for sel in selx if bool(sel.Object.isDerivedFrom("Part::2DObject") or
                sel.Object.isDerivedFrom("Part::FeaturePython") or
                bool(hasattr(sel.Object,"Shape") and len(sel.Object.Shape.Vertexes) == 1) or
                sel.Object.isDerivedFrom("Sketcher::SketchObject")) and
                sel.HasSubObjects == False]
    if len(selected) < 1:
        return False #need at least 1 profile for Sweep

    #filter out sketches with multiple wires
    #should be removed if/when multiple wires are supported for Part::Sweep
    filtered = [sel for sel in selected if len(sel.Shape.Wires) < 2]
    if len(filtered) < 1:
        return False #must have at least 1 profile to sweep

    if len(filtered) != len(selected):
        return False #user will not get expected result since some profiles are not supported

    solid = "True"
    base = filtered[0]
    if hasattr(base.Shape, "Wire1"):
        if not base.Shape.Wire1.isClosed():
            solid = "False"

    spines = [sel for sel in selx if sel.HasSubObjects]
    if len(spines) != 1:
        return False #all edges must be from the same object

    spineEdges = spines[0].SubElementNames
    subNames = [name for name in spineEdges if "Edge" in name]
    if len(subNames) == 0:
        return False

    if solid == "False":
        App.Console.PrintWarning("Solid = False\n")

    doc = filtered[0].Document
    cmd = getAddObjStr(doc, "Part::Sweep", "Sweep")
    cmd += getSetPropListStr(doc, "Sections", filtered)
    cmd += getActStr(doc) + ".Solid = " + solid + "\n"
    cmd += getActStr(doc) + ".Frenet = False\n"
    cmd += getActStr(doc) + ".Transition = \'Right corner\'\n"
    cmd += getVisStr(doc, filtered)
    cmd += getActStr(doc) + ".Spine = (" + getObjStr(doc, spines[0].Object) + ", " + getSubListStr(subNames) + ")\n"
    Gui.doCommand(cmd)
    doc.recompute()
    return True
