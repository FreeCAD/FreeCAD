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

import FreeCAD,Draft,ArchCommands,ArchFloor
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Site"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

def makeSite(objectslist=None,baseobj=None,name="Site"):
    '''makeBuilding(objectslist): creates a site including the
    objects from the given list.'''
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython",name)
    obj.Label = translate("Arch",name)
    _Site(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSite(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    if baseobj:
        obj.Terrain = baseobj
    return obj

class _CommandSite:
    "the Arch Site command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Site',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Site","Site"),
                'Accel': "S, I",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Site","Creates a site object including selected objects.")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        link = p.GetBool("FreeLinking",False)
        siteobj = []
        warning = False
        for obj in sel :
            if Draft.getType(obj) == "Building":
                siteobj.append(obj)
            else :
                if link == True :
                    siteobj.append(obj)
                else:
                    warning = True
        if warning :
            message = "Please select only Building objects or nothing!\n\
Site are not allowed to accept other object than Building.\n\
Other objects will be removed from the selection.\n\
You can change that in the preferences."
            self.printMessage( message )
        if sel and len(siteobj) == 0:
            message = "There is no valid object in the selection.\n\
Site creation aborted."
            self.printMessage( message )
        else :
            ss = "[ "
            for o in siteobj:
                ss += "FreeCAD.ActiveDocument." + o.Name + ", "
            ss += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Site"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeSite("+ss+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

    def printMessage(self, message):
        FreeCAD.Console.PrintMessage(translate("Arch", message))
        if FreeCAD.GuiUp :
            reply = QtGui.QMessageBox.information(None,"", message)

class _Site(ArchFloor._Floor):
    "The Site object"
    def __init__(self,obj):
        ArchFloor._Floor.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Terrain","Arch","The terrain of this site")
        obj.addProperty("App::PropertyString","Address","Arch","The street and housenumber of this site")
        obj.addProperty("App::PropertyString","PostalCode","Arch","The postal or zip code of this site")
        obj.addProperty("App::PropertyString","City","Arch","The city of this site")
        obj.addProperty("App::PropertyString","Country","Arch","The country of this site")
        obj.addProperty("App::PropertyFloat","Latitude","Arch","The latitude of this site")
        obj.addProperty("App::PropertyFloat","Longitude","Arch","The latitude of this site")
        obj.addProperty("App::PropertyString","Url","Arch","An url that shows this site in a mapping website")
        self.Type = "Site"
        obj.setEditorMode('Height',2)

class _ViewProviderSite(ArchFloor._ViewProviderFloor):
    "A View Provider for the Site object"
    def __init__(self,vobj):
        ArchFloor._ViewProviderFloor.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Site_Tree.svg"

    def claimChildren(self):
        return self.Object.Group+[self.Object.Terrain]

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Site',_CommandSite())
