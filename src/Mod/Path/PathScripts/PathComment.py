# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Used for CNC machine comments for Path module. Create a comment and place it in the Document tree.'''

import FreeCAD
import FreeCADGui
import Path
from PySide import QtCore

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class Comment:

    def __init__(self, obj):
        obj.addProperty("App::PropertyString", "Comment",
                        "Path", "Comment or note for CNC program")
        obj.Proxy = self
        mode = 2
        obj.setEditorMode('Placement', mode)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        pass

    def execute(self, obj):
        output = ""
        output += '(' + str(obj.Comment) + ')\n'
        path = Path.Path(output)
        obj.Path = path


class _ViewProviderComment:

    def __init__(self, vobj):  # mandatory
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        vobj.setEditorMode('Visibility', mode)

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def getIcon(self):  # optional
        return ":/icons/Path-Comment.svg"

    def onChanged(self, vobj, prop):  # optional
        # pylint: disable=unused-argument
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        vobj.setEditorMode('Visibility', mode)


class CommandPathComment:

    def GetResources(self):
        return {'Pixmap': 'Path-Comment',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Comment", "Comment"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Comment", "Add a Comment to your CNC program")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Comment", "Create a Comment in your CNC program"))
        FreeCADGui.addModule("PathScripts.PathComment")
        snippet = '''
import Path
import PathScripts
from PathScripts import PathUtils
obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Comment")
PathScripts.PathComment.Comment(obj)
PathScripts.PathComment._ViewProviderComment(obj.ViewObject)

PathUtils.addToJob(obj)
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Comment', CommandPathComment())


FreeCAD.Console.PrintLog("Loading PathComment... done\n")
