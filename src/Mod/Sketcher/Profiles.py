# ***************************************************************************
# *   Copyright (c) 2014 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore

    # s=os.path.dirname(__file__)
    # s=os.path.join(s,"ProfileLib")
    # FreeCADGui.addIconPath(s)

__title__ = "Sketcher profile lib handling"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecad.org"


def isProfileActive():
    return not FreeCAD.ActiveDocument is None


def getSketch():
    edit = FreeCADGui.ActiveDocument.getInEdit()
    if edit and edit.isDerivedFrom("SketcherGui::ViewProviderSketch"):
        return edit.Object
    # act = FreeCAD.ActiveDocument.ActiveObject
    # if act and act.isDerivedFrom('Sketcher::SketchObject'):
    #    return act
    return None


class _CommandProfileHexagon1:
    "The basis hexagon profile command definition"

    def GetResources(self):
        return {
            "Pixmap": "Sketcher_CreateHexagon",
            "MenuText": QtCore.QT_TRANSLATE_NOOP(
                "Sketcher_ProfilesHexagon1", "Creates a hexagonal profile"
            ),
            "Accel": "",
            "CmdType": "ForEdit",
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Sketcher_ProfilesHexagon1", "Creates a hexagonal profile in the sketch"
            ),
        }

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create hexagon profile")
        FreeCADGui.addModule("ProfileLib.Hexagon")
        sketch = getSketch()
        if not sketch is None:
            FreeCADGui.doCommand("ProfileLib.Hexagon.makeHexagonSimple('%s')" % (sketch.Name))
        else:
            FreeCADGui.doCommand("ProfileLib.Hexagon.makeHexagonSimple()")
        FreeCAD.ActiveDocument.recompute()

    def IsActive(self):
        return isProfileActive()


FreeCADGui.addCommand("Sketcher_ProfilesHexagon1", _CommandProfileHexagon1())
