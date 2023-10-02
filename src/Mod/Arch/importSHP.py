# -*- coding: utf-8 -*-
#***************************************************************************
#*   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
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

import os
import FreeCAD
translate = FreeCAD.Qt.translate

if open.__module__ in ['__builtin__','io']:
    pythonopen = open

def open(filename):

    """opens a SHP/SHX/DBF file in a new FreeCAD document"""

    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    doc = insert(filename,doc.Name)
    return doc


def insert(filename,docname,record=None):

    """imports a SHP/SHX/DBF file in an existing FreeCAD document.
    the record attribute is an optional string indicating the shapefile
    field to use to give elevations to the different shapes. If not used,
    if running in GUI mode, a dialog will pop up to ask the user which
    field to use."""

    if not checkShapeFileLibrary():
        return

    import shapefile
    import Part

    # read the shape file
    # doc at https://github.com/GeospatialPython/pyshp

    shp = shapefile.Reader(filename)

    # check which record to use for elevation
    if not record:
        fields = ["None"] + [field[0] for field in shp.fields]
        if FreeCAD.GuiUp:
            import FreeCADGui
            from PySide import QtGui
            reply = QtGui.QInputDialog.getItem(FreeCADGui.getMainWindow(),
                                               translate("Arch","Shapes elevation"),
                                               translate("Arch","Choose which field provides shapes elevations:"),
                                               fields)
            if reply[1] and reply[0] != "None":
                    record = reply[0]

    # build shapes
    shapes = []
    for shaperec in shp.shapeRecords():
        shape = None
        pts = []
        for p in shaperec.shape.points:
            if len(p) > 2:
                pts.append(FreeCAD.Vector(p[0],p[1],p[2]))
            else:
                pts.append(FreeCAD.Vector(p[0],p[1],0))
        if shp.shapeTypeName in ["POLYGON","POLYGONZ"]:
            # faces
            pts.append(pts[0])
            shape = Part.makePolygon(pts)
            shape = Part.Face(shape)
        elif shp.shapeTypeName in ["POINT","POINTZ"]:
            # points
            verts = [Part.Vertex(p) for p in pts]
            if verts:
                shape = Part.makeCompound(verts)
        else:
            # polylines
            shape = Part.makePolygon(pts)
        if record:
            elev = shaperec.record[record]
            if elev:
                shape.translate(FreeCAD.Vector(0,0,elev))
        if shape:
            shapes.append(shape)
    if shapes:
        result = Part.makeCompound(shapes)
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature","shapefile")
        obj.Shape = result
        obj.Label = os.path.splitext(os.path.basename(filename))[0]
        FreeCAD.ActiveDocument.recompute()
    else:
        FreeCAD.Console.PrintWarning(translate("Arch","No shape found in this file")+"\n")

def getFields(filename):

    """returns the fields found in the given file"""

    if not checkShapeFileLibrary():
        return
    import shapefile
    shp = shapefile.Reader(filename)
    return [field[0] for field in shp.fields]

def checkShapeFileLibrary():

    """Looks for and/or installs the ShapeFile library"""

    try:
        import shapefile
    except Exception:
        url = "https://raw.githubusercontent.com/GeospatialPython/pyshp/master/shapefile.py"
        if FreeCAD.GuiUp:
            import addonmanager_utilities
            import FreeCADGui
            from PySide import QtGui
            reply = QtGui.QMessageBox.question(FreeCADGui.getMainWindow(),
                                               translate("Arch","Shapefile module not found"),
                                               translate("Arch","The shapefile python library was not found on your system. Would you like to download it now from <a href=\"https://github.com/GeospatialPython/pyshp\">https://github.com/GeospatialPython/pyshp</a>? It will be placed in your macros folder."),
                                               QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                               QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.Yes:
                u = addonmanager_utilities.urlopen(url)
                if not u:
                    FreeCAD.Console.PrintError(translate("Arch","Error: Unable to download from:")+" "+url+"\n")
                    return False
                b = u.read()
                p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
                fp = p.GetString("MacroPath",os.path.join(FreeCAD.getUserAppDataDir(),"Macros"))
                fp = os.path.join(fp,"shapefile.py")
                f = pythonopen(fp,"wb")
                f.write(b)
                f.close()
                try:
                    import shapefile
                except Exception:
                    FreeCAD.Console.PrintError(translate("Arch","Could not download shapefile module. Aborting.")+"\n")
                    return False
            else:
                FreeCAD.Console.PrintError(translate("Arch","Shapefile module not downloaded. Aborting.")+"\n")
                return False
        else:
            FreeCAD.Console.PrintError(translate("Arch","Shapefile module not found. Aborting.")+"\n")
            FreeCAD.Console.PrintMessage(translate("Arch","The shapefile library can be downloaded from the following URL and installed in your macros folder:")+"\n")
            FreeCAD.Console.PrintMessage(url)
            return False
    return True
