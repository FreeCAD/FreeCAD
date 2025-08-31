# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import os
from builtins import open as pyopen

import FreeCAD
translate = FreeCAD.Qt.translate


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
            import urllib.request
            import FreeCADGui
            from PySide import QtGui
            reply = QtGui.QMessageBox.question(FreeCADGui.getMainWindow(),
                                               translate("Arch","Shapefile module not found"),
                                               translate("Arch","The shapefile Python library was not found on your system. Would you like to download it now from %1? It will be placed in your macros folder.").replace("%1","<a href=\"https://github.com/GeospatialPython/pyshp\">https://github.com/GeospatialPython/pyshp</a>"),
                                               QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                               QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.Yes:
                u = urllib.request.urlopen(url)
                if not u:
                    FreeCAD.Console.PrintError(translate("Arch","Error: Unable to download from %1").replace("%1",url)+"\n")
                    return False
                b = u.read()
                fp = os.path.join(FreeCAD.getUserMacroDir(True),"shapefile.py")
                f = pyopen(fp,"wb")
                f.write(b)
                f.close()
            else:
                FreeCAD.Console.PrintError(translate("Arch","Shapefile module not downloaded. Aborting.")+"\n")
                return False
        else:
            FreeCAD.Console.PrintError(translate("Arch","Shapefile module not found. Aborting.")+"\n")
            FreeCAD.Console.PrintMessage(translate("Arch","The shapefile library can be downloaded from the following URL and installed in your macros folder:")+"\n")
            FreeCAD.Console.PrintMessage(url)
            return False
    return True
