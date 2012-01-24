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

# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Base
from FreeCAD import Part
# FreeCADShip modules
from shipUtils import Paths, Translator

class Preview(object):
    def __init__(self):
        """ Constructor.
        """
        self.baseLine = None
        self.baseLineLabel = None
        self.reinit()

    def reinit(self):
        """ Reinitializate drawer.
        """
        self.clean()

    def update(self, L, B, T):
        """ Update the 3D view printing annotations.
        @param L Ship length.
        @param B Ship beam.
        @param T Ship draft.
        """
        # Destroy all previous entities
        self.clean()
        # Draw base line
        xStart   = -0.6*L;
        xEnd     =  0.6*L;
        baseLine = Part.makeLine((xStart,0,0),(xEnd,0,0))
        Part.show(baseLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.baseLine = objs[len(objs)-1]
        self.baseLine.Label = 'BaseLine'
        self.baseLineLabel = DrawText('BaseLineText', str(Translator.translate('Base line')), Base.Vector(xEnd,0,0))
        # Draw free surface
        fsLine = Part.makeLine((xStart,0,T),(xEnd,0,T))
        Part.show(fsLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.fsLine = objs[len(objs)-1]
        self.fsLine.Label = 'FreeSurface'
        self.fsLineLabel = DrawText('FSText', str(Translator.translate('Free surface')), Base.Vector(xEnd,0,T))
        # Draw forward perpendicular
        zStart = -0.1*T
        zEnd   =  1.1*T
        fpLine = Part.makeLine((0.5*L,0,zStart),(0.5*L,0,zEnd))
        Part.show(fpLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.fpLine = objs[len(objs)-1]
        self.fpLine.Label = 'ForwardPerpendicular'
        self.fpLineLabel = DrawText('FPText', str(Translator.translate('Forward perpendicular')), Base.Vector(0.5*L,0,zEnd))
        # Draw after perpendicular
        apLine = Part.makeLine((-0.5*L,0,zStart),(-0.5*L,0,zEnd))
        Part.show(apLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.apLine = objs[len(objs)-1]
        self.apLine.Label = 'AfterPerpendicular'
        self.apLineLabel = DrawText('APText', str(Translator.translate('After perpendicular')), Base.Vector(-0.5*L,0,zEnd))
        # Draw amin frame
        amLine = Part.makeLine((0,-0.5*B,zStart),(0,-0.5*B,zEnd))
        Part.show(amLine)
        objs = FreeCAD.ActiveDocument.Objects
        self.amLine = objs[len(objs)-1]
        self.amLine.Label = 'AminFrame'
        self.amLineLabel = DrawText('AMText', str(Translator.translate('Amin frame')), Base.Vector(0,-0.5*B,zEnd))
        
    def clean(self):
        """ Erase all annotations from screen.
        """
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

def DrawText(name, string, position, displayMode="Screen", angle=0.0, justification="Left", colour=(0.00,0.00,0.00), size=12):
    """ Draws a text in a desired position.
    @param name Name of the object
    @param string Text to draw (recommended format u'')    
    @param position Point to draw the text
    @param angle Counter clockwise rotation of text
    @param justification Alignement of the text ("Left", "Right" or "Center")
    @param colour Colour of the text
    @param size Font size
    @return FreeCAD annotation object
    """
    # Create the object
    text = FreeCAD.ActiveDocument.addObject("App::Annotation",name)
    # Set the text
    text.LabelText = [string, u'']
    # Set the options
    text.Position = position
    FreeCADGui.ActiveDocument.getObject(text.Name).Rotation = angle
    FreeCADGui.ActiveDocument.getObject(text.Name).Justification = justification
    FreeCADGui.ActiveDocument.getObject(text.Name).FontSize = size
    FreeCADGui.ActiveDocument.getObject(text.Name).TextColor = colour
    FreeCADGui.ActiveDocument.getObject(text.Name).DisplayMode = displayMode
    return FreeCAD.ActiveDocument.getObject(text.Name)
