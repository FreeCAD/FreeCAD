# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM base constraint ViewProvider"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package _BaseViewProvider
#  \ingroup FEM
#  \brief FreeCAD _Base ViewProvider for FEM workbench

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView

from pivy import coin

False if FemGui.__name__ else True  # flake8, dummy FemGui usage


class ViewProxy(object):
    """Proxy View Provider for Pythons base constraint."""

    def __init__(self, vobj):
        vobj.Proxy = self

    # needs to be overwritten, if no standard icon name is used
    # see constraint body heat source as an example
    def getIcon(self):
        """after load from FCStd file, self.icon does not exist, return constant path instead"""
        # https://forum.freecadweb.org/viewtopic.php?f=18&t=44009
        if hasattr(self.Object.Proxy, "Type") and self.Object.Proxy.Type.startswith("Fem::"):
            return ":/icons/{}.svg".format(self.Object.Proxy.Type.replace("Fem::", "FEM_"))
        else:
            return ""

    def attach(self, vobj):
        default = coin.SoGroup()
        vobj.addDisplayMode(default, "Default")
        self.Object = vobj.Object  # used on various places, claim childreens, get icon, etc.
        # self.ViewObject = vobj  # not used ATM

    def getDisplayModes(self, obj):
        "Return a list of display modes."
        modes = ["Default"]
        return modes

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self, mode):
        return mode

    def setEdit(self, vobj, mode=0, TaskPanel=None, hide_mesh=True):
        if TaskPanel is None:
            # avoid edit mode by return False
            # https://forum.freecadweb.org/viewtopic.php?t=12139&start=10#p161062
            return False
        if hide_mesh is True:
            # hide all FEM meshes and VTK FemPostPipeline objects
            for o in vobj.Object.Document.Objects:
                if (
                    o.isDerivedFrom("Fem::FemMeshObject")
                    or o.isDerivedFrom("Fem::FemPostPipeline")
                ):
                    o.ViewObject.hide()
        # show task panel
        task = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(task)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode
        # https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = "Active Task Dialog found! Please close this one before opening  a new one!"
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + "\n")
        return True

    # they are needed, see https://forum.freecadweb.org/viewtopic.php?f=18&t=44021
    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
