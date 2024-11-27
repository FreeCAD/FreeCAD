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

__title__  = "FreeCAD Arch API"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## \defgroup ARCH Arch
#  \ingroup PYTHONWORKBENCHES
#  \brief Architecture and BIM tools
#
#  This module provides tools specialized in Building Information Modeling (BIM).
#  such as convenience tools to build walls, windows or structures, and
#  IFC import/export capabilities.

'''The Arch module provides tools specialized in BIM modeling.'''

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    FreeCADGui.updateLocale()
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


# generic functions

from ArchCommands import *
from ArchWindowPresets import *


# TODO: migrate this one

from ArchStructure import *


# make functions

def makeAxis(num=1,size=1000,name=None):

    '''makeAxis([num],[size],[name]): makes an Axis set
    based on the given number of axes and interval distances'''

    import ArchAxis
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Axis")
    obj.Label = name if name else translate("Arch","Axes")
    ArchAxis._Axis(obj)
    if FreeCAD.GuiUp:
        ArchAxis._ViewProviderAxis(obj.ViewObject)
    if num:
        dist = []
        angles = []
        for i in range(num):
            if i == 0:
                dist.append(0)
            else:
                dist.append(float(size))
            angles.append(float(0))
        obj.Distances = dist
        obj.Angles = angles
    FreeCAD.ActiveDocument.recompute()
    return obj


def makeAxisSystem(axes,name=None):

    '''makeAxisSystem(axes,[name]): makes a system from the given list of axes'''

    import ArchAxisSystem
    if not isinstance(axes,list):
        axes = [axes]
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","AxisSystem")
    obj.Label = name if name else translate("Arch","Axis System")
    ArchAxisSystem._AxisSystem(obj)
    obj.Axes = axes
    if FreeCAD.GuiUp:
        ArchAxisSystem._ViewProviderAxisSystem(obj.ViewObject)
    FreeCAD.ActiveDocument.recompute()
    return obj


def makeBuildingPart(objectslist=None,baseobj=None,name=None):

    '''makeBuildingPart([objectslist],[name]): creates a buildingPart including the
    objects from the given list.'''

    import ArchBuildingPart
    obj = FreeCAD.ActiveDocument.addObject("App::GeometryPython","BuildingPart")
    #obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","BuildingPart")
    obj.Label = name if name else translate("Arch","BuildingPart")
    ArchBuildingPart.BuildingPart(obj)
    obj.IfcType = "Building Element Part"
    if FreeCAD.GuiUp:
        ArchBuildingPart.ViewProviderBuildingPart(obj.ViewObject)
    if objectslist:
        obj.addObjects(objectslist)
    return obj


def makeFloor(objectslist=None,baseobj=None,name=None):

    """makes a BuildingPart and turns it into a Floor/Level"""

    obj = makeBuildingPart(objectslist)
    obj.Label = name if name else translate("Arch","Level")
    obj.IfcType = "Building Storey"
    obj.CompositionType = "ELEMENT"
    return obj


def makeBuilding(objectslist=None,baseobj=None,name=None):

    """makes a BuildingPart and turns it into a Building"""

    import ArchBuildingPart
    obj = makeBuildingPart(objectslist)
    obj.Label = name if name else translate("Arch","Building")
    obj.IfcType = "Building"
    obj.CompositionType = "ELEMENT"
    t = QT_TRANSLATE_NOOP("App::Property","The type of this building")
    obj.addProperty("App::PropertyEnumeration","BuildingType","Building",t)
    obj.BuildingType = ArchBuildingPart.BuildingTypes
    if FreeCAD.GuiUp:
        obj.ViewObject.ShowLevel = False
        obj.ViewObject.ShowLabel = False
    return obj


