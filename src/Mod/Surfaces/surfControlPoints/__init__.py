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
import FreeCAD
import FreeCADGui
from FreeCAD import Base
from FreeCAD import Part

# Qt libraries
from PyQt4 import QtGui,QtCore

# Main object
import TaskPanel
from surfUtils import Geometry, Translator
import Instance

def isValidObject(obj):
    """ Check if object can use control points.
    @param obj Object to test
    @return True if control points can be performed, False otherwise
    """
    # Ensure that right object has been provided
    if not obj.isDerivedFrom('Part::Feature'):
        return False
    faces = Geometry.getFaces(obj)
    if not faces:
        return False
    return True

def load():
    """ Show control points over selected geometry. """
    # Get selected objects
    objs = FreeCADGui.Selection.getSelection()
    # Perform control points over valid objects
    flag = False
    for i in range(0,len(objs)):
        obj = objs[i]
        if not isValidObject(obj):
            continue
        flag = True
        # Create control points object
        ctrl = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", obj.Label + "ControlPoints")
        inst = Instance.ControlPoints(ctrl, obj)
        Instance.ViewProviderShip(ctrl.ViewObject)
    if flag:
        TaskPanel.createTask()
        return
    # Destroy control points
    for i in range(0,len(objs)):
        obj = objs[i]
        props = obj.PropertiesList
        try:
            props.index("ValidCtrlPoints")
        except ValueError:
            continue
        if not obj.ValidCtrlPoints:
            continue
        flag = True
        # restore face object properties
        face = obj.Object
        FreeCADGui.ActiveDocument.getObject(face).Selectable = True
        FreeCADGui.ActiveDocument.getObject(face).Transparency = 0
        # remove control points object
        FreeCAD.ActiveDocument.removeObject(obj.Name)
    # Show errors
    if not flag:
        TaskPanel.createTask()

