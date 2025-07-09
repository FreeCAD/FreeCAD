# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__  = "FreeCAD Project"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package ArchProject
#  \ingroup ARCH
#  \brief The Project object and tools
#
#  This module provides tools to build Project objects.

"""This module provides tools to build Project objects.  Project objects are
objects specifically for better IFC compatibility, allowing the user to tweak
certain IFC relevant values.
"""

import FreeCAD
import ArchIFC
import ArchIFCView

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt


class _Project(ArchIFC.IfcContext):
    """The project object.

    Takes a <Part::FeaturePython>, and turns it into a Project. Then takes a
    list of Arch sites to own as its children.

    Parameters
    ----------
    obj: <App::DocumentObjectGroupPython> or <App::FeaturePython>
        The object to turn into a Project.
    """

    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "Project"
        self.setProperties(obj)
        obj.IfcType = "Project"

    def setProperties(self, obj):
        """Give the object properties unique to projects.

        Add the IFC context properties, and the group extension if it does not
        already exist.
        """

        ArchIFC.IfcContext.setProperties(self, obj)
        pl = obj.PropertiesList
        if not hasattr(obj,"Group"):
            obj.addExtension("App::GroupExtensionPython")

    def onDocumentRestored(self, obj):
        """Method run when the document is restored. Re-add the properties."""
        self.setProperties(obj)

    def dumps(self):

        return None

    def loads(self,state):

        self.Type = "Project"

    def addObject(self,obj,child):

        "Adds an object to the group of this BuildingPart"

        if not child in obj.Group:
            g = obj.Group
            g.append(child)
            obj.Group = g

class _ViewProviderProject(ArchIFCView.IfcContextView):
    """A View Provider for the project object.

    Parameters
    ----------
    vobj: <Gui.ViewProviderDocumentObject>
        The view provider to turn into a project view provider.
    """

    def __init__(self,vobj):
        vobj.Proxy = self
        vobj.addExtension("Gui::ViewProviderGroupExtensionPython")

    def getIcon(self):
        """Return the path to the appropriate icon.

        Returns
        -------
        str
            Path to the appropriate icon .svg file.
        """

        import Arch_rc
        return ":/icons/Arch_Project_Tree.svg"

    def removeDisplaymodeChildNodes(self,vobj):
        """Remove all child nodes from the 4 default display modes.

        This avoids 'ghosts' of the objects in the Group property.
        See:
        ArchSite.py
        https://forum.freecad.org/viewtopic.php?f=10&t=74731
        """

        from pivy import coin
        from draftutils import gui_utils

        if not hasattr(self, "displaymodes_cleaned"):
            if vobj.RootNode.getNumChildren():
                main_switch = gui_utils.find_coin_node(vobj.RootNode, coin.SoSwitch)  # The display mode switch.
                if main_switch is not None and main_switch.getNumChildren() == 4:  # Check if all display modes are available.
                    for node in tuple(main_switch.getChildren()):
                        node.removeAllChildren()
                    self.displaymodes_cleaned = True

    def onChanged(self,vobj,prop):
        self.removeDisplaymodeChildNodes(vobj)
