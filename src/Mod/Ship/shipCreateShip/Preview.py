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

import FreeCAD
import FreeCADGui
from FreeCAD import Base
import Part
import Units
from PySide import QtGui, QtCore
from shipUtils import Paths


class Preview(object):
    def __init__(self):
        """Constructor."""
        self.baseLine = None
        self.baseLineLabel = None
        self.reinit()

    def reinit(self):
        """Reinitializate drawer, removing all the previous annotations"""
        self.clean()

    def update(self, L, B, T):
        """Update the 3D view printing the annotations.

        Keyword arguments:
        L -- Selected ship length.
        B -- Selected ship breadth.
        T -- Selected ship draft.
        """
        self.clean()
        # Move to the international system units
        L *= Units.Metre.Value
        B *= Units.Metre.Value
        T *= Units.Metre.Value
        # Draw the base line
        xStart = -0.6 * L
        xEnd = 0.6 * L
        baseLine = Part.makeLine((xStart, 0, 0), (xEnd, 0, 0))
        Part.show(baseLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.baseLine = objs[len(objs) - 1]
        self.baseLine.Label = 'BaseLine'
        try:
            text = str(QtGui.QApplication.translate(
                "ship_create",
                "Base line",
                None,
                QtGui.QApplication.UnicodeUTF8))
        except:
            text = "Base line"
        self.baseLineLabel = DrawText('BaseLineText',
                                      text,
                                      Base.Vector(xEnd, 0, 0))
        # Draw the free surface line
        fsLine = Part.makeLine((xStart, 0, T), (xEnd, 0, T))
        Part.show(fsLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.fsLine = objs[len(objs) - 1]
        self.fsLine.Label = 'FreeSurface'
        try:
            text = str(QtGui.QApplication.translate(
                "ship_create",
                "Free surface",
                None,
                QtGui.QApplication.UnicodeUTF8))
        except:
            text = "Free surface"
        self.fsLineLabel = DrawText('FSText', text, Base.Vector(xEnd, 0, T))
        # Draw the forward perpendicular
        zStart = -0.1 * T
        zEnd = 1.1 * T
        fpLine = Part.makeLine((0.5 * L, 0, zStart), (0.5 * L, 0, zEnd))
        Part.show(fpLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.fpLine = objs[len(objs) - 1]
        self.fpLine.Label = 'ForwardPerpendicular'
        try:
            text = str(QtGui.QApplication.translate(
                "ship_create",
                "Forward perpendicular",
                None,
                QtGui.QApplication.UnicodeUTF8))
        except:
            text = "Forward perpendicular"
        self.fpLineLabel = DrawText('FPText',
                                    text,
                                    Base.Vector(0.5 * L, 0, zEnd))
        # Draw the after perpendicular
        apLine = Part.makeLine((-0.5 * L, 0, zStart), (-0.5 * L, 0, zEnd))
        Part.show(apLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.apLine = objs[len(objs) - 1]
        self.apLine.Label = 'AfterPerpendicular'
        try:
            text = str(QtGui.QApplication.translate(
                "ship_create",
                "After perpendicular",
                None,
                QtGui.QApplication.UnicodeUTF8))
        except:
            text = "After perpendicular"
        self.apLineLabel = DrawText('APText',
                                    text,
                                    Base.Vector(-0.5 * L, 0, zEnd))
        # Draw the amin frame
        amLine = Part.makeLine((0, -0.5 * B, zStart), (0, -0.5 * B, zEnd))
        Part.show(amLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.amLine = objs[len(objs) - 1]
        self.amLine.Label = 'AminFrame'
        try:
            text = str(QtGui.QApplication.translate(
                "ship_create",
                "Main frame",
                None,
                QtGui.QApplication.UnicodeUTF8))
        except:
            text = "Main frame"
        self.amLineLabel = DrawText('AMText',
                                    text,
                                    Base.Vector(0, -0.5 * B, zEnd))

    def clean(self):
        """Remove all previous annotations from screen."""
        if not self.baseLine:
            return
        FreeCAD.ActiveDocument.removeObject(self.baseLine.Name)
        FreeCAD.ActiveDocument.removeObject(self.baseLineLabel.Name)
        FreeCAD.ActiveDocument.removeObject(self.fsLine.Name)
        FreeCAD.ActiveDocument.removeObject(self.fsLineLabel.Name)
        FreeCAD.ActiveDocument.removeObject(self.fpLine.Name)
        FreeCAD.ActiveDocument.removeObject(self.fpLineLabel.Name)
        FreeCAD.ActiveDocument.removeObject(self.apLine.Name)
        FreeCAD.ActiveDocument.removeObject(self.apLineLabel.Name)
        FreeCAD.ActiveDocument.removeObject(self.amLine.Name)
        FreeCAD.ActiveDocument.removeObject(self.amLineLabel.Name)


def DrawText(name,
             string,
             position,
             displayMode="Screen",
             angle=0.0,
             justification="Left",
             colour=(0.0, 0.0, 0.0),
             size=12):
    """Draw a text in the screen.

    Keyword arguments:
    name -- Name (label) of the object to generate.
    string -- Text to draw (it is strongly recommended to use format u'').
    position -- Point to draw the text.
    angle -- Counter clockwise rotation of text.
    justification -- Alignement of the text ("Left", "Right" or "Center").
    colour -- Colour of the text.
    size -- Font size (in points pt).

    Returns:
    A FreeCAD annotation object
    """
    # Create the object
    text = FreeCAD.ActiveDocument.addObject("App::Annotation", name)
    # Set the text
    text.LabelText = [string, u'']
    # Set the options
    text.Position = position
    doc = FreeCADGui.ActiveDocument
    doc.getObject(text.Name).Rotation = angle
    doc.getObject(text.Name).Justification = justification
    doc.getObject(text.Name).FontSize = size
    doc.getObject(text.Name).TextColor = colour
    doc.getObject(text.Name).DisplayMode = displayMode
    return FreeCAD.ActiveDocument.getObject(text.Name)