def convertFloors(floor=None):

    """convert the given Floor or Building (or all Arch Floors from the
    active document if none is given) into BuildingParts"""

    import Draft
    import ArchBuildingPart
    todel = []
    if floor:
        objset = [floor]
    else:
        objset = FreeCAD.ActiveDocument.Objects
    for obj in objset:
        if Draft.getType(obj) in ["Floor","Building"]:
            nobj = makeBuildingPart(obj.Group)
            if Draft.getType(obj) == "Floor":
                nobj.IfcType = "Building Storey"
                nobj.CompositionType = "ELEMENT"
            else:
                nobj.IfcType = "Building"
                nobj.CompositionType = "ELEMENT"
                t = QT_TRANSLATE_NOOP("App::Property","The type of this building")
                nobj.addProperty("App::PropertyEnumeration","BuildingType","Building",t)
                nobj.BuildingType = ArchBuildingPart.BuildingTypes
            label = obj.Label
            for parent in obj.InList:
                if hasattr(parent,"Group"):
                    if obj in parent.Group:
                        parent.addObject(nobj)
                        #g = parent.Group
                        #g.append(nobj)
                        #parent.Group = g
            todel.append(obj.Name)
            if obj.ViewObject:
                # some bug makes this trigger even efter the object has been deleted...
                obj.ViewObject.Proxy.Object = None
                # in case FreeCAD doesn't allow 2 objs with same label
            obj.Label = obj.Label+" to delete"
            nobj.Label = label
    for n in todel:
        from DraftGui import todo
        todo.delay(FreeCAD.ActiveDocument.removeObject,n)


