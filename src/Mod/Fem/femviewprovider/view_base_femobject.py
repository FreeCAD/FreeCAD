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
__url__ = "https://www.freecad.org"

## @package view_base_femobject
#  \ingroup FEM
#  \brief view provider as base for all FEM objects


import FreeCAD
import FreeCADGui

import FemGui  # needed to display the icons in TreeView

False if FemGui.__name__ else True  # flake8, dummy FemGui usage


class VPBaseFemObject(object):
    """Proxy View Provider for FEM FeaturePythons base constraint."""

    def __init__(self, vobj):
        vobj.Proxy = self

    # needs to be overwritten, if no standard icon name is used
    # see constraint body heat source as an example
    def getIcon(self):
        """after load from FCStd file, self.icon does not exist, return constant path instead"""
        # https://forum.freecad.org/viewtopic.php?f=18&t=44009
        if not hasattr(self.Object, "Proxy"):
            FreeCAD.Console.PrintMessage("{}, has no Proxy.\n".format(self.Object.Name))
            return ""
        if not hasattr(self.Object.Proxy, "Type"):
            FreeCAD.Console.PrintMessage(
                "{}: Proxy does has not have attribute Type.\n"
                .format(self.Object.Name)
            )
            return ""
        if (
            isinstance(self.Object.Proxy.Type, str)
            and self.Object.Proxy.Type.startswith("Fem::")
        ):
            icon_path = "/icons/{}.svg".format(self.Object.Proxy.Type.replace("Fem::", "FEM_"))
            FreeCAD.Console.PrintLog("{} --> {}\n".format(self.Object.Name, icon_path))
            return ":/{}".format(icon_path)
        else:
            FreeCAD.Console.PrintError("No icon returned for {}\n".format(self.Object.Name))
            FreeCAD.Console.PrintMessage("{}\n".format(self.Object.Proxy.Type))
            return ""

    def attach(self, vobj):
        self.Object = vobj.Object  # used on various places, claim childreens, get icon, etc.
        # self.ViewObject = vobj  # not used ATM

    def setEdit(self, vobj, mode=0, TaskPanel=None, hide_mesh=True):
        if TaskPanel is None:
            # avoid edit mode by return False
            # https://forum.freecad.org/viewtopic.php?t=12139&start=10#p161062
            return False
        if hide_mesh:
            # hide all FEM meshes and FemPost function objects
            for obj in vobj.Object.Document.Objects:
                if (
                    obj.isDerivedFrom("Fem::FemMeshObject")
                    or obj.isDerivedFrom("Fem::FemPostPlaneFunction")
                    or obj.isDerivedFrom("Fem::FemPostSphereFunction")
                ):
                    obj.ViewObject.hide()
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
        # https://forum.freecad.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = "Active Task Dialog found! Please close this one before opening  a new one!"
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + "\n")
        return True

    # they are needed, see:
    # https://forum.freecad.org/viewtopic.php?f=18&t=44021
    # https://forum.freecad.org/viewtopic.php?f=18&t=44009
    def dumps(self):
        return None

    def loads(self, state):
        return None
