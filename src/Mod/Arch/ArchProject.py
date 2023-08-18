# -*- coding: utf8 -*-
#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module provides tools to build Project objects.  Project objects are
objects specifically for better IFC compatibility, allowing the user to tweak
certain IFC relevant values.
"""

import FreeCAD
import ArchIFC
import ArchIFCView
if FreeCAD.GuiUp:
    import FreeCADGui
    from draftutils.translate import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt

## @package ArchProject
#  \ingroup ARCH
#  \brief The Project object and tools
#
#  This module provides tools to build Project objects.

__title__  = "FreeCAD Project"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

def makeProject(sites=None, name=None):
    """Create an Arch project.

    If sites are provided, add them as children of the new project.

    Parameters
    ----------
    sites: list of <Part::FeaturePython>, optional
        Sites to add as children of the project. Ultimately this could be
        anything, however.
    name: str, optional
        The label for the project.

    Returns
    -------
    <Part::FeaturePython>
        The created project.
    """

    if not FreeCAD.ActiveDocument:
        return FreeCAD.Console.PrintError("No active document. Aborting\n")

    import Part
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Project")
    obj.Label = name if name else translate("Arch", "Project")
    _Project(obj)
    if FreeCAD.GuiUp:
        _ViewProviderProject(obj.ViewObject)
    if sites:
        obj.Group = sites
    return obj

class _CommandProject:
    """The command definition for the Arch workbench's gui tool, Arch Project.

    A tool for creating Arch projects.

    Creates a project from the objects selected by the user that have the Site
    IfcType, if any.

    Find documentation on the end user usage of Arch Project here:
    https://wiki.freecad.org/Arch_Project
    """

    def GetResources(self):
        """Return a dictionary with the visual aspects of the Arch Project tool."""
        return {'Pixmap'  : 'Arch_Project',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Project", "Project"),
                'Accel': "P, O",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Project", "Creates a project entity aggregating the selected sites.")}

    def IsActive(self):
        """Determine whether or not the Arch Project tool is active.

        Inactive commands are indicated by a greyed-out icon in the menus and toolbars.
        """
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        """Executed when Arch Project is called.

        Create a project from the objects selected by the user that have the
        Site IfcType, if any.
        """

        selection = FreeCADGui.Selection.getSelection()
        siteobj = []

        for obj in selection:
            if hasattr(obj, "IfcType") and obj.IfcType == "Site":
                siteobj.append(obj)

        ss = "[ "
        for o in siteobj:
            ss += "FreeCAD.ActiveDocument." + o.Name + ", "
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Project"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("obj = Arch.makeProject("+ss+")")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

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
        self.Type = "Project"

    def onDocumentRestored(self, obj):
        """Method run when the document is restored. Re-add the properties."""
        self.setProperties(obj)

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

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Project', _CommandProject())
