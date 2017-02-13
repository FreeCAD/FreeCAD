# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
from PathScripts.PathUtils import loopdetect
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Path Commands"
__author__ = "sliptonic"
__url__ = "http://www.freecadweb.org"


class _CommandSelectLoop:
    "the Arch RemoveShape command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Path-SelectLoop',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_SelectLoop","Finish Selecting Loop"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_SelectLoop","Complete loop selection from two edges"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            sel = FreeCADGui.Selection.getSelectionEx()[0]
            sub1 = sel.SubElementNames[0]
            if sub1[0:4] != 'Edge':
                return False
            sub2 = sel.SubElementNames[1]
            if sub2[0:4] != 'Edge':
                return False
            return True
        except:
            return False



    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()[0]
        obj = sel.Object
        edge1 = sel.SubObjects[0]
        edge2 = sel.SubObjects[1]
        loopwire = loopdetect(obj, edge1, edge2)
        if loopwire is not None:
            FreeCADGui.Selection.clearSelection()
            elist = obj.Shape.Edges
            for e in elist:
                for i in loopwire.Edges:
                    if e.hashCode() == i.hashCode():
                        FreeCADGui.Selection.addSelection(obj, "Edge"+str(elist.index(e)+1))

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_SelectLoop',_CommandSelectLoop())

def findShape(shape,subname=None,subtype=None):
    '''To find a higher oder shape containing the subshape with subname.
        E.g. to find the wire containing 'Edge1' in shape,
            findShape(shape,'Edge1','Wires')
    '''
    if not subname:
        return shape
    ret = shape.getElement(subname)
    if not subtype or not ret or ret.isNull():
        return ret;
    if subname.startswith('Face'):
        tp = 'Faces'
    elif subname.startswith('Edge'):
        tp = 'Edges'
    elif subname.startswith('Vertex'):
        tp = 'Vertex'
    else:
        return ret
    for obj in getattr(shape,subtype):
        for sobj in getattr(obj,tp):
            if sobj.isEqual(ret):
                return obj
    return ret
