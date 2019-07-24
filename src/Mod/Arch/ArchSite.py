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

import FreeCAD,Draft,ArchCommands,math,re,datetime,ArchIFC
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


def getSunDirections(longitude,latitude,tz=None):

    """getSunDirections(longitude,latitude,[tz]): returns a list of 9
    directional 3D vectors corresponding to sun direction at 9h, 12h
    and 15h on summer solstice, equinox and winter solstice. Tz is the
    timezone related to UTC (ex: -3 = UTC-3)"""

    oldversion = False
    try:
        import pysolar
    except:
        try:
            import Pysolar as pysolar
        except:
            FreeCAD.Console.PrintError("The pysolar module was not found. Unable to generate solar diagrams\n")
            return None
        else:
            oldversion = True

    if tz:
        tz = datetime.timezone(datetime.timedelta(hours=-3))
    else:
        tz = datetime.timezone.utc

    year = datetime.datetime.now().year
    hpts = [ [] for i in range(24) ]
    m = [(6,21),(9,21),(12,21)]
    pts = []
    for i,d in enumerate(m):
        for h in [9,12,15]:
            if oldversion:
                dt = datetime.datetime(year, d[0], d[1], h)
                alt = math.radians(pysolar.solar.GetAltitudeFast(latitude, longitude, dt))
                az = pysolar.solar.GetAzimuth(latitude, longitude, dt)
                az = -90 + az # pysolar's zero is south, ours is X direction
            else:
                dt = datetime.datetime(year, d[0], d[1], h, tzinfo=tz)
                alt = math.radians(pysolar.solar.get_altitude_fast(latitude, longitude, dt))
                az = pysolar.solar.get_azimuth(latitude, longitude, dt)
                az = 90 + az # pysolar's zero is north, ours is X direction
            if az < 0:
                az = 360 + az
            az = math.radians(az)
            zc = math.sin(alt)
            ic = math.cos(alt)
            xc = math.cos(az)*ic
            yc = math.sin(az)*ic
            p = FreeCAD.Vector(xc,yc,zc).negative()
            p.normalize()
            if not oldversion:
                p.x = -p.x # No idea why that is, empirical find
            pts.append(p)
    return pts


def makeSolarDiagram(longitude,latitude,scale=1,complete=False,tz=None):

    """makeSolarDiagram(longitude,latitude,[scale,complete,tz]):
    returns a solar diagram as a pivy node. If complete is
    True, the 12 months are drawn. Tz is the timezone related to
    UTC (ex: -3 = UTC-3)"""

    oldversion = False
    try:
        import pysolar
    except:
        try:
            import Pysolar as pysolar
        except:
            FreeCAD.Console.PrintError("The pysolar module was not found. Unable to generate solar diagrams\n")
            return None
        else:
            oldversion = True

    from pivy import coin

    if not scale:
        return None

    if tz:
        tz = datetime.timezone(datetime.timedelta(hours=-3))
    else:
        tz = datetime.timezone.utc

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
            if oldversion:
                dt = datetime.datetime(year, d[0], d[1], h)
                alt = math.radians(pysolar.solar.GetAltitudeFast(latitude, longitude, dt))
                az = pysolar.solar.GetAzimuth(latitude, longitude, dt)
                az = -90 + az # pysolar's zero is south, ours is X direction
            else:
                dt = datetime.datetime(year, d[0], d[1], h, tzinfo=tz)
                alt = math.radians(pysolar.solar.get_altitude_fast(latitude, longitude, dt))
                az = pysolar.solar.get_azimuth(latitude, longitude, dt)
                az = 90 + az # pysolar's zero is north, ours is X direction
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
                    if not oldversion:
                        h = 24-h # not sure why this is needed now... But it is.
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

# Values in mm
COMPASS_POINTER_LENGTH = 1000
COMPASS_POINTER_WIDTH = 100


