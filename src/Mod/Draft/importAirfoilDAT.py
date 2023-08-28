# -*- coding: utf-8 -*-
## @package importAirfoilDAT
#  \ingroup DRAFT
#  \brief Airfoil (.dat) file importer
#
# This module provides support for importing airfoil .dat files
'''@package importAirfoilDAT
\ingroup DRAFT
\brief Airfoil (.dat) file importer

This module provides support for importing airfoil .dat files.
'''
# Check code with
# flake8 --ignore=E226,E266,E401,W503

# ***************************************************************************
# *   Copyright (c) 2010 Heiko Jakob <heiko.jakob@gediegos.de>              *
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

__title__ = "FreeCAD Draft Workbench - Airfoil data importer"
__author__ = "Heiko Jakob <heiko.jakob@gediegos.de>"
__url__ = "https://www.freecad.org"

import re
import os
import FreeCAD
import Draft
import Part
from FreeCAD import Vector
from FreeCAD import Console as FCC


if FreeCAD.GuiUp:
    from draftutils.translate import translate
else:
    def translate(context, txt):
        return txt

if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open

useDraftWire = True


def open(filename):
    """Open filename and parse.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.

    Returns
    -------
    App::Document
        The new FreeCAD document object created, with the parsed information.
    """
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    process(filename)
    doc.recompute()


def insert(filename, docname):
    """Get an active document and parse.

    If no document exists, it is created.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.
    docname : str
        The name of the active App::Document if one exists, or
        of the new one created.

    Returns
    -------
    App::Document
        The active FreeCAD document, or the document created if none exists,
        with the parsed information.
    """
    groupname = os.path.splitext(os.path.basename(filename))[0]
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    obj = process(filename)
    if obj is not None:
        importgroup = doc.addObject("App::DocumentObjectGroup", groupname)
        importgroup.Group = [obj]
    doc.recompute()


def process(filename):
    """Process the filename and create a Draft Wire from the data.

    The common airfoil dat format has many flavors.
    This code should work with almost every dialect.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.

    Returns
    -------
    Part::Part2DObject or None.
        The created Draft Wire object or None if the file contains less
        than 3 points.
    """
    # Regex to identify data rows and throw away unused metadata
    xval = r'(?P<xval>\-?\s*\d*\.*\d*([Ee]\-?\d+)?)'
    yval = r'(?P<yval>\-?\s*\d*\.*\d*([Ee]\-?\d+)?)'
    _regex = r'^\s*' + xval + r'\,?\s*' + yval + r'\s*$'

    regex = re.compile(_regex)
    afile = pythonopen(filename, 'r')
    # read the airfoil name which is always at the first line
    airfoilname = afile.readline().strip()

    coords = []
    # upside = True
    # last_x = None

    # Collect the data
    for lin in afile:
        curdat = regex.match(lin)
        if (curdat is not None
                and curdat.group("xval")
                and curdat.group("yval")):
            x = float(curdat.group("xval"))
            y = float(curdat.group("yval"))

            # Some files specify the number of upper and lower points on the 2nd line:
            # "       67.       72."
            # See: http://airfoiltools.com/airfoil
            # This line must be skipped:
            if x < 2 and y < 2:
                # the normal processing
                coords.append(Vector(x, y, 0))

    afile.close()

    if len(coords) < 3:
        FCC.PrintError(translate("ImportAirfoilDAT", "Did not find enough coordinates") + "\n")
        return None

    # sometimes coords are divided in upper an lower side
    # so that x-coordinate begin new from leading or trailing edge
    # check for start coordinates in the middle of list
    if coords[0:-1].count(coords[0]) > 1:
        flippoint = coords.index(coords[0], 1)
        upper = coords[0:flippoint]
        lower = coords[flippoint+1:]
        lower.reverse()
        for i in lower:
            upper.append(i)
        coords = upper

    # do we use the parametric Draft Wire?
    if useDraftWire:
        obj = Draft.make_wire(coords, True)
        # obj.label = airfoilname
    else:
        # alternate solution, uses common Part Faces
        lines = []
        first_v = None
        last_v = None
        for v in coords:
            if first_v is None:
                first_v = v

            # Line between v and last_v if they're not equal
            if (last_v is not None) and (last_v != v):
                lines.append(Part.makeLine(last_v, v))

            # The new last_v
            last_v = v

        # close the wire if needed
        if last_v != first_v:
            lines.append(Part.makeLine(last_v, first_v))

        wire = Part.Wire(lines)
        face = Part.Face(wire)
        obj = FreeCAD.ActiveDocument.addObject('Part::Feature', airfoilname)
        obj.Shape = face

    return obj
