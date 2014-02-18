#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
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

from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
from FreeCAD import Base, Vector
import Part
import Units
from shipUtils import Paths


class Preview(object):
    def __init__(self):
        """ Constructor. """
        self.obj = None
        self.reinit()

    def reinit(self):
        """ Reinitializate the drawer. """
        self.clean()

    def update(self, L, B, T, sectionsL, sectionsB, sectionsT, shape):
        """ Update the 3D view annotations.
        @param L Ship Lpp.
        @param B Ship beam.
        @param T Ship draft.
        @param sectionsL Transversal sections.
        @param sectionsB Longitudinal sections.
        @param sectionsT Water lines.
        @param shape Ship surfaces shell
        @return Sections object. None if errors happens.
        """
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Computing sections",
            None,
            QtGui.QApplication.UnicodeUTF8)
        FreeCAD.Console.PrintMessage(msg + '...\n')
        # Destroy all previous entities
        self.clean()
        # Receive data
        nL = len(sectionsL)
        nB = len(sectionsB)
        nT = len(sectionsT)
        if not (nL or nB or nT):
            return None
        # Found sections
        sections = []
        for i in range(0, nL):
            pos = sectionsL[i] * Units.Metre.Value
            # Cut ship
            section = shape.slice(Vector(1.0, 0.0, 0.0), pos)
            for j in range(0, len(section)):
                edges = section[j].Edges
                # We have 3 cases,
                # * when the section is before midship (starboard side drawn)
                # * When the section is midship (both sides drawn)
                # * When the section is after midship (board side drawn)
                if pos > 0.01 * L * Units.Metre.Value:
                    for k in range(len(edges) - 1, -1, -1):
                        edge = edges[k]
                        bbox = edge.BoundBox
                        if bbox.YMin < -0.01 * B * Units.Metre.Value:
                            del edges[k]
                elif pos < -0.01 * L * Units.Metre.Value:
                    for k in range(len(edges) - 1, -1, -1):
                        edge = edges[k]
                        bbox = edge.BoundBox
                        if bbox.YMax > 0.01 * B * Units.Metre.Value:
                            del edges[k]
                sections.extend(edges)
        for i in range(0, nB):
            pos = sectionsB[i] * Units.Metre.Value
            section = shape.slice(Vector(0.0, 1.0, 0.0), pos)
            for j in range(0, len(section)):
                edges = section[j].Edges
                # The longitudinal sections are printed in both sides.
                section[j] = section[j].mirror(Vector(0.0, 0.0, 0.0),
                                               Vector(0.0, 1.0, 0.0))
                edges2 = section[j].Edges
                sections.extend(edges)
                sections.extend(edges2)
        for i in range(0, nT):
            pos = sectionsT[i] * Units.Metre.Value
            section = shape.slice(Vector(0.0, 0.0, 1.0), pos)
            for j in range(0, len(section)):
                edges = section[j].Edges
                # We have 3 cases,
                # * when the section is below draft (starboard side drawn)
                # * When the section is draft (both sides drawn)
                # * When the section is above draft (starboard side drawn)
                if pos > T * 1.01 * Units.Metre.Value:
                    for k in range(len(edges) - 1, -1, -1):
                        edge = edges[k]
                        bbox = edge.BoundBox
                        if bbox.YMax > 0.01 * B * Units.Metre.Value:
                            del edges[k]
                elif pos < T * 0.99 * Units.Metre.Value:
                    for k in range(len(edges) - 1, -1, -1):
                        edge = edges[k]
                        bbox = edge.BoundBox
                        if bbox.YMin < -0.01 * B * Units.Metre.Value:
                            del edges[k]
                sections.extend(edges)
        # Trabform and join all the BSplines into a shape
        if not sections:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Any valid ship section found",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintWarning(msg + '\n')
            return
        obj = sections[0]
        for i in range(1, len(sections)):
            # Just create a group of edges
            obj = obj.oldFuse(sections[i])
        Part.show(obj)
        objs = FreeCAD.ActiveDocument.Objects
        self.obj = objs[len(objs) - 1]
        self.obj.Label = 'OutlineDraw'
        return self.obj

    def clean(self):
        """ Erase all the annotations from the screen.
        """
        if not self.obj:
            return
        FreeCAD.ActiveDocument.removeObject(self.obj.Name)
        self.obj = None
