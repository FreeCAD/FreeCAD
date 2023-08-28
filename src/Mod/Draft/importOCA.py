# -*- coding: utf8 -*-
## @package importOCA
#  \ingroup DRAFT
#  \brief OCA (Open CAD Format) file importer & exporter
'''@package importOCA
\ingroup DRAFT
\brief OCA (Open CAD Format) file importer & exporter

This module provides support for importing from and exporting to
the OCA format or GCAD format from GCAD3D (http://www.gcad3d.org/).
See: https://groups.google.com/forum/#!forum/open_cad_format

As of 2019 this file format is practically obsolete, and this module is not
maintained.
'''
# Check code with
# flake8 --ignore=E226,E266,E401,W503

# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
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

__title__ = "FreeCAD Draft Workbench - OCA importer/exporter"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "https://www.freecad.org"

import FreeCAD, os, Part, DraftVecUtils, DraftGeomUtils
from FreeCAD import Vector
from FreeCAD import Console as FCC

if FreeCAD.GuiUp:
    from draftutils.translate import translate
else:
    def translate(context, txt):
        return txt

# Save the native open function to avoid collisions
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")


def getpoint(data):
    """Turn an OCA point definition into a FreeCAD.Vector.

    Parameters
    ----------
    data : list
        Different types of data.

    Returns
    -------
    Base::Vector3
        A vector with the data arranged, depending on the contents of `data`.
    """
    FCC.PrintMessage("found point %s \n" % data)
    if len(data) == 3:
        return Vector(float(data[0]), float(data[1]), float(data[2]))
    elif (data[0] == "P") and (len(data) == 4):
        return Vector(float(data[1]), float(data[2]), float(data[3]))
    elif (data[0][0] == "P") and (len(data[0]) > 1):
        if len(data) == 1:
            return objects[data[0]]
        else:
            if data[1][0] == "R":
                return objects[data[0]].add(objects[data[1]])
            elif data[1][0] == "C":
                # Error: DraftGeomUtils.findProjection()
                # doesn't exist
                return DraftGeomUtils.findProjection(objects[data[0]],
                                                     objects[data[1]])
    elif data[0][0] == "C":
        if objects[data[0]]:
            p1 = objects[data[0]].Curve.Position
            if len(data) == 1:
                return p1
            else:
                if data[1][0] == "L":
                    L = objects[data[1]]
                    return p1.add(DraftGeomUtils.vec(L))


def getarea(data):
    """Turn an OCA area definition into a FreeCAD Part.Wire.

    Parameters
    ----------
    data : list
        Different types of data.

    Returns
    -------
    Part.Wire
        A wire object from the points in `data`.
    """
    FCC.PrintMessage("found area %s \n" % data)
    if data[0] == "S":
        if data[1] == "POL":
            pts = data[2:]
            verts = []
            for p in pts:
                if p[0] == "P":
                    verts.append(getpoint([p]))
            w = Part.makePolygon(verts)
            return w


def getarc(data):
    """Turn an OCA arc definition into a FreeCAD Part.Edge.

    Parameters
    ----------
    data : list
        Different types of data.

    Returns
    -------
    Part.Edge
        An edge object from the points in `data`.
    """
    FCC.PrintMessage("found arc %s \n" % data)
    c = None
    if data[0] == "ARC":
        # 3-points arc
        pts = data[1:]
        verts = []
        for p in range(len(pts)):
            if pts[p] == "P":
                verts.append(getpoint(pts[p:p+3]))
            elif pts[p][0] == "P":
                verts.append(getpoint([pts[p]]))
        if verts[0] and verts[1] and verts[2]:
            c = Part.Arc(verts[0], verts[1], verts[2])
    elif data[0][0] == "P":
        # 2-point circle
        verts = []
        rad = None
        lines = []
        for p in range(len(data)):
            if data[p] == "P":
                verts.append(getpoint(data[p:p+4]))
            elif data[p][0] == "P":
                verts.append(getpoint([data[p]]))
            elif data[p] == "VAL":
                rad = float(data[p+1])
            elif data[p][0] == "L":
                lines.append(objects[data[p]])
        c = Part.Circle()
        c.Center = verts[0]
        if rad:
            c.Radius = rad
        else:
            # Error: DraftVecUtils.new()
            # doesn't exist
            c.Radius = DraftVecUtils.new(verts[0], verts[1]).Length
    elif data[0][0] == "L":
        # 2-lines circle
        lines = []
        rad = None
        for p in range(len(data)):
            if data[p] == "VAL":
                rad = float(data[p+1])
            elif data[p][0] == "L":
                lines.append(objects[data[p]])
        circles = DraftGeomUtils.circleFrom2LinesRadius(lines[0],
                                                        lines[1],
                                                        rad)
        if circles:
            c = circles[0]
    if c:
        return c.toShape()


def getline(data):
    """Turns an OCA line definition into a FreeCAD Part.Edge.

    Parameters
    ----------
    data : list
        Different types of data.

    Returns
    -------
    Part.Edge
        An edge object from the points in `data`.
    """
    FCC.PrintMessage("found line %s \n" % data)
    verts = []
    for p in range(len(data)):
        if data[p] == "P":
            verts.append(getpoint(data[p:p+4]))
        elif data[p][0] == "P":
            verts.append(getpoint([data[p]]))
    L = Part.LineSegment(verts[0], verts[1])
    return L.toShape()