def makeCurtainWall(baseobj=None,name=None):

    """makeCurtainWall([baseobj],[name]): Creates a curtain wall in the active document"""

    import ArchCurtainWall
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","CurtainWall")
    obj.Label = name if name else translate("Arch","Curtain Wall")
    ArchCurtainWall.CurtainWall(obj)
    if FreeCAD.GuiUp:
        ArchCurtainWall.ViewProviderCurtainWall(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            baseobj.ViewObject.hide()
    return obj


def makeEquipment(baseobj=None,placement=None,name=None):

    """makeEquipment([baseobj],[placement],[name]): creates an equipment object
    from the given base object."""

    import ArchEquipment
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Equipment")
    obj.Label = name if name else translate("Arch","Equipment")
    ArchEquipment._Equipment(obj)
    if baseobj:
        if baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Mesh = baseobj
        else:
            obj.Base = baseobj
    if placement:
        obj.Placement = placement
    if FreeCAD.GuiUp:
        ArchEquipment._ViewProviderEquipment(obj.ViewObject)
        if baseobj:
            baseobj.ViewObject.hide()
    return obj


def makeFence(section, post, path):

    """Makes a Fence object"""

    import ArchFence
    obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', 'Fence')
    ArchFence._Fence(obj)
    obj.Section = section
    obj.Post = post
    obj.Path = path
    if FreeCAD.GuiUp:
        ArchFence._ViewProviderFence(obj.ViewObject)
        ArchFence.hide(section)
        ArchFence.hide(post)
        ArchFence.hide(path)
    return obj


def makeFrame(baseobj,profile,name=None):

    """makeFrame(baseobj,profile,[name]): creates a frame object from a base sketch (or any other object
    containing wires) and a profile object (an extrudable 2D object containing faces or closed wires)"""

    import ArchFrame
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Frame")
    obj.Label = name if name else translate("Arch","Frame")
    ArchFrame._Frame(obj)
    if FreeCAD.GuiUp:
        ArchFrame._ViewProviderFrame(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
    if profile:
        obj.Profile = profile
        if FreeCAD.GuiUp:
            profile.ViewObject.hide()
    return obj


def makeGrid(name=None):

    '''makeGrid([name]): makes a grid object'''

    import ArchGrid
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Grid")
    obj.Label = name if name else translate("Arch","Grid")
    ArchGrid.ArchGrid(obj)
    if FreeCAD.GuiUp:
        ArchGrid.ViewProviderArchGrid(obj.ViewObject)
        obj.ViewObject.Transparency = 85
    FreeCAD.ActiveDocument.recompute()
    return obj


def makeMaterial(name=None,color=None,transparency=None):

    '''makeMaterial([name],[color],[transparency]): makes an Material object'''

    import ArchMaterial
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython","Material")
    obj.Label = name if name else translate("Arch","Material")
    ArchMaterial._ArchMaterial(obj)
    if FreeCAD.GuiUp:
        ArchMaterial._ViewProviderArchMaterial(obj.ViewObject)
    getMaterialContainer().addObject(obj)
    if color:
        obj.Color = color[:3]
        if len(color) > 3:
            obj.Transparency = color[3]*100
    if transparency:
        obj.Transparency = transparency
    return obj


def makeMultiMaterial(name=None):

    '''makeMultiMaterial([name]): makes an MultiMaterial object'''

    import ArchMaterial
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","MultiMaterial")
    obj.Label = name if name else translate("Arch","MultiMaterial")
    ArchMaterial._ArchMultiMaterial(obj)
    if FreeCAD.GuiUp:
        ArchMaterial._ViewProviderArchMultiMaterial(obj.ViewObject)
    getMaterialContainer().addObject(obj)
    return obj


def getMaterialContainer():

    '''getMaterialContainer(): returns a group object to put materials in'''

    import ArchMaterial
    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "MaterialContainer":
            return obj
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython","MaterialContainer")
    obj.Label = "Materials"
    ArchMaterial._ArchMaterialContainer(obj)
    if FreeCAD.GuiUp:
        ArchMaterial._ViewProviderArchMaterialContainer(obj.ViewObject)
    return obj


def getDocumentMaterials():

    '''getDocumentMaterials(): returns all the arch materials of the document'''

    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "MaterialContainer":
            mats = []
            for o in obj.Group:
                if o.isDerivedFrom("App::MaterialObjectPython"):
                    mats.append(o)
            return mats
    return []


def makePanel(baseobj=None,length=0,width=0,thickness=0,placement=None,name=None):

    '''makePanel([baseobj],[length],[width],[thickness],[placement],[name]): creates a
    panel element based on the given profile object and the given
    extrusion thickness. If no base object is given, you can also specify
    length and width for a simple cubic object.'''

    import ArchPanel
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Panel")
    obj.Label = name if name else translate("Arch","Panel")
    ArchPanel._Panel(obj)
    if FreeCAD.GuiUp:
        ArchPanel._ViewProviderPanel(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            obj.Base.ViewObject.hide()
    if width:
        obj.Width = width
    if thickness:
        obj.Thickness = thickness
    if length:
        obj.Length = length
    return obj


def makePanelCut(panel,name=None):

    """makePanelCut(panel,[name]) : Creates a 2D view of the given panel
    in the 3D space, positioned at the origin."""

    import ArchPanel
    view = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PanelCut")
    view.Label = name if name else translate("Arch","View of")+" "+panel.Label
    ArchPanel.PanelCut(view)
    view.Source = panel
    if FreeCAD.GuiUp:
        ArchPanel.ViewProviderPanelCut(view.ViewObject)
    return view


def makePanelSheet(panels=[],name=None):

    """makePanelSheet([panels],[name]) : Creates a sheet with the given panel cuts
    in the 3D space, positioned at the origin."""

    import ArchPanel
    sheet = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PanelSheet")
    sheet.Label = name if name else translate("Arch","PanelSheet")
    ArchPanel.PanelSheet(sheet)
    if panels:
        sheet.Group = panels
    if FreeCAD.GuiUp:
        ArchPanel.ViewProviderPanelSheet(sheet.ViewObject)
    return sheet


def makePipe(baseobj=None,diameter=0,length=0,placement=None,name=None):

    "makePipe([baseobj],[diameter],[length],[placement],[name]): creates an pipe object from the given base object"

    import ArchPipe
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj= FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Pipe")
    obj.Label = name if name else translate("Arch","Pipe")
    ArchPipe._ArchPipe(obj)
    if FreeCAD.GuiUp:
        ArchPipe._ViewProviderPipe(obj.ViewObject)
        if baseobj:
            baseobj.ViewObject.hide()
    if baseobj:
        obj.Base = baseobj
    else:
        if length:
            obj.Length = length
        else:
            obj.Length = 1000
    if diameter:
        obj.Diameter = diameter
    else:
        obj.Diameter = params.get_param_arch("PipeDiameter")
    obj.Width = obj.Diameter
    obj.Height = obj.Diameter
    if placement:
        obj.Placement = placement
    return obj


def makePipeConnector(pipes,radius=0,name=None):

    "makePipeConnector(pipes,[radius],[name]): creates a connector between the given pipes"

    import ArchPipe
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj= FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Connector")
    obj.Label = name if name else translate("Arch","Connector")
    ArchPipe._ArchPipeConnector(obj)
    obj.Pipes = pipes
    if not radius:
        radius = pipes[0].Diameter
    obj.Radius = radius
    if FreeCAD.GuiUp:
        ArchPipe._ViewProviderPipe(obj.ViewObject)
    return obj


def makeProfile(profile=[0,'REC','REC100x100','R',100,100]):

    '''makeProfile(profile): returns a shape  with the face defined by the profile data'''

    import ArchProfile
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython", "Profile")
    obj.Label = profile[2] + "_"
    if profile[3]=="C":
        ArchProfile._ProfileC(obj, profile)
    elif profile[3]=="H":
        ArchProfile._ProfileH(obj, profile)
    elif profile[3]=="R":
        ArchProfile._ProfileR(obj, profile)
    elif profile[3]=="RH":
        ArchProfile._ProfileRH(obj, profile)
    elif profile[3]=="U":
        ArchProfile._ProfileU(obj, profile)
    elif profile[3]=="L":
        ArchProfile._ProfileL(obj, profile)
    elif profile[3]=="T":
        ArchProfile._ProfileT(obj, profile)
    else :
        print("Profile not supported")
    if FreeCAD.GuiUp:
        ArchProfile.ViewProviderProfile(obj.ViewObject)
    return obj


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

    WARNING: This object is obsoleted in favour of the NativeIFC project
    """

    import ArchProject
    import Part
    if not FreeCAD.ActiveDocument:
        return FreeCAD.Console.PrintError("No active document. Aborting\n")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Project")
    obj.Label = name if name else translate("Arch", "Project")
    ArchProject._Project(obj)
    if FreeCAD.GuiUp:
        ArchProject._ViewProviderProject(obj.ViewObject)
    if sites:
        obj.Group = sites
    return obj


def makeRebar(baseobj=None,sketch=None,diameter=None,amount=1,offset=None,name=None):

    """makeRebar([baseobj],[sketch],[diameter],[amount],[offset],[name]):
    adds a Reinforcement Bar object to the given structural object,
    using the given sketch as profile."""

    import ArchRebar
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Rebar")
    obj.Label = name if name else translate("Arch","Rebar")
    ArchRebar._Rebar(obj)
    if FreeCAD.GuiUp:
        ArchRebar._ViewProviderRebar(obj.ViewObject)
    if baseobj and sketch:
        if hasattr(sketch,"AttachmentSupport"):
            if sketch.AttachmentSupport:
                if isinstance(sketch.AttachmentSupport,tuple):
                    if sketch.AttachmentSupport[0] == baseobj:
                        sketch.AttachmentSupport = None
                elif sketch.AttachmentSupport == baseobj:
                    sketch.AttachmentSupport = None
        obj.Base = sketch
        if FreeCAD.GuiUp:
            sketch.ViewObject.hide()
        obj.Host = baseobj
    elif sketch and not baseobj:
        # a rebar could be based on a wire without the existence of a Structure
        obj.Base = sketch
        if FreeCAD.GuiUp:
            sketch.ViewObject.hide()
        obj.Host = None
    elif baseobj and not sketch:
        obj.Shape = baseobj.Shape
    if diameter:
        obj.Diameter = diameter
    else:
        obj.Diameter = params.get_param_arch("RebarDiameter")
    obj.Amount = amount
    obj.Document.recompute()
    if offset is not None:
        obj.OffsetStart = offset
        obj.OffsetEnd = offset
    else:
        obj.OffsetStart = params.get_param_arch("RebarOffset")
        obj.OffsetEnd = params.get_param_arch("RebarOffset")
    obj.Mark = obj.Label
    return obj


def makeReference(filepath=None, partname=None, name=None):

    """makeReference([filepath],[partname],[name]): Creates an Arch Reference object"""

    import ArchReference
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","ArchReference")
    obj.Label = name if name else translate("Arch","External Reference")
    ArchReference.ArchReference(obj)
    if FreeCAD.GuiUp:
        ArchReference.ViewProviderArchReference(obj.ViewObject)
    if filepath:
        obj.File = filepath
    if partname:
        obj.Part = partname
    import Draft
    Draft.select(obj)
    return obj


def makeRoof(baseobj=None,
             facenr=0,
             angles=[45.0],
             run=[250.0],
             idrel=[-1],
             thickness=[50.0],
             overhang=[100.0],
             name=None):

    '''makeRoof(baseobj, [facenr], [angle], [name]): Makes a roof based on
    a closed wire or an object.

    You can provide a list of angles, run, idrel, thickness, overhang for
    each edge in the wire to define the roof shape. The default for angle is
    45 and the list is automatically completed to match the number of edges
    in the wire.

    If the base object is a solid the roof uses its shape.
    '''

    import ArchRoof
    import Part
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Roof")
    obj.Label = name if name else translate("Arch", "Roof")
    baseWire = None
    ArchRoof._Roof(obj)
    if FreeCAD.GuiUp:
        ArchRoof._ViewProviderRoof(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if hasattr(obj.Base, "Shape"):
            if obj.Base.Shape.Solids:
                if FreeCAD.GuiUp:
                    obj.Base.ViewObject.hide()
            else:
                if (obj.Base.Shape.Faces and obj.Face):
                    baseWire = obj.Base.Shape.Faces[obj.Face-1].Wires[0]
                    if FreeCAD.GuiUp:
                        obj.Base.ViewObject.hide()
                elif obj.Base.Shape.Wires:
                    baseWire = obj.Base.Shape.Wires[0]
                    if FreeCAD.GuiUp:
                        obj.Base.ViewObject.hide()
        if baseWire:
            if baseWire.isClosed():
                if FreeCAD.GuiUp:
                    obj.Base.ViewObject.hide()
                edges = Part.__sortEdges__(baseWire.Edges)
                ln = len(edges)
                obj.Angles    = ArchRoof.adjust_list_len(angles, ln, angles[0])
                obj.Runs      = ArchRoof.adjust_list_len(run, ln, run[0])
                obj.IdRel     = ArchRoof.adjust_list_len(idrel, ln, idrel[0])
                obj.Thickness = ArchRoof.adjust_list_len(thickness, ln, thickness[0])
                obj.Overhang  = ArchRoof.adjust_list_len(overhang, ln, overhang[0])
    obj.Face = facenr
    return obj


def makeSectionPlane(objectslist=None,name=None):

    """makeSectionPlane([objectslist],[name]) : Creates a Section plane objects including the
    given objects. If no object is given, the whole document will be considered."""

    import ArchSectionPlane
    import Draft
    import WorkingPlane
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Section")
    obj.Label = name if name else translate("Arch","Section")
    ArchSectionPlane._SectionPlane(obj)
    if FreeCAD.GuiUp:
        ArchSectionPlane._ViewProviderSectionPlane(obj.ViewObject)
    if objectslist:
        obj.Objects = objectslist
        bb = FreeCAD.BoundBox()
        for o in Draft.get_group_contents(objectslist):
            if hasattr(o,"Shape") and hasattr(o.Shape,"BoundBox"):
                bb.add(o.Shape.BoundBox)
        obj.Placement = WorkingPlane.get_working_plane().get_placement()
        obj.Placement.Base = bb.Center
        if FreeCAD.GuiUp:
            margin = bb.XLength*0.1
            obj.ViewObject.DisplayLength = bb.XLength+margin
            obj.ViewObject.DisplayHeight = bb.YLength+margin
    return obj


def makeSite(objectslist=None,baseobj=None,name=None):

    '''makeBuilding([objectslist],[baseobj],[name]): creates a site including the
    objects from the given list.'''

    import ArchSite
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import Part
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Site")
    obj.Label = name if name else translate("Arch","Site")
    ArchSite._Site(obj)
    if FreeCAD.GuiUp:
        ArchSite._ViewProviderSite(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    if baseobj:
        import Part
        if isinstance(baseobj,Part.Shape):
            obj.Shape = baseobj
        else:
            obj.Terrain = baseobj
    return obj


def makeSpace(objects=None,baseobj=None,name=None):

    """makeSpace([objects],[baseobj],[name]): Creates a space object from the given objects.
    Objects can be one document object, in which case it becomes the base shape of the space
    object, or a list of selection objects as got from getSelectionEx(), or a list of tuples
    (object, subobjectname)"""

    import ArchSpace
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Space")
    obj.Label = name if name else translate("Arch","Space")
    ArchSpace._Space(obj)
    if FreeCAD.GuiUp:
        ArchSpace._ViewProviderSpace(obj.ViewObject)
    if baseobj:
        objects = baseobj
    if objects:
        if not isinstance(objects,list):
            objects = [objects]
        if len(objects) == 1:
            obj.Base = objects[0]
            if FreeCAD.GuiUp:
                objects[0].ViewObject.hide()
        else:
            obj.Proxy.addSubobjects(obj,objects)
    return obj


def makeStairs(baseobj=None,length=None,width=None,height=None,steps=None,name=None):

    """makeStairs([baseobj],[length],[width],[height],[steps],[name]): creates a Stairs
    objects with given attributes."""

    import ArchStairs
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return

    stairs = []
    additions = []
    label = name if name else translate("Arch","Stairs")

    def setProperty(obj,length,width,height,steps):
        if length:
            obj.Length = length
        else:
            obj.Length = params.get_param_arch("StairsLength")
        if width:
            obj.Width = width
        else:
            obj.Width = params.get_param_arch("StairsWidth")
        if height:
            obj.Height = height
        else:
            obj.Height = params.get_param_arch("StairsHeight")
        if steps:
            obj.NumberOfSteps = steps
        obj.Structure = "Massive"
        obj.StructureThickness = 150
        obj.DownSlabThickness = 150
        obj.UpSlabThickness = 150

        obj.RailingOffsetLeft = 60
        obj.RailingOffsetRight = 60
        obj.RailingHeightLeft = 900
        obj.RailingHeightRight = 900

    if baseobj:
        if not isinstance(baseobj,list):
            baseobj = [baseobj]
        lenSelection = len(baseobj)
        if lenSelection > 1:
            stair = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Stairs")
            stair.Label = label
            ArchStairs._Stairs(stair)
            stairs.append(stair)
            stairs[0].Label = label
            i = 1
        else:
            i = 0
        for baseobjI in baseobj:
            stair = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Stairs")
            stair.Label = label
            ArchStairs._Stairs(stair)
            stairs.append(stair)
            stairs[i].Label = label
            stairs[i].Base = baseobjI

            if len(baseobjI.Shape.Edges) > 1:
                stepsI = 1                              #'landing' if 'multi-edges' currently
            elif steps:
                stepsI = steps
            else:
                stepsI = 20
            setProperty(stairs[i],None,width,height,stepsI)

            if i > 1:
                additions.append(stairs[i])
                stairs[i].LastSegment = stairs[i-1]
            else:
                if len(stairs) > 1:                     # i.e. length >1, have a 'master' staircase created
                    stairs[0].Base = stairs[1]
            i += 1
        if lenSelection > 1:
            stairs[0].Additions = additions
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Stairs")
        obj.Label = label
        ArchStairs._Stairs(obj)
        setProperty(obj,length,width,height,steps)
        stairs.append(obj)
    if FreeCAD.GuiUp:
        if baseobj:
            for stair in stairs:
                ArchStairs._ViewProviderStairs(stair.ViewObject)
        else:
            ArchStairs._ViewProviderStairs(obj.ViewObject)
    if stairs:
        for stair in stairs:
            stair.recompute()
        makeRailing(stairs)
        # return stairs - all other functions expect one object as return value
        return stairs[0]
    else:
        obj.recompute()
        return obj


def makeRailing(stairs):

    "simple make Railing function"

    import ArchPipe

    def makeRailingLorR(stairs,side="L"):
        for stair in reversed(stairs):
            if side == "L":
                outlineLR = stair.OutlineLeft
                outlineLRAll = stair.OutlineLeftAll
                stairRailingLR = "RailingLeft"
            elif side == "R":
                outlineLR = stair.OutlineRight
                outlineLRAll = stair.OutlineRightAll
                stairRailingLR = "RailingRight"
            if outlineLR or outlineLRAll:
                lrRail = makePipe(baseobj=None,diameter=0,length=0,placement=None,name=translate("Arch","Railing"))
                if outlineLRAll:
                    setattr(stair, stairRailingLR, lrRail)
                    break
                elif outlineLR:
                    setattr(stair, stairRailingLR, lrRail)

    if stairs is None:
        sel = FreeCADGui.Selection.getSelection()
        sel0 = sel[0]
        stairs = []
        # TODO currently consider 1st selected object, then would tackle multiple objects?
        if Draft.getType(sel[0]) == "Stairs":
            stairs.append(sel0)
            if Draft.getType(sel0.Base) == "Stairs":
                stairs.append(sel0.Base)
                additions = sel0.Additions
                for additionsI in additions:
                    if Draft.getType(additionsI) == "Stairs":
                        stairs.append(additionsI)
            else:
                stairs.append(sel[0])
        else:
            print("No Stairs object selected")
            return

    makeRailingLorR(stairs,"L")
    makeRailingLorR(stairs,"R")


def makeTruss(baseobj=None,name=None):

    """
    makeTruss([baseobj],[name]): Creates a space object from the given object (a line)
    """

    import ArchTruss
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Truss")
    obj.Label = name if name else translate("Arch","Truss")
    ArchTruss.Truss(obj)
    if FreeCAD.GuiUp:
        ArchTruss.ViewProviderTruss(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            baseobj.ViewObject.hide()
    return obj


def makeWall(baseobj=None,height=None,length=None,width=None,align=None,face=None,name=None):
    """Create a wall based on a given object, and returns the generated wall.

    TODO: It is unclear what defines which units this function uses.

    Parameters
    ----------
    baseobj: <Part::PartFeature>, optional
        The base object with which to build the wall. This can be a sketch, a
        draft object, a face, or a solid. It can also be left as None.
    height: float, optional
        The height of the wall.
    length: float, optional
        The length of the wall. Not used if the wall is based off an object.
        Will use Arch default if left empty.
    width: float, optional
        The width of the wall. Not used if the base object is a face.  Will use
        Arch default if left empty.
    align: str, optional
        Either "Center", "Left", or "Right". Effects the alignment of the wall
        on its baseline.
    face: int, optional
        The index number of a face on the given baseobj, to base the wall on.
    name: str, optional
        The name to give to the created wall.

    Returns
    -------
    <Part::FeaturePython>
        Returns the generated wall.

    Notes
    -----
    Creates a new <Part::FeaturePython> object, and turns it into a parametric wall
    object. This <Part::FeaturePython> object does not yet have any shape.

    The wall then uses the baseobj.Shape as the basis to extrude out a wall shape,
    giving the new <Part::FeaturePython> object a shape.

    It then hides the original baseobj.
    """

    import ArchWall
    import Draft
    from draftutils import params
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Wall")
    if name:
        obj.Label = name
    else:
        obj.Label = translate("Arch","Wall")
    ArchWall._Wall(obj)
    if FreeCAD.GuiUp:
        ArchWall._ViewProviderWall(obj.ViewObject)
    if baseobj:
        if hasattr(baseobj,'Shape') or baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Base = baseobj
        else:
            FreeCAD.Console.PrintWarning(str(translate("Arch","Walls can only be based on Part or Mesh objects")))
    if face:
        obj.Face = face
    if length:
        obj.Length = length
    if width:
        obj.Width = width
    else:
        obj.Width = params.get_param_arch("WallWidth")
    if height:
        obj.Height = height
    else:
        obj.Height = params.get_param_arch("WallHeight")
    if align:
        obj.Align = align
    else:
        obj.Align = ["Center","Left","Right"][params.get_param_arch("WallAlignment")]
    if obj.Base and FreeCAD.GuiUp:
        if Draft.getType(obj.Base) != "Space":
            obj.Base.ViewObject.hide()
    return obj


def joinWalls(walls,delete=False):
    """Join the given list of walls into one sketch-based wall.

    Take the first wall in the list, and adds on the other walls in the list.
    Return the modified first wall.

    Setting delete to True, will delete the other walls. Only join walls
    if the walls have the same width, height and alignment.

    Parameters
    ----------
    walls: list of <Part::FeaturePython>
        List containing the walls to add to the first wall in the list. Walls must
        be based off a base object.
    delete: bool, optional
        If True, deletes the other walls in the list.

    Returns
    -------
    <Part::FeaturePython>
    """

    import Part
    import Draft
    import ArchWall
    if not walls:
        return None
    if not isinstance(walls,list):
        walls = [walls]
    if not ArchWall.areSameWallTypes(walls):
        return None
    deleteList = []
    base = walls.pop()
    if base.Base:
        if base.Base.Shape.Faces:
            return None
        # Use ArchSketch if SketchArch add-on is present
        if Draft.getType(base.Base) == "ArchSketch":
            sk = base.Base
        else:
            try:
                import ArchSketchObject
                newSk=ArchSketchObject.makeArchSketch()
            except:
                if Draft.getType(base.Base) != "Sketcher::SketchObject":
                    newSk=FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","WallTrace")
                else:
                    newSk=None
            if newSk:
                sk = Draft.makeSketch(base.Base,autoconstraints=True, addTo=newSk)
                base.Base = sk
            else:
                sk = base.Base
    for w in walls:
        if w.Base:
            if not w.Base.Shape.Faces:
                for e in w.Base.Shape.Edges:
                    l = e.Curve
                    if isinstance(l,Part.Line):
                        l = Part.LineSegment(e.Vertexes[0].Point,e.Vertexes[-1].Point)
                    sk.addGeometry(l)
                    deleteList.append(w.Name)
    if delete:
        for n in deleteList:
            FreeCAD.ActiveDocument.removeObject(n)
    FreeCAD.ActiveDocument.recompute()
    base.ViewObject.show()
    return base


def makeWindow(baseobj=None,width=None,height=None,parts=None,name=None):

    '''makeWindow(baseobj,[width,height,parts,name]): creates a window based on the
    given base 2D object (sketch or draft).'''

    import ArchWindow
    import Draft
    from DraftGui import todo
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if baseobj:
        if Draft.getType(baseobj) == "Window":
            obj = Draft.clone(baseobj)
            return obj
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Window")
    ArchWindow._Window(obj)
    if name:
        obj.Label = name
    else:
        obj.Label = translate("Arch","Window")
    if FreeCAD.GuiUp:
        ArchWindow._ViewProviderWindow(obj.ViewObject)
    if width:
        obj.Width = width
    if height:
        obj.Height = height
    if baseobj:
        obj.Normal = baseobj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        obj.Base = baseobj
    if parts is not None:
        obj.WindowParts = parts
    else:
        if baseobj:
            if baseobj.getLinkedObject().isDerivedFrom("Part::Part2DObject"):
                # create default component
                if baseobj.Shape.Wires:
                    tp = "Frame"
                    if len(baseobj.Shape.Wires) == 1:
                        tp = "Solid panel"
                    i = 0
                    ws = ''
                    for w in baseobj.Shape.Wires:
                        if w.isClosed():
                            if ws: ws += ","
                            ws += "Wire" + str(i)
                            i += 1
                    obj.WindowParts = ["Default",tp,ws,"1","0"]
            else:
                # bind properties from base obj if existing
                for prop in ["Height","Width","Subvolume","Tag","Description","Material"]:
                    for p in baseobj.PropertiesList:
                        if (p == prop) or p.endswith("_"+prop):
                            obj.setExpression(prop, baseobj.Name+"."+p)

    if obj.Base and FreeCAD.GuiUp:
        obj.Base.ViewObject.DisplayMode = "Wireframe"
        obj.Base.ViewObject.hide()
        todo.delay(ArchWindow.recolorize,[obj.Document.Name,obj.Name])
    return obj
