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

import FreeCAD,Draft,ArchCommands,ArchFloor,math,re,datetime
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchSite
#  \ingroup ARCH
#  \brief The Site object and tools
#
#  This module provides tools to build Site objects.
#  Sites are containers for Arch objects, and also define a
#  terrain surface

__title__="FreeCAD Site"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"



def makeSite(objectslist=None,baseobj=None,name="Site"):

    '''makeBuilding(objectslist): creates a site including the
    objects from the given list.'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import Part
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Site")
    obj.Label = translate("Arch",name)
    _Site(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSite(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    if baseobj:
        import Part
        if isinstance(baseobj,Part.Shape):
            obj.Shape = baseobj
        else:
            obj.Terrain = baseobj
    return obj


def makeSolarDiagram(longitude,latitude,scale=1,complete=False):

    """makeSolarDiagram(longitude,latitude,[scale,complete]):
    returns a solar diagram as a pivy node. If complete is
    True, the 12 months are drawn"""

    from subprocess import call
    py3_failed = call(["python3", "-c", "import Pysolar"])

    if py3_failed:
        try:
            import Pysolar
        except:
            print("Pysolar is not installed. Unable to generate solar diagrams")
            return None
    else:
        from subprocess import check_output

    from pivy import coin

    if not scale:
        return None

    def toNode(shape):
        "builds a pivy node from a simple linear shape"
        from pivy import coin
        buf = shape.writeInventor(2,0.01)
        buf = buf.replace("\n","")
        pts = re.findall("point \[(.*?)\]",buf)[0]
        pts = pts.split(",")
        pc = []
        for p in pts:
            v = p.strip().split()
            pc.append([float(v[0]),float(v[1]),float(v[2])])
        coords = coin.SoCoordinate3()
        coords.point.setValues(0,len(pc),pc)
        line = coin.SoLineSet()
        line.numVertices.setValue(-1)
        item = coin.SoSeparator()
        item.addChild(coords)
        item.addChild(line)
        return item

    circles = []
    sunpaths = []
    hourpaths = []
    circlepos = []
    hourpos = []

    # build the base circle + number positions
    import Part
    for i in range(1,9):
        circles.append(Part.makeCircle(scale*(i/8.0)))
    for ad in range(0,360,15):
        a = math.radians(ad)
        p1 = FreeCAD.Vector(math.cos(a)*scale,math.sin(a)*scale,0)
        p2 = FreeCAD.Vector(math.cos(a)*scale*0.125,math.sin(a)*scale*0.125,0)
        p3 = FreeCAD.Vector(math.cos(a)*scale*1.08,math.sin(a)*scale*1.08,0)
        circles.append(Part.LineSegment(p1,p2).toShape())
        circlepos.append((ad,p3))

    # build the sun curves at solstices and equinoxe
    year = datetime.datetime.now().year
    hpts = [ [] for i in range(24) ]
    m = [(6,21),(7,21),(8,21),(9,21),(10,21),(11,21),(12,21)]
    if complete:
        m.extend([(1,21),(2,21),(3,21),(4,21),(5,21)])
    for i,d in enumerate(m):
        pts = []
        for h in range(24):
            if not py3_failed:
                dt = "datetime.datetime(%s, %s, %s, %s)" % (year, d[0], d[1], h)
                alt_call = "python3 -c 'import datetime,Pysolar; print (Pysolar.solar.get_altitude_fast(%s, %s, %s))'" % (latitude, longitude, dt)
                alt = math.radians(float(check_output(alt_call, shell=True).strip()))
                az_call = "python3 -c 'import datetime,Pysolar; print (Pysolar.solar.get_azimuth(%s, %s, %s))'" % (latitude, longitude, dt)
                az = float(re.search('.+$', check_output(az_call, shell=True)).group(0))
            else:
                dt = datetime.datetime(year,d[0],d[1],h)
                alt = math.radians(Pysolar.solar.GetAltitudeFast(latitude,longitude,dt))
                az = Pysolar.solar.GetAzimuth(latitude,longitude,dt)
            az = -90 + az # pysolar's zero is south
            if az < 0:
                az = 360 + az
            az = math.radians(az)
            zc = math.sin(alt)*scale
            ic = math.cos(alt)*scale
            xc = math.cos(az)*ic
            yc = math.sin(az)*ic
            p = FreeCAD.Vector(xc,yc,zc)
            pts.append(p)
            hpts[h].append(p)
            if i in [0,6]:
                ep = FreeCAD.Vector(p)
                ep.multiply(1.08)
                if ep.z >= 0:
                    if h == 12:
                        if i == 0:
                            h = "SUMMER"
                        else:
                            h = "WINTER"
                        if latitude < 0:
                            if h == "SUMMER":
                                h = "WINTER"
                            else:
                                h = "SUMMER"
                    hourpos.append((h,ep))
        if i < 7:
            sunpaths.append(Part.makePolygon(pts))
    for h in hpts:
        if complete:
            h.append(h[0])
        hourpaths.append(Part.makePolygon(h))

    # cut underground lines
    sz = 2.1*scale
    cube = Part.makeBox(sz,sz,sz)
    cube.translate(FreeCAD.Vector(-sz/2,-sz/2,-sz))
    sunpaths = [sp.cut(cube) for sp in sunpaths]
    hourpaths = [hp.cut(cube) for hp in hourpaths]

    # build nodes
    ts = 0.005*scale # text scale
    mastersep = coin.SoSeparator()
    circlesep = coin.SoSeparator()
    numsep = coin.SoSeparator()
    pathsep = coin.SoSeparator()
    hoursep = coin.SoSeparator()
    hournumsep = coin.SoSeparator()
    mastersep.addChild(circlesep)
    mastersep.addChild(numsep)
    mastersep.addChild(pathsep)
    mastersep.addChild(hoursep)
    for item in circles:
        circlesep.addChild(toNode(item))
    for item in sunpaths:
        for w in item.Edges:
            pathsep.addChild(toNode(w))
    for item in hourpaths:
        for w in item.Edges:
            hoursep.addChild(toNode(w))
    for p in circlepos:
        text = coin.SoText2()
        s = p[0]-90
        s = -s
        if s > 360:
            s = s - 360
        if s < 0:
            s = 360 + s
        if s == 0:
            s = "N"
        elif s == 90:
            s = "E"
        elif s == 180:
            s = "S"
        elif s == 270:
            s = "W"
        else:
            s = str(s)
        text.string = s
        text.justification = coin.SoText2.CENTER
        coords = coin.SoTransform()
        coords.translation.setValue([p[1].x,p[1].y,p[1].z])
        coords.scaleFactor.setValue([ts,ts,ts])
        item = coin.SoSeparator()
        item.addChild(coords)
        item.addChild(text)
        numsep.addChild(item)
    for p in hourpos:
        text = coin.SoText2()
        s = str(p[0])
        text.string = s
        text.justification = coin.SoText2.CENTER
        coords = coin.SoTransform()
        coords.translation.setValue([p[1].x,p[1].y,p[1].z])
        coords.scaleFactor.setValue([ts,ts,ts])
        item = coin.SoSeparator()
        item.addChild(coords)
        item.addChild(text)
        numsep.addChild(item)
    return mastersep


class _CommandSite:

    "the Arch Site command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Site',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Site","Site"),
                'Accel': "S, I",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Site","Creates a site object including selected objects.")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        link = p.GetBool("FreeLinking",False)
        siteobj = []
        warning = False
        for obj in sel :
            if (Draft.getType(obj) == "Building") or (hasattr(obj,"IfcRole") and obj.IfcRole == "Building"):
                siteobj.append(obj)
            else :
                if link == True :
                    siteobj.append(obj)
                else:
                    warning = True
        if warning :
            message = translate( "Arch" ,  "Please either select only Building objects or nothing at all!\n\
Site is not allowed to accept any other object besides Building.\n\
Other objects will be removed from the selection.\n\
Note: You can change that in the preferences.")
            ArchCommands.printMessage( message )
        if sel and len(siteobj) == 0:
            message = translate( "Arch" ,  "There is no valid object in the selection.\n\
Site creation aborted.") + "\n"
            ArchCommands.printMessage( message )
        else :
            ss = "[ "
            for o in siteobj:
                ss += "FreeCAD.ActiveDocument." + o.Name + ", "
            ss += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Site"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("obj = Arch.makeSite("+ss+")")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()



class _Site(ArchFloor._Floor):

    "The Site object"

    def __init__(self,obj):

        ArchFloor._Floor.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcRole = "Site"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Terrain" in pl:
            obj.addProperty("App::PropertyLink","Terrain","Site",QT_TRANSLATE_NOOP("App::Property","The base terrain of this site"))
        if not "Address" in pl:
            obj.addProperty("App::PropertyString","Address","Site",QT_TRANSLATE_NOOP("App::Property","The street and house number of this site, with postal box or apartment number if needed"))
        if not "PostalCode" in pl:
            obj.addProperty("App::PropertyString","PostalCode","Site",QT_TRANSLATE_NOOP("App::Property","The postal or zip code of this site"))
        if not "City" in pl:
            obj.addProperty("App::PropertyString","City","Site",QT_TRANSLATE_NOOP("App::Property","The city of this site"))
        if not "Region" in pl:
            obj.addProperty("App::PropertyString","Region","Site",QT_TRANSLATE_NOOP("App::Property","The region, province or county of this site"))
        if not "Country" in pl:
            obj.addProperty("App::PropertyString","Country","Site",QT_TRANSLATE_NOOP("App::Property","The country of this site"))
        if not "Latitude" in pl:
            obj.addProperty("App::PropertyFloat","Latitude","Site",QT_TRANSLATE_NOOP("App::Property","The latitude of this site"))
        if not "Longitude" in pl:
            obj.addProperty("App::PropertyFloat","Longitude","Site",QT_TRANSLATE_NOOP("App::Property","The latitude of this site"))
        if not "Declination" in pl:
            obj.addProperty("App::PropertyAngle","Declination","Site",QT_TRANSLATE_NOOP("App::Property","Angle between the true North and the North direction in this document"))
        if "NorthDeviation"in pl:
            obj.Declination = obj.NorthDeviation.Value
            obj.removeProperty("NorthDeviation")
        if not "Elevation" in pl:
            obj.addProperty("App::PropertyLength","Elevation","Site",QT_TRANSLATE_NOOP("App::Property","The elevation of level 0 of this site"))
        if not "Url" in pl:
            obj.addProperty("App::PropertyString","Url","Site",QT_TRANSLATE_NOOP("App::Property","A url that shows this site in a mapping website"))
        if not "Additions" in pl:
            obj.addProperty("App::PropertyLinkList","Additions","Site",QT_TRANSLATE_NOOP("App::Property","Other shapes that are appended to this object"))
        if not "Subtractions" in pl:
            obj.addProperty("App::PropertyLinkList","Subtractions","Site",QT_TRANSLATE_NOOP("App::Property","Other shapes that are subtracted from this object"))
        if not "ProjectedArea" in pl:
            obj.addProperty("App::PropertyArea","ProjectedArea","Site",QT_TRANSLATE_NOOP("App::Property","The area of the projection of this object onto the XY plane"))
        if not "Perimeter" in pl:
            obj.addProperty("App::PropertyLength","Perimeter","Site",QT_TRANSLATE_NOOP("App::Property","The perimeter length of this terrain"))
        if not "AdditionVolume" in pl:
            obj.addProperty("App::PropertyVolume","AdditionVolume","Site",QT_TRANSLATE_NOOP("App::Property","The volume of earth to be added to this terrain"))
        if not "SubtractionVolume" in pl:
            obj.addProperty("App::PropertyVolume","SubtractionVolume","Site",QT_TRANSLATE_NOOP("App::Property","The volume of earth to be removed from this terrain"))
        if not "ExtrusionVector" in pl:
            obj.addProperty("App::PropertyVector","ExtrusionVector","Site",QT_TRANSLATE_NOOP("App::Property","An extrusion vector to use when performing boolean operations"))
            obj.ExtrusionVector = FreeCAD.Vector(0,0,-100000)
        if not "RemoveSplitter" in pl:
            obj.addProperty("App::PropertyBool","RemoveSplitter","Site",QT_TRANSLATE_NOOP("App::Property","Remove splitters from the resulting shape"))
        if not "OriginOffset" in pl:
            obj.addProperty("App::PropertyVector","OriginOffset","Site",QT_TRANSLATE_NOOP("App::Property","An optional offset between the model (0,0,0) origin and the point indicated by the geocoordinates"))
        if not hasattr(obj,"Group"):
            obj.addExtension("App::GroupExtensionPython", self)
        self.Type = "Site"
        obj.setEditorMode('Height',2)

    def onDocumentRestored(self,obj):

        ArchFloor._Floor.onDocumentRestored(self,obj)
        self.setProperties(obj)

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
                        print("Error computing areas for ",obj.Label)
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
        if obj.Terrain.Shape.Solids:
            shapesolid = obj.Terrain.Shape.copy()
        else:
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
        vobj.addExtension("Gui::ViewProviderGroupExtensionPython", self)
        self.setProperties(vobj)

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "SolarDiagram" in pl:
            vobj.addProperty("App::PropertyBool","SolarDiagram","Site",QT_TRANSLATE_NOOP("App::Property","Show solar diagram or not"))
        if not "SolarDiagramScale" in pl:
            vobj.addProperty("App::PropertyFloat","SolarDiagramScale","Site",QT_TRANSLATE_NOOP("App::Property","The scale of the solar diagram"))
            vobj.SolarDiagramScale = 1
        if not "SolarDiagramPosition" in pl:
            vobj.addProperty("App::PropertyVector","SolarDiagramPosition","Site",QT_TRANSLATE_NOOP("App::Property","The position of the solar diagram"))
        if not "SolarDiagramColor" in pl:
            vobj.addProperty("App::PropertyColor","SolarDiagramColor","Site",QT_TRANSLATE_NOOP("App::Property","The color of the solar diagram"))
            vobj.SolarDiagramColor = (0.16,0.16,0.25)

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

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

    def attach(self,vobj):

        ArchFloor._ViewProviderFloor.attach(self,vobj)
        from pivy import coin
        self.diagramsep = coin.SoSeparator()
        self.color = coin.SoBaseColor()
        self.coords = coin.SoTransform()
        self.diagramswitch = coin.SoSwitch()
        self.diagramswitch.whichChild = -1
        self.diagramswitch.addChild(self.diagramsep)
        self.diagramsep.addChild(self.coords)
        self.diagramsep.addChild(self.color)
        vobj.Annotation.addChild(self.diagramswitch)

    def updateData(self,obj,prop):

        if prop in ["Longitude","Latitude"]:
            self.onChanged(obj.ViewObject,"SolarDiagram")
        elif prop == "Declination":
            self.onChanged(obj.ViewObject,"SolarDiagramPosition")

    def onChanged(self,vobj,prop):

        if prop == "SolarDiagramPosition":
            if hasattr(vobj,"SolarDiagramPosition"):
                p = vobj.SolarDiagramPosition
                self.coords.translation.setValue([p.x,p.y,p.z])
            if hasattr(vobj.Object,"Declination"):
                from pivy import coin
                self.coords.rotation.setValue(coin.SbVec3f((0,0,1)),math.radians(vobj.Object.Declination.Value))
        elif prop == "SolarDiagramColor":
            if hasattr(vobj,"SolarDiagramColor"):
                l = vobj.SolarDiagramColor
                self.color.rgb.setValue([l[0],l[1],l[2]])
        elif "SolarDiagram" in prop:
            if hasattr(self,"diagramnode"):
                self.diagramsep.removeChild(self.diagramnode)
                del self.diagramnode
            if hasattr(vobj,"SolarDiagram") and hasattr(vobj,"SolarDiagramScale"):
                if vobj.SolarDiagram:
                    self.diagramnode = makeSolarDiagram(vobj.Object.Longitude,vobj.Object.Latitude,vobj.SolarDiagramScale)
                    if self.diagramnode:
                        self.diagramsep.addChild(self.diagramnode)
                        self.diagramswitch.whichChild = 0
                    else:
                        del self.diagramnode
                else:
                    self.diagramswitch.whichChild = -1


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Site',_CommandSite())
