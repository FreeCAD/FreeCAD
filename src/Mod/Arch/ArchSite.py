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
    import Part
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
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
            message = translate( "Arch" ,  "Please select only Building objects or nothing!\n\
Site are not allowed to accept other object than Building.\n\
Other objects will be removed from the selection.\n\
You can change that in the preferences." )
            ArchCommands.printMessage( message )
        if sel and len(siteobj) == 0:
            message = translate( "Arch" ,  "There is no valid object in the selection.\n\
Site creation aborted." )
            ArchCommands.printMessage( message )
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



class _Site(ArchFloor._Floor):

    "The Site object"

    def __init__(self,obj):

        ArchFloor._Floor.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Terrain","Arch","The base terrain of this site")
        obj.addProperty("App::PropertyString","Address","Arch","The street and housenumber of this site")
        obj.addProperty("App::PropertyString","PostalCode","Arch","The postal or zip code of this site")
        obj.addProperty("App::PropertyString","City","Arch","The city of this site")
        obj.addProperty("App::PropertyString","Country","Arch","The country of this site")
        obj.addProperty("App::PropertyFloat","Latitude","Arch","The latitude of this site")
        obj.addProperty("App::PropertyFloat","Longitude","Arch","The latitude of this site")
        obj.addProperty("App::PropertyString","Url","Arch","An url that shows this site in a mapping website")
        obj.addProperty("App::PropertyLinkList","Group","Arch","The objects that are part of this site")
        obj.addProperty("App::PropertyLinkList","Additions","Arch","Other shapes that are appended to this object")
        obj.addProperty("App::PropertyLinkList","Subtractions","Arch","Other shapes that are subtracted from this object")
        obj.addProperty("App::PropertyArea","ProjectedArea","Arch","The area of the projection of this object onto the XY plane")
        obj.addProperty("App::PropertyLength","Perimeter","Arch","The perimeter length of this terrain")
        obj.addProperty("App::PropertyVolume","AdditionVolume","Arch","The volume of earth to be added to this terrain")
        obj.addProperty("App::PropertyVolume","SubtractionVolume","Arch","The volume of earth to be removed from this terrain")
        obj.addProperty("App::PropertyVector","ExtrusionVector","Arch","An extrusion vector to use when performing boolean operations")
        obj.addProperty("App::PropertyBool","RemoveSplitter","Arch","Remove splitters from the resulting shape")
        self.Type = "Site"
        obj.setEditorMode('Height',2)
        obj.ExtrusionVector = FreeCAD.Vector(0,0,-100000)

    def execute(self,obj):

        ArchFloor._Floor.execute(self,obj)

        if not obj.isDerivedFrom("Part::Feature"): # old-style Site
            return

        pl = obj.Placement
        shape = None
        if obj.Terrain:
            if obj.Terrain.isDerivedFrom("Part::Feature"):
                if obj.Terrain.Shape:
                    if not obj.Terrain.Shape.isNull():
                        shape = obj.Terrain.Shape.copy()
        if shape:
            shells = []
            for sub in obj.Subtractions:
                if sub.isDerivedFrom("Part::Feature"):
                    if sub.Shape:
                        if sub.Shape.Solids:
                            for sol in sub.Shape.Solids:
                                rest = shape.cut(sol)
                                shells.append(sol.Shells[0].common(shape.extrude(obj.ExtrusionVector)))
                                shape = rest
            for sub in obj.Additions:
                if sub.isDerivedFrom("Part::Feature"):
                    if sub.Shape:
                        if sub.Shape.Solids:
                            for sol in sub.Shape.Solids:
                                rest = shape.cut(sol)
                                shells.append(sol.Shells[0].cut(shape.extrude(obj.ExtrusionVector)))
                                shape = rest
            if not shape.isNull():
                if shape.isValid():
                    for shell in shells:
                        shape = shape.fuse(shell)
                    if obj.RemoveSplitter:
                        shape = shape.removeSplitter()
                    obj.Shape = shape
                    if not pl.isNull():
                        obj.Placement = pl
                    self.computeAreas(obj)

    def onChanged(self,obj,prop):

        ArchFloor._Floor.onChanged(self,obj,prop)
        if prop == "Terrain":
            if obj.Terrain:
                if FreeCAD.GuiUp:
                    obj.Terrain.ViewObject.hide()
                self.execute(obj)

    def computeAreas(self,obj):

        if not obj.Shape:
            return
        if obj.Shape.isNull():
            return
        if not obj.Shape.isValid():
            return
        if not obj.Shape.Faces:
            return
        if not hasattr(obj,"Perimeter"): # check we have a latest version site
            return
        if not obj.Terrain:
            return
        # compute area
        fset = []
        for f in obj.Shape.Faces:
            if f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1)) < 1.5707:
                fset.append(f)
        if fset:
            import Drawing,Part
            pset = []
            for f in fset:
                try:
                    pf = Part.Face(Part.Wire(Drawing.project(f,FreeCAD.Vector(0,0,1))[0].Edges))
                except Part.OCCError:
                    # error in computing the area. Better set it to zero than show a wrong value
                    if obj.ProjectedArea.Value != 0:
                        print "Error computing areas for ",obj.Label
                        obj.ProjectedArea = 0
                else:
                    pset.append(pf)
            if pset:
                self.flatarea = pset.pop()
                for f in pset:
                    self.flatarea = self.flatarea.fuse(f)
                self.flatarea = self.flatarea.removeSplitter()
                if obj.ProjectedArea.Value != self.flatarea.Area:
                    obj.ProjectedArea = self.flatarea.Area
        # compute perimeter
        lut = {}
        for e in obj.Shape.Edges:
            lut.setdefault(e.hashCode(),[]).append(e)
        l = 0
        for e in lut.values():
            if len(e) == 1: # keep only border edges
                l += e[0].Length
        if l:
                if obj.Perimeter.Value != l:
                    obj.Perimeter = l
        # compute volumes
        shapesolid = obj.Terrain.Shape.extrude(obj.ExtrusionVector)
        addvol = 0
        subvol = 0
        for sub in obj.Subtractions:
            subvol += sub.Shape.common(shapesolid).Volume
        for sub in obj.Additions:
            addvol += sub.Shape.cut(shapesolid).Volume
        if obj.SubtractionVolume.Value != subvol:
            obj.SubtractionVolume = subvol
        if obj.AdditionVolume.Value != addvol:
            obj.AdditionVolume = addvol




class _ViewProviderSite(ArchFloor._ViewProviderFloor):

    "A View Provider for the Site object"

    def __init__(self,vobj):
        ArchFloor._ViewProviderFloor.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Site_Tree.svg"

    def claimChildren(self):
        objs = self.Object.Group+[self.Object.Terrain]
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        if hasattr(self.Object,"Additions") and prefs.GetBool("swallowAdditions",True):
            objs.extend(self.Object.Additions)
        if hasattr(self.Object,"Subtractions") and prefs.GetBool("swallowSubtractions",True):
            objs.extend(self.Object.Subtractions)
        return objs

    def setEdit(self,vobj,mode):
        if mode == 0:
            import ArchComponent
            taskd = ArchComponent.ComponentTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
            return True
        return False

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return False


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Site',_CommandSite())