class Compass(object):
    def __init__(self):
        self.rootNode = self.setupCoin()

    def show(self):
        from pivy import coin
        self.compassswitch.whichChild = coin.SO_SWITCH_ALL

    def hide(self):
        from pivy import coin
        self.compassswitch.whichChild = coin.SO_SWITCH_NONE

    def rotate(self, angleInDegrees):
        from pivy import coin
        self.transform.rotation.setValue(
            coin.SbVec3f(0, 0, 1), math.radians(angleInDegrees))

    def locate(self, x,y,z):
        from pivy import coin
        self.transform.translation.setValue(x, y, z)

    def scale(self, area):
        from pivy import coin

        scale = round(max(math.sqrt(area.getValueAs("m^2").Value) / 10, 1))

        self.transform.scaleFactor.setValue(coin.SbVec3f(scale, scale, 1))

    def setupCoin(self):
        from pivy import coin

        compasssep = coin.SoSeparator()

        self.transform = coin.SoTransform()

        darkNorthMaterial = coin.SoMaterial()
        darkNorthMaterial.diffuseColor.set1Value(
            0, 0.5, 0, 0)  # north dark color

        lightNorthMaterial = coin.SoMaterial()
        lightNorthMaterial.diffuseColor.set1Value(
            0, 0.9, 0, 0)  # north light color

        darkGreyMaterial = coin.SoMaterial()
        darkGreyMaterial.diffuseColor.set1Value(0, 0.9, 0.9, 0.9)  # dark color

        lightGreyMaterial = coin.SoMaterial()
        lightGreyMaterial.diffuseColor.set1Value(
            0, 0.5, 0.5, 0.5)  # light color

        coords = self.buildCoordinates()

        # coordIndex = [0, 1, 2, -1, 2, 3, 0, -1]

        lightColorFaceset = coin.SoIndexedFaceSet()
        lightColorCoordinateIndex = [4, 5, 6, -1, 8, 9, 10, -1, 12, 13, 14, -1]
        lightColorFaceset.coordIndex.setValues(
            0, len(lightColorCoordinateIndex), lightColorCoordinateIndex)

        darkColorFaceset = coin.SoIndexedFaceSet()
        darkColorCoordinateIndex = [6, 7, 4, -1, 10, 11, 8, -1, 14, 15, 12, -1]
        darkColorFaceset.coordIndex.setValues(
            0, len(darkColorCoordinateIndex), darkColorCoordinateIndex)

        lightNorthFaceset = coin.SoIndexedFaceSet()
        lightNorthCoordinateIndex = [2, 3, 0, -1]
        lightNorthFaceset.coordIndex.setValues(
            0, len(lightNorthCoordinateIndex), lightNorthCoordinateIndex)

        darkNorthFaceset = coin.SoIndexedFaceSet()
        darkNorthCoordinateIndex = [0, 1, 2, -1]
        darkNorthFaceset.coordIndex.setValues(
            0, len(darkNorthCoordinateIndex), darkNorthCoordinateIndex)

        self.compassswitch = coin.SoSwitch()
        self.compassswitch.whichChild = coin.SO_SWITCH_NONE
        self.compassswitch.addChild(compasssep)

        lightGreySeparator = coin.SoSeparator()
        lightGreySeparator.addChild(lightGreyMaterial)
        lightGreySeparator.addChild(lightColorFaceset)

        darkGreySeparator = coin.SoSeparator()
        darkGreySeparator.addChild(darkGreyMaterial)
        darkGreySeparator.addChild(darkColorFaceset)

        lightNorthSeparator = coin.SoSeparator()
        lightNorthSeparator.addChild(lightNorthMaterial)
        lightNorthSeparator.addChild(lightNorthFaceset)

        darkNorthSeparator = coin.SoSeparator()
        darkNorthSeparator.addChild(darkNorthMaterial)
        darkNorthSeparator.addChild(darkNorthFaceset)

        compasssep.addChild(coords)
        compasssep.addChild(self.transform)
        compasssep.addChild(lightGreySeparator)
        compasssep.addChild(darkGreySeparator)
        compasssep.addChild(lightNorthSeparator)
        compasssep.addChild(darkNorthSeparator)

        return self.compassswitch

    def buildCoordinates(self):
        from pivy import coin

        coords = coin.SoCoordinate3()

        # North Arrow
        coords.point.set1Value(0, 0, 0, 0)
        coords.point.set1Value(1, COMPASS_POINTER_WIDTH,
                               COMPASS_POINTER_WIDTH, 0)
        coords.point.set1Value(2, 0, COMPASS_POINTER_LENGTH, 0)
        coords.point.set1Value(3, -COMPASS_POINTER_WIDTH,
                               COMPASS_POINTER_WIDTH, 0)

        # East Arrow
        coords.point.set1Value(4, 0, 0, 0)
        coords.point.set1Value(
            5, COMPASS_POINTER_WIDTH, -COMPASS_POINTER_WIDTH, 0)
        coords.point.set1Value(6, COMPASS_POINTER_LENGTH, 0, 0)
        coords.point.set1Value(7, COMPASS_POINTER_WIDTH,
                               COMPASS_POINTER_WIDTH, 0)

        # South Arrow
        coords.point.set1Value(8, 0, 0, 0)
        coords.point.set1Value(
            9, -COMPASS_POINTER_WIDTH, -COMPASS_POINTER_WIDTH, 0)
        coords.point.set1Value(10, 0, -COMPASS_POINTER_LENGTH, 0)
        coords.point.set1Value(
            11, COMPASS_POINTER_WIDTH, -COMPASS_POINTER_WIDTH, 0)

        # West Arrow
        coords.point.set1Value(12, 0, 0, 0)
        coords.point.set1Value(13, -COMPASS_POINTER_WIDTH,
                               COMPASS_POINTER_WIDTH, 0)
        coords.point.set1Value(14, -COMPASS_POINTER_LENGTH, 0, 0)
        coords.point.set1Value(
            15, -COMPASS_POINTER_WIDTH, -COMPASS_POINTER_WIDTH, 0)

        return coords

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
            if (Draft.getType(obj) == "Building") or (hasattr(obj,"IfcType") and obj.IfcType == "Building"):
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