def gettranslation(data):
    """Retrieve a translation (move) vector from `data`.

    Parameters
    ----------
    data : list
        Different types of data.

    Returns
    -------
    Base::Vector3
        A vector with X, Y, or Z displacement, or (0, 0, 0).
    """
    FCC.PrintMessage("found translation %s \n" % data)
    if data[0] == "Z":
        return Vector(0, 0, float(data[1]))
    elif data[0] == "Y":
        return Vector(0, float(data[1]), 0)
    elif data[0] == "X":
        return Vector(float(data[1]), 0, 0)
    return Vector(0, 0, 0)


def writepoint(vector):
    """Write a FreeCAD.Vector in OCA format.

    Parameters
    ----------
    vector : Base::Vector3
        A vector with three components.

    Returns
    -------
    str
        A string "P(X Y Z)" with the information from `vector`.
    """
    return "P("+str(vector.x)+" "+str(vector.y)+" "+str(vector.z)+")"


def createobject(oid, doc):
    """Create Part::Feature object in the current document.

    Parameters
    ----------
    oid : str
        ID number of a particular object in the document.

    doc : App::Document
        A FreeCAD document to which the object is added.
    Returns
    -------
    None
    """
    if isinstance(objects[oid], Part.Shape):
        ob = doc.addObject("Part::Feature", oid)
        ob.Shape = objects[oid]
        if FreeCAD.GuiUp:
            ob.ViewObject.ShapeColor = color


def parse(filename, doc):
    """Import an opened OCA file into the given document.

    Parameters
    ----------
    filename : str
        The path to the OCA file.
    doc : App::Document
        A FreeCAD document to which the object is added.

    Returns
    -------
    None
    """
    filebuffer = pythonopen(filename)
    global objects
    objects = {}
    global color
    color = (0, 0, 0)
    for l in filebuffer:
        readline = l.replace(",", " ").upper()
        if "=" in readline:
            # entity definitions
            pair = readline.split("=")
            _id = pair[0]
            data = pair[1]
            data = data.replace(",", " ")
            data = data.replace("(", " ")
            data = data.replace(")", " ")
            data = data.split()
            if _id[0] == "P":
                # point
                objects[_id] = getpoint(data)
            elif ((_id[0] == "A") and params.GetBool("ocaareas")):
                # area
                objects[_id] = getarea(data)
                createobject(_id, doc)

            elif _id[0] == "C":
                # arc or circle
                objects[_id] = getarc(data)
                createobject(_id, doc)

            elif _id[0] == "L":
                # line
                objects[_id] = getline(data)
                createobject(_id, doc)

            elif _id[0] == "R":
                # translation
                objects[_id] = gettranslation(data)

        elif readline[0:6] == "DEFCOL":
            # color
            c = readline.split()
            color = (float(c[1])/255,
                     float(c[2])/255,
                     float(c[3])/255)


def open(filename):
    """Open filename and parse.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.

    Returns
    -------
    None
    """
    docname = os.path.split(filename)[1]
    doc = FreeCAD.newDocument(docname)
    if docname[-4:] == "gcad":
        doc.Label = docname[:-5]
    else:
        doc.Label = docname[:-4]
    parse(filename, doc)
    doc.recompute()


def insert(filename, docname):
    """Get an active document and parse.

    If no document exist, it is created.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.
    docname : str
        The name of the active App::Document if one exists, or
        of the new one created.

    Returns
    -------
    None
    """
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    parse(filename, doc)
    doc.recompute()


def export(exportList, filename):
    """Export the OCA file with a given list of objects.

    The objects must be edges or faces, in order to be processed
    and exported.

    Parameters
    ----------
    exportList : list
        List of document objects to export.
    filename : str
        Path to the new file.

    Returns
    -------
    None
        If `exportList` doesn't have shapes to export.
    """
    faces = []
    edges = []

    # getting faces and edges
    for ob in exportList:
        if ob.Shape.Faces:
            for f in ob.Shape.Faces:
                faces.append(f)
        else:
            for e in ob.Shape.Edges:
                edges.append(e)
    if not (edges or faces):
        FCC.PrintMessage(translate("importOCA",
                                   "OCA: found no data to export")
                         + "\n")
        return

    # writing file
    oca = pythonopen(filename, 'w')
    oca.write("#oca file generated from FreeCAD\r\n")
    oca.write("# edges\r\n")
    count = 1
    for e in edges:
        if DraftGeomUtils.geomType(e) == "Line":
            oca.write("L"+str(count)+"=")
            oca.write(writepoint(e.Vertexes[0].Point))
            oca.write(" ")
            oca.write(writepoint(e.Vertexes[-1].Point))
            oca.write("\r\n")
        elif DraftGeomUtils.geomType(e) == "Circle":
            if len(e.Vertexes) > 1:
                oca.write("C"+str(count)+"=ARC ")
                oca.write(writepoint(e.Vertexes[0].Point))
                oca.write(" ")
                oca.write(writepoint(DraftGeomUtils.findMidpoint(e)))
                oca.write(" ")
                oca.write(writepoint(e.Vertexes[-1].Point))
            else:
                oca.write("C"+str(count)+"= ")
                oca.write(writepoint(e.Curve.Center))
                oca.write(" ")
                oca.write(str(e.Curve.Radius))
            oca.write("\r\n")
        count += 1
    oca.write("# faces\r\n")
    for f in faces:
        oca.write("A"+str(count)+"=S(POL")
        for v in f.Vertexes:
            oca.write(" ")
            oca.write(writepoint(v.Point))
        oca.write(" ")
        oca.write(writepoint(f.Vertexes[0].Point))
        oca.write(")\r\n")
        count += 1

    # closing
    oca.close()
    FCC.PrintMessage(translate("importOCA",
                               "successfully exported")
                     + " " + filename + "\n")
