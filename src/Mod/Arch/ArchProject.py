# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

import FreeCAD,Draft,ArchComponent,ArchCommands,math,re,datetime,ArchIFC
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
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

__title__="FreeCAD Project"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

def makeProject(objectslist=None, baseobj=None, name="Project"):

    '''makeProject(sites): creates a project aggregating the list of sites.'''

    if not FreeCAD.ActiveDocument:
        return FreeCAD.Console.PrintError("No active document. Aborting\n")

    import Part
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Project")
    obj.Label = translate("Arch", name)
    _Project(obj)
    if FreeCAD.GuiUp:
        _ViewProviderProject(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    return obj

class _CommandProject:

    "the Arch Project command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Site', # TODO: replace with a custom icon
                'MenuText': QT_TRANSLATE_NOOP("Arch_Project", "Project"),
                'Accel': "P, O",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Project", "Creates a project entity aggregating the selected sites.")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
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

    def __init__(self, obj):
        obj.Proxy = self
        self.setProperties(obj)
        obj.IfcType = "Project"

    def setProperties(self, obj):
        ArchIFC.IfcContext.setProperties(self, obj)
        pl = obj.PropertiesList
        if not hasattr(obj,"Group"):
            obj.addExtension("App::GroupExtensionPython", self)

    def onDocumentRestored(self, obj):
        self.setProperties(obj)

class _ViewProviderProject:

    def __init__(self,vobj):
        vobj.Proxy = self
        vobj.addExtension("Gui::ViewProviderGroupExtensionPython", self)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Site_Tree.svg" # TODO: replace with custom icon

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Project', _CommandProject())