class _Site:

    "The Site object"

    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)
        obj.IfcType = "Site"

    def setProperties(self,obj):

        import ArchIFC
        ArchIFC.setProperties(obj)

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
        if not "IfcType" in pl:
            obj.addProperty("App::PropertyEnumeration","IfcType","IFC",QT_TRANSLATE_NOOP("App::Property","The type of this object"))
            obj.IfcType = ArchIFC.IfcTypes
            obj.IcfType = "Site"
        if not "TimeZone" in pl:
            obj.addProperty("App::PropertyInteger","TimeZone","Site",QT_TRANSLATE_NOOP("App::Property","The time zone where this site is located"))
        self.Type = "Site"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def execute(self,obj):

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

        ArchIFC.onChanged(obj, prop)
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




class _ViewProviderSite:

    "A View Provider for the Site object"

    def __init__(self,vobj):

        vobj.Proxy = self
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
        if not "Orientation" in pl:
            vobj.addProperty("App::PropertyEnumeration", "Orientation", "Site", QT_TRANSLATE_NOOP(
                "App::Property", "When set to 'True North' the whole geometry will be rotated to match the true north of this site"))
            vobj.Orientation = ["Project North", "True North"]
            vobj.Orientation = "Project North"
        if not "Compass" in pl:
            vobj.addProperty("App::PropertyBool", "Compass", "Compass", QT_TRANSLATE_NOOP("App::Property", "Show compass or not"))
        if not "CompassRotation" in pl:
            vobj.addProperty("App::PropertyAngle", "CompassRotation", "Compass", QT_TRANSLATE_NOOP("App::Property", "The rotation of the Compass relative to the Site"))
        if not "CompassPosition" in pl:
            vobj.addProperty("App::PropertyVector", "CompassPosition", "Compass", QT_TRANSLATE_NOOP("App::Property", "The position of the Compass relative to the Site placement"))
        if not "UpdateDeclination" in pl:
            vobj.addProperty("App::PropertyBool", "UpdateDeclination", "Compass", QT_TRANSLATE_NOOP("App::Property", "Update the Declination value based on the compass rotation"))

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Site_Tree.svg"

    def claimChildren(self):

        objs = []
        if hasattr(self,"Object"):
            objs = self.Object.Group+[self.Object.Terrain]
            prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
            if hasattr(self.Object,"Additions") and prefs.GetBool("swallowAdditions",True):
                objs.extend(self.Object.Additions)
            if hasattr(self.Object,"Subtractions") and prefs.GetBool("swallowSubtractions",True):
                objs.extend(self.Object.Subtractions)
        return objs

    def setEdit(self,vobj,mode):

        if (mode == 0) and hasattr(self,"Object"):
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

        self.Object = vobj.Object
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
        self.compass = Compass()
        self.updateCompassVisibility(vobj)
        self.updateCompassScale(vobj)
        self.rotateCompass(vobj)
        vobj.Annotation.addChild(self.compass.rootNode)

    def updateData(self,obj,prop):

        if prop in ["Longitude","Latitude"]:
            self.onChanged(obj.ViewObject,"SolarDiagram")
        elif prop == "Declination":
            self.onChanged(obj.ViewObject,"SolarDiagramPosition")
            self.updateTrueNorthRotation()
        elif prop == "Terrain":
            self.updateCompassLocation(obj.ViewObject)
        elif prop == "Placement":
            self.updateCompassLocation(obj.ViewObject)
            self.updateDeclination(obj.ViewObject)
        elif prop == "ProjectedArea":
            self.updateCompassScale(obj.ViewObject)

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
                    tz = 0
                    if hasattr(vobj.Object,"TimeZone"):
                        tz = vobj.Object.TimeZone
                    self.diagramnode = makeSolarDiagram(vobj.Object.Longitude,vobj.Object.Latitude,vobj.SolarDiagramScale,tz=tz)
                    if self.diagramnode:
                        self.diagramsep.addChild(self.diagramnode)
                        self.diagramswitch.whichChild = 0
                    else:
                        del self.diagramnode
                else:
                    self.diagramswitch.whichChild = -1
        elif prop == 'Visibility':
            if vobj.Visibility:
                self.updateCompassVisibility(self.Object)
            else:
                self.compass.hide()
        elif prop == 'Orientation':
            if vobj.Orientation == 'True North':
                self.addTrueNorthRotation()
            else:
                self.removeTrueNorthRotation()
        elif prop == "UpdateDeclination":
            self.updateDeclination(vobj)
        elif prop == "Compass":
            self.updateCompassVisibility(vobj)
        elif prop == "CompassRotation":
            self.updateDeclination(vobj)
            self.rotateCompass(vobj)
        elif prop == "CompassPosition":
            self.updateCompassLocation(vobj)

    def updateDeclination(self,vobj):

        if not hasattr(vobj, 'UpdateDeclination') or not vobj.UpdateDeclination:
            return
        compassRotation = vobj.CompassRotation.Value
        siteRotation = math.degrees(vobj.Object.Placement.Rotation.Angle) # This assumes Rotation.axis = (0,0,1)
        vobj.Object.Declination = compassRotation + siteRotation

    def addTrueNorthRotation(self):

        if hasattr(self, 'trueNorthRotation') and self.trueNorthRotation is not None:
            return
        from pivy import coin
        self.trueNorthRotation = coin.SoTransform()
        sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
        sg.insertChild(self.trueNorthRotation, 0)
        self.updateTrueNorthRotation()

    def removeTrueNorthRotation(self):

        if hasattr(self, 'trueNorthRotation') and self.trueNorthRotation is not None:
            sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
            sg.removeChild(self.trueNorthRotation)
            self.trueNorthRotation = None

    def updateTrueNorthRotation(self):

        if hasattr(self, 'trueNorthRotation') and self.trueNorthRotation is not None:
            from pivy import coin
            angle = self.Object.Declination.Value
            self.trueNorthRotation.rotation.setValue(coin.SbVec3f(0, 0, 1), math.radians(-angle))

    def updateCompassVisibility(self, vobj):

        if not hasattr(self, 'compass'):
            return
        show = hasattr(vobj, 'Compass') and vobj.Compass
        if show:
            self.compass.show()
        else:
            self.compass.hide()

    def rotateCompass(self, vobj):

        if not hasattr(self, 'compass'):
            return
        if hasattr(vobj, 'CompassRotation'):
            self.compass.rotate(vobj.CompassRotation.Value)

    def updateCompassLocation(self, vobj):

        if not hasattr(self, 'compass'):
            return
        if not vobj.Object.Shape:
            return
        boundBox = vobj.Object.Shape.BoundBox
        pos = vobj.Object.Placement.Base
        x = 0
        y = 0
        if hasattr(vobj, "CompassPosition"):
            x = vobj.CompassPosition.x
            y = vobj.CompassPosition.y
        z = boundBox.ZMax = pos.z
        self.compass.locate(x,y,z+1000)

    def updateCompassScale(self, vobj):

        if not hasattr(self, 'compass'):
            return
        self.compass.scale(vobj.Object.ProjectedArea)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Site',_CommandSite())
