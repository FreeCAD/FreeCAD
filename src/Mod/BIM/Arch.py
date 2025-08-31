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

"""Core API for architectural and Building Information Modeling (BIM) in FreeCAD.

Provides tools for creating parametric architectural elements (walls, windows,
structures) and managing BIM data. Serves as the foundation for both the BIM
Workbench and third-party extensions.

## Features
- Parametric architectural components (walls, floors, roofs, windows)
- BIM data support (materials, IFC properties, classification systems)
- Integration with FreeCAD's core (Part, Draft) and other workbenches
- Object creation utilities for architectural workflows

## Usage
Designed for:
1. Internal API for FreeCAD's built-in BIM commands
2. Public API for add-on developers creating extension macros, workbenches, or
   other specialized BIM tools

## Examples
```python
import Arch
wall = Arch.makeWall(length=5000, width=200, height=3000)  # mm units
wall.recompute()
```
"""
__title__  = "FreeCAD Arch API"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

import FreeCAD
from typing import Optional

if FreeCAD.GuiUp:
    import FreeCADGui
    FreeCADGui.updateLocale()

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


# Importing all members from these modules enables us to use them directly by
# simply importing the Arch module, as if they were part of this module.
from ArchCommands import *
from ArchWindowPresets import *

# TODO: migrate this one
# Currently makeStructure, makeStructuralSystem need migration
from ArchStructure import *


# make functions

def makeAxis(num=1, size=1000, name=None):
    """
    Creates an axis set in the active document.

    Parameters
    ----------
    num : int, optional
        The number of axes to create. Defaults to 1.
    size : float, optional
        The interval distance between axes. Defaults to 1000.
    name : str, optional
        The name to assign to the created axis object. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created axis object.
    """
    import ArchAxis
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Axis")
    obj.Label = name if name else translate("Arch", "Axes")
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


def makeAxisSystem(axes, name=None):
    """
    Creates an axis system from the given list of axes.

    Parameters
    ----------
    axes : list of Part::FeaturePython
        A list of axis objects to include in the axis system.
    name : str, optional
        The name to assign to the created axis system. Defaults to None.

    Returns
    -------
    App::FeaturePython
        The created axis system object.
    """
    import ArchAxisSystem
    if not isinstance(axes, list):
        axes = [axes]
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython", "AxisSystem")
    obj.Label = name if name else translate("Arch", "Axis System")
    ArchAxisSystem._AxisSystem(obj)
    obj.Axes = axes
    if FreeCAD.GuiUp:
        ArchAxisSystem._ViewProviderAxisSystem(obj.ViewObject)
    FreeCAD.ActiveDocument.recompute()
    return obj


def makeBuildingPart(objectslist=None, baseobj=None, name=None):
    """
    Creates a building part including the given objects in the list.

    Parameters
    ----------
    objectslist : list of Part::FeaturePython, optional
        A list of objects to include in the building part. Defaults to None.
    baseobj : Part::FeaturePython, optional
        The base object for the building part. Defaults to None.
    name : str, optional
        The name to assign to the created building part. Defaults to None.

    Returns
    -------
    App::GeometryPython
        The created building part object.
    """
    import ArchBuildingPart
    obj = FreeCAD.ActiveDocument.addObject("App::GeometryPython", "BuildingPart")
    #obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","BuildingPart")
    obj.Label = name if name else translate("Arch", "BuildingPart")
    ArchBuildingPart.BuildingPart(obj)
    obj.IfcType = "Building Element Part"
    if FreeCAD.GuiUp:
        ArchBuildingPart.ViewProviderBuildingPart(obj.ViewObject)
    if objectslist:
        if isinstance(objectslist, (list, tuple)):
            obj.addObjects(objectslist)
        else:
            obj.addObject(objectslist)
    return obj


def makeFloor(objectslist=None, baseobj=None, name=None):
    """
    Creates a floor/level in the active document.

    Parameters
    ----------
    objectslist : list of Part::FeaturePython, optional
        A list of objects to include in the floor. Defaults to None.
    baseobj : Part::FeaturePython, optional
        The base object for the floor. Defaults to None.
    name : str, optional
        The name to assign to the created floor. Defaults to None.

    Returns
    -------
    App::GeometryPython
        The created floor object.
    """
    obj = makeBuildingPart(objectslist)
    obj.Label = name if name else translate("Arch", "Level")
    obj.IfcType = "Building Storey"
    obj.CompositionType = "ELEMENT"
    return obj


def makeBuilding(objectslist=None, baseobj=None, name=None):
    """
    Creates a building in the active document.

    Parameters
    ----------
    objectslist : list of Part::FeaturePython, optional
        A list of objects to include in the building. Defaults to None.
    baseobj : Part::FeaturePython, optional
        The base object for the building. Defaults to None.
    name : str, optional
        The name to assign to the created building. Defaults to None.

    Returns
    -------
    App::GeometryPython
        The created building object.
    """
    import ArchBuildingPart
    obj = makeBuildingPart(objectslist)
    obj.Label = name if name else translate("Arch", "Building")
    obj.IfcType = "Building"
    obj.CompositionType = "ELEMENT"
    t = QT_TRANSLATE_NOOP("App::Property", "The type of this building")
    obj.addProperty("App::PropertyEnumeration", "BuildingType", "Building", t, locked=True)
    obj.BuildingType = ArchBuildingPart.BuildingTypes
    if FreeCAD.GuiUp:
        obj.ViewObject.ShowLevel = False
        obj.ViewObject.ShowLabel = False
    return obj


def make2DDrawing(objectslist=None, baseobj=None, name=None):
    """
    Creates a 2D drawing view in the active document.

    Parameters
    ----------
    objectslist : list of Part::FeaturePython, optional
        A list of objects to include in the drawing. Defaults to None.
    baseobj : Part::FeaturePython, optional
        The base object for the drawing. Defaults to None.
    name : str, optional
        The name to assign to the created drawing. Defaults to None.

    Returns
    -------
    App::GeometryPython
        The created 2D drawing object.
    """
    obj = makeBuildingPart(objectslist)
    obj.Label = name if name else translate("Arch", "Drawing")
    obj.IfcType = "Annotation"
    obj.ObjectType = "DRAWING"
    obj.setEditorMode("Area", 2)
    obj.setEditorMode("Height", 2)
    obj.setEditorMode("LevelOffset", 2)
    obj.setEditorMode("OnlySolids", 2)
    obj.setEditorMode("HeightPropagate", 2)
    if FreeCAD.GuiUp:
        obj.ViewObject.DisplayOffset = FreeCAD.Placement()
        obj.ViewObject.ShowLevel = False
    return obj


def convertFloors(floor=None):
    """
    Converts the given floor or building into building parts.

    Parameters
    ----------
    floor : Part::FeaturePython, optional
        The floor or building to convert. If None, all Arch floors in the active document
        are converted. Defaults to None.

    Returns
    -------
    None
    """
    import Draft
    import ArchBuildingPart
    todel = []
    if floor:
        objset = [floor]
    else:
        objset = FreeCAD.ActiveDocument.Objects
    for obj in objset:
        if Draft.getType(obj) in ["Floor", "Building"]:
            nobj = makeBuildingPart(obj.Group)
            if Draft.getType(obj) == "Floor":
                nobj.IfcType = "Building Storey"
                nobj.CompositionType = "ELEMENT"
            else:
                nobj.IfcType = "Building"
                nobj.CompositionType = "ELEMENT"
                t = QT_TRANSLATE_NOOP("App::Property", "The type of this building")
                nobj.addProperty(
                    "App::PropertyEnumeration", "BuildingType", "Building", t, locked=True
                )
                nobj.BuildingType = ArchBuildingPart.BuildingTypes
            label = obj.Label
            for parent in obj.InList:
                if hasattr(parent, "Group"):
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
            obj.Label = obj.Label + " to delete"
            nobj.Label = label
    for n in todel:
        from draftutils import todo
        todo.ToDo.delay(FreeCAD.ActiveDocument.removeObject, n)


def makeCurtainWall(baseobj=None, name=None):
    """
    Creates a curtain wall object in the active document.

    Parameters
    ----------
    baseobj : Part::FeaturePython, optional
        The base object for the curtain wall. Defaults to None.
    name : str, optional
        The name to assign to the created curtain wall. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created curtain wall object.
    """
    curtainWall = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="CurtainWall",
        internalName="CurtainWall",
        defaultLabel=name if name else translate("Arch", "Curtain Wall"),
        viewProviderName="ViewProviderCurtainWall",
    )

    # Initialize all relevant properties
    if baseobj:
        curtainWall.Base = baseobj
        if FreeCAD.GuiUp:
            baseobj.ViewObject.hide()

    return curtainWall


def makeEquipment(baseobj=None, placement=None, name=None):
    """
    Creates an equipment object from the given base object in the active document.

    Parameters
    ----------
    baseobj : Part::FeaturePython or Mesh::Feature, optional
        The base object for the equipment. Defaults to None.
    placement : FreeCAD.Placement, optional
        The placement of the equipment. Defaults to None.
    name : str, optional
        The name to assign to the created equipment. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created equipment object.
    """
    equipment = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Equipment",
        internalName="Equipment",
        defaultLabel=name if name else translate("Arch", "Equipment"),
    )

    # Initialize all relevant properties
    if baseobj:
        if baseobj.isDerivedFrom("Mesh::Feature"):
            equipment.Mesh = baseobj
        else:
            equipment.Base = baseobj
    if placement:
        equipment.Placement = placement

    if FreeCAD.GuiUp and baseobj:
        baseobj.ViewObject.hide()
    return equipment


def makeFence(section, post, path):
    """
    Creates a fence object in the active document.

    Parameters
    ----------
    section : Part::FeaturePython
        The section profile of the fence.
    post : Part::FeaturePython
        The post profile of the fence.
    path : Part::FeaturePython
        The path along which the fence is created.

    Returns
    -------
    Part::FeaturePython
        The created fence object.
    """
    fence = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Fence",
        internalName="Fence",
        defaultLabel=translate("Arch", "Fence"),
    )
    fence.Section = section
    fence.Post = post
    fence.Path = path
    if FreeCAD.GuiUp:
        import ArchFence
        ArchFence.hide(section)
        ArchFence.hide(post)
        ArchFence.hide(path)
    return fence


def makeFrame(baseobj, profile, name=None):
    """Creates a frame object from a base sketch (or any other object containing wires) and a
    profile object (an extrudable 2D object containing faces or closed wires).

    Parameters
    ----------
    baseobj : Part::FeaturePython
        The base object containing wires to define the frame.
    profile : Part::FeaturePython
        The profile object, an extrudable 2D object containing faces or closed wires.
    name : str, optional
        The name to assign to the created frame. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created frame object.
    """
    frame = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Frame",
        internalName="Frame",
        defaultLabel=name if name else translate("Arch", "Frame"),
    )

    # Initialize all relevant properties
    if baseobj:
        frame.Base = baseobj
    if profile:
        frame.Profile = profile
        if FreeCAD.GuiUp:
            profile.ViewObject.hide()

    return frame


def makeGrid(name=None):
    """
    Creates a grid object in the active document.

    Parameters
    ----------
    name : str, optional
        The name to assign to the created grid. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created grid object.
    """
    grid = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="ArchGrid",
        internalName="Grid",
        defaultLabel=name if name else translate("Arch", "Grid"),
        moduleName="ArchGrid",
        viewProviderName="ViewProviderArchGrid",
    )

    # Initialize all relevant properties
    if FreeCAD.GuiUp:
        grid.ViewObject.Transparency = 85

    FreeCAD.ActiveDocument.recompute()

    return grid


def makeMaterial(name=None, color=None, transparency=None):
    """
    Creates a material object in the active document.

    Parameters
    ----------
    name : str, optional
        The name to assign to the created material. Defaults to None.
    color : tuple of float, optional
        The RGB color of the material. Defaults to None.
    transparency : float, optional
        The transparency level of the material. Defaults to None.

    Returns
    -------
    App::MaterialObjectPython
        The created material object.
    """
    material = _initializeArchObject(
        "App::MaterialObjectPython",
        baseClassName="_ArchMaterial",
        internalName="Material",
        defaultLabel=name if name else translate("Arch", "Material"),
    )
    getMaterialContainer().addObject(material)

    # Initialize all relevant properties
    if color:
        r, g, b = color[:3]
        material.Color = (r, g, b)
        if len(color) > 3:
            alpha = color[3]
            material.Transparency = alpha * 100
    if transparency:
        material.Transparency = transparency

    return material


def makeMultiMaterial(name=None):
    """
    Creates a multi-material object in the active document.

    Parameters
    ----------
    name : str, optional
        The name to assign to the created multi-material. Defaults to None.

    Returns
    -------
    App::FeaturePython
        The created multi-material object.
    """
    multimaterial = _initializeArchObject(
        "App::FeaturePython",
        baseClassName="_ArchMultiMaterial",
        internalName="MultiMaterial",
        defaultLabel=name if name else translate("Arch", "MultiMaterial"),
        moduleName="ArchMaterial",
    )
    getMaterialContainer().addObject(multimaterial)

    return multimaterial


def getMaterialContainer():
    """
    Returns a group object to store materials in the active document.

    Returns
    -------
    App::DocumentObjectGroupPython
        The material container object.
    """
    # Check if a container already exists
    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "MaterialContainer":
            return obj

    # If no container exists, create one
    materialContainer = _initializeArchObject(
        "App::DocumentObjectGroupPython",
        baseClassName="_ArchMaterialContainer",
        internalName="MaterialContainer",
        defaultLabel=translate("Arch", "Materials"),
        moduleName="ArchMaterial",
    )

    return materialContainer


def getDocumentMaterials():
    """
    Retrieves all material objects in the active document.

    Returns
    -------
    list of App::MaterialObjectPython
        A list of all material objects in the document.
    """
    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "MaterialContainer":
            materials = []
            for o in obj.Group:
                if o.isDerivedFrom("App::MaterialObjectPython"):
                    materials.append(o)
            return materials
    return []


def makePanel(baseobj=None, length=0, width=0, thickness=0, placement=None, name=None):
    """
    Creates a panel element based on the given profile object and the given
    extrusion thickness. If no base object is given, you can also specify
    length and width for a simple cubic object.

    Parameters
    ----------
    baseobj : Part::FeaturePython, optional
        The base profile object for the panel. Defaults to None.
    length : float, optional
        The length of the panel. Defaults to 0.
    width : float, optional
        The width of the panel. Defaults to 0.
    thickness : float, optional
        The thickness of the panel. Defaults to 0.
    placement : FreeCAD.Placement, optional
        The placement of the panel. Defaults to None.
    name : str, optional
        The name to assign to the created panel. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created panel object.
    """
    panel = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Panel",
        internalName="Panel",
        defaultLabel=name if name else translate("Arch", "Panel"),
    )

    # Initialize all relevant properties
    if baseobj:
        panel.Base = baseobj
        if FreeCAD.GuiUp:
            panel.Base.ViewObject.hide()
    if width:
        panel.Width = width
    if thickness:
        panel.Thickness = thickness
    if length:
        panel.Length = length

    return panel


def makePanelCut(panel, name=None):
    """
    Creates a 2D view of the given panel in the 3D space, positioned at the origin.

    Parameters
    ----------
    panel : Part::FeaturePython
        The panel object to create a 2D view for.
    name : str, optional
        The name to assign to the created panel cut. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created panel cut object.
    """
    view = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="PanelCut",
        internalName="PanelCut",
        defaultLabel=name if name else translate("Arch", f"View of {panel.Label}"),
        moduleName="ArchPanel",
        viewProviderName="ViewProviderPanelCut",
    )
    view.Source = panel
    return view


def makePanelSheet(panels=[], name=None):
    """
    Creates a sheet with the given panel cuts in the 3D space, positioned at the origin.

    Parameters
    ----------
    panels : list of Part::FeaturePython, optional
        A list of panel cuts to include in the sheet. Defaults to an empty list.
    name : str, optional
        The name to assign to the created panel sheet. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created panel sheet object.
    """
    sheet = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="PanelSheet",
        internalName="PanelSheet",
        defaultLabel=name if name else translate("Arch", "PanelSheet"),
        moduleName="ArchPanel",
        viewProviderName="ViewProviderPanelSheet",
    )
    if panels:
        sheet.Group = panels
    return sheet


def makePipe(baseobj=None, diameter=0, length=0, placement=None, name=None):
    """
    Creates a pipe object from the given base object or specified dimensions.

    Parameters
    ----------
    baseobj : Part::FeaturePython, optional
        The base object for the pipe. Defaults to None.
    diameter : float, optional
        The diameter of the pipe. Defaults to 0.
    length : float, optional
        The length of the pipe. Defaults to 0.
    placement : FreeCAD.Placement, optional
        The placement of the pipe. Defaults to None.
    name : str, optional
        The name to assign to the created pipe. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created pipe object.
    """
    pipe = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_ArchPipe",
        internalName="Pipe",
        defaultLabel=name if name else translate("Arch", "Pipe"),
        viewProviderName="_ViewProviderPipe",
    )

    # Initialize all relevant properties
    pipe.Diameter = diameter if diameter else params.get_param_arch("PipeDiameter")
    pipe.Width = pipe.Diameter
    pipe.Height = pipe.Diameter

    if baseobj:
        pipe.Base = baseobj
    else:
        pipe.Length = length if length else 1000

    if placement:
        pipe.Placement = placement

    if FreeCAD.GuiUp:
        if baseobj:
            baseobj.ViewObject.hide()

    return pipe


def makePipeConnector(pipes, radius=0, name=None):
    """
    Creates a connector between the given pipes.

    Parameters
    ----------
    pipes : list of Part::FeaturePython
        A list of pipe objects to connect.
    radius : float, optional
        The curvature radius of the connector. Defaults to 0, which uses the diameter of the first
        pipe.
    name : str, optional
        The name to assign to the created connector. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created pipe connector object.
    """
    pipeConnector = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_ArchPipeConnector",
        internalName="Connector",
        defaultLabel=name if name else translate("Arch", "Connector"),
        moduleName="ArchPipe",
        viewProviderName="_ViewProviderPipe",
    )

    # Initialize all relevant properties
    pipeConnector.Pipes = pipes
    if radius:
        pipeConnector.Radius = radius
    elif pipes[0].ProfileType == "Circle":
        pipeConnector.Radius = pipes[0].Diameter
    else:
        pipeConnector.Radius = max(pipes[0].Height, pipes[0].Width)

    return pipeConnector


def makeProfile(profile=[0, 'REC', 'REC100x100', 'R', 100, 100]):
    """
    Creates a profile object based on the given profile data.

    Parameters
    ----------
    profile : list, optional
        A list defining the profile data. Defaults to [0, 'REC', 'REC100x100', 'R', 100, 100].
        The list should contain the following elements:

        0. listOrder: str
            The order of the profile data. Currently not used.
        1. profileSubClass: str
            The subclass of a given profile class (e.g. 'REC' for the 'C' class).
        2. profileName: str
            The name of the profile (e.g., 'REC100x100').
        3. profileClass: str
            The class of the profile (e.g., 'REC', 'C', 'H', etc.).
        4. dimensionsList: int
            A variable set of arguments that define the dimensions of the profile. Their
            interpretation and count depends on the type of profile. Not implemented
            as a list, it's a variable number of arguments within the main profile
            argument. For instance, a C profile will define outside diameter and thickness,
            whereas a H profile will define width, height, web thickness, and flange thickness.
            See https://wiki.freecad.org/Arch_Profile for more details on profile presets.

    Returns
    -------
    Part::Part2DObjectPython
        The created profile object.
    """
    import ArchProfile
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython", "Profile")

    profileName, profileClass = profile[2:4]

    match profileClass:
        case "C":
            ArchProfile._ProfileC(obj, profile)
        case "H":
            ArchProfile._ProfileH(obj, profile)
        case "R":
            ArchProfile._ProfileR(obj, profile)
        case "RH":
            ArchProfile._ProfileRH(obj, profile)
        case "U":
            ArchProfile._ProfileU(obj, profile)
        case "L":
            ArchProfile._ProfileL(obj, profile)
        case "T":
            ArchProfile._ProfileT(obj, profile)
        case _:
            print("Profile not supported")

    if FreeCAD.GuiUp:
        ArchProfile.ViewProviderProfile(obj.ViewObject)

    # Initialize all relevant properties
    obj.Label = profileName + "_"

    return obj


def makeProject(sites=None, name=None):
    """Create an Arch project.

    If sites are provided, add them as children of the new project.

    .. deprecated:: 1.0.0

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

    Notes
    -----
    This function is deprecated and will be removed in a future version.
    The NativeIFC project is the new way to create IFC projects.
    """
    project = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Project",
        internalName="Project",
        defaultLabel=name if name else translate("Arch", "Project"),
    )

    # Initialize all relevant properties
    if sites:
        project.Group = sites

    return project

def makeRebar(
    baseobj: Optional[FreeCAD.DocumentObject] = None,
    sketch: Optional[FreeCAD.DocumentObject] = None,
    diameter: Optional[float] = None,
    amount: int = 1,
    offset: Optional[float] = None,
    name: Optional[str] = None
) -> Optional[FreeCAD.DocumentObject]:
    """
    Creates a reinforcement bar (rebar) object.

    The rebar's geometry is typically defined by a `sketch` object (e.g., a Sketcher::SketchObject
    or a Draft.Wire). This sketch represents the path of a single bar. The `amount` and `spacing`
    (calculated by the object) properties then determine how many such bars are created and
    distributed.

    The `baseobj` usually acts as the structural host for the rebar. The rebar's distribution (e.g.,
    spacing, direction) can be calculated relative to this host object's dimensions if a `Host` is
    assigned and the rebar logic uses it.

    Parameters
    ----------
    baseobj : FreeCAD.DocumentObject, optional
        The structural object to host the rebar (e.g., an ArchStructure._Structure created with
        `Arch.makeStructure()`). If provided with `sketch`, it's set as `rebar.Host`. If provided
        *without* a `sketch`, `rebar.Shape` is set from `baseobj.Shape`, and `rebar.Host` remains
        None. Defaults to None.
    sketch : FreeCAD.DocumentObject, optional
        An object (e.g., "Sketcher::SketchObject") whose shape defines the rebar's path. Assigned to
        `rebar.Base`. If the sketch is attached to `baseobj` before calling this function (e.g. for
        positioning purposes), this function may clear that specific attachment to avoid conflicts,
        as the rebar itself will be hosted. Defaults to None.
    diameter : float, optional
        The diameter of the rebar. If None, uses Arch preferences ("RebarDiameter"). Defaults to
        None.
    amount : int, optional
        The number of rebar instances. Defaults to 1.
    offset : float, optional
        Concrete cover distance, sets `rebar.OffsetStart` and `rebar.OffsetEnd`. If None, uses Arch
        preferences ("RebarOffset"). Defaults to None.
    name : str, optional
        The user-visible name (Label) for the rebar. If None, defaults to "Rebar". Defaults to None.

    Returns
    -------
    FreeCAD.DocumentObject or None
        The created rebar object, or None if creation fails.

    Examples
    --------
    >>> import FreeCAD, Arch, Part, Sketcher
    >>> doc = FreeCAD.newDocument()
    >>> # Create a host structure (e.g., a concrete beam)
    >>> beam = Arch.makeStructure(length=2000, width=200, height=300)
    >>> doc.recompute() # Ensure beam's shape is ready
    >>>
    >>> # Create a sketch for the rebar path
    >>> rebar_sketch = doc.addObject('Sketcher::SketchObject')
    >>> # For positioning, attach the sketch to a face of the beam *before* makeRebar
    >>> # Programmatically select a face (e.g., the first one)
    >>> # For stable scripts, select faces by more reliable means
    >>> rebar_sketch.AttachmentSupport = (beam, ['Face1']) # Faces are 1-indexed
    >>> rebar_sketch.MapMode = "FlatFace"
    >>> # Define sketch geometry relative to the attached face's plane
    >>> rebar_sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(25, 25, 0),
    ...                                         FreeCAD.Vector(1975, 25, 0)), False)
    >>> doc.recompute() # Recompute sketch after geometry and attachment
    >>>
    >>> # Create the rebar object, linking it to the beam and using the sketch
    >>> rebar_obj = Arch.makeRebar(baseobj=beam, sketch=rebar_sketch, diameter=12,
    ...                            amount=4, offset=25)
    >>> doc.recompute() # Trigger rebar's geometry calculation
    """
    rebar = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Rebar",
        internalName="Rebar",
        defaultLabel=name if name else translate("Arch", "Rebar"),
        moduleName="ArchRebar",
        viewProviderName="_ViewProviderRebar",
    )

    # Initialize all relevant properties
    if baseobj and sketch:
        # Case 1: both the structural element (base object) and a sketch defining the shape and path
        # of a single rebar strand are provided. This is the most common scenario.
        if hasattr(sketch, "AttachmentSupport"):
            if sketch.AttachmentSupport:
                # If the sketch is already attached to the base object, remove that attachment.
                # Support two AttachmentSupport (PropertyLinkList) formats:
                # 1. Tuple: (baseobj, subelement)
                # 2. Direct object: baseobj
                # TODO: why is the list format not checked for here?
                # ~ 3. List: [baseobj, subelement] ~
                if isinstance(sketch.AttachmentSupport, tuple):
                    if sketch.AttachmentSupport[0] == baseobj:
                        sketch.AttachmentSupport = None
                elif sketch.AttachmentSupport == baseobj:
                    sketch.AttachmentSupport = None
        rebar.Base = sketch
        if FreeCAD.GuiUp:
            sketch.ViewObject.hide()
        rebar.Host = baseobj
    elif not baseobj and sketch:
        # Case 2: standalone rebar strand defined by a sketch, not attached to any structural
        # element.
        rebar.Base = sketch
        if FreeCAD.GuiUp:
            sketch.ViewObject.hide()
        rebar.Host = None
    elif baseobj and not sketch:
        # Case 3: rebar strand defined by the shape of a structural element (base object). The
        # base object becomes the rebar.
        rebar.Shape = baseobj.Shape
    rebar.Diameter = diameter if diameter else params.get_param_arch("RebarDiameter")
    rebar.Amount = amount
    rebar.Document.recompute()
    if offset is not None:
        rebar.OffsetStart = offset
        rebar.OffsetEnd = offset
    else:
        rebar.OffsetStart = params.get_param_arch("RebarOffset")
        rebar.OffsetEnd = params.get_param_arch("RebarOffset")
    rebar.Mark = rebar.Label

    return rebar


def makeReference(filepath=None, partname=None, name=None):
    """
    Creates an Arch reference object.

    Parameters
    ----------
    filepath : str, optional
        The file path of the external reference. Defaults to None.
    partname : str, optional
        The name of the part in the external file. Defaults to None.
    name : str, optional
        The name to assign to the created reference. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created reference object.
    """
    reference = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="ArchReference",
        internalName="ArchReference",
        defaultLabel=name if name else translate("Arch", "External Reference"),
        moduleName="ArchReference",
        viewProviderName="ViewProviderArchReference",
    )

    if filepath:
        reference.File = filepath
    if partname:
        reference.Part = partname

    import Draft
    Draft.select(reference)

    return reference


def makeRoof(baseobj=None,
             facenr=0,
             angles=[45.0],
             run=[250.0],
             idrel=[-1],
             thickness=[50.0],
             overhang=[100.0],
             name=None):
    """
    Creates a roof object based on a closed wire or an object.

    Parameters
    ----------
    baseobj : Part::FeaturePython, optional
        The base object for the roof. Defaults to None.
    facenr : int, optional
        The face number to use as the base. Defaults to 0.
    angles : list of float, optional
        The angles for each edge of the roof. Defaults to [45.0].
    run : list of float, optional
        The run distances for each edge. Defaults to [250.0].
    idrel : list of int, optional
        The relative IDs for each edge. Defaults to [-1].
    thickness : list of float, optional
        The thickness of the roof for each edge. Defaults to [50.0].
    overhang : list of float, optional
        The overhang distances for each edge. Defaults to [100.0].
    name : str, optional
        The name to assign to the created roof. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created roof object.

    Notes
    -----
    1. If the base object is a solid the roof uses its shape.
    2. The angles, run, idrel, thickness, and overhang lists are automatically
       completed to match the number of edges in the wire.
    """
    import Part
    import ArchRoof

    baseWire = None

    roof = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Roof",
        internalName="Roof",
        defaultLabel=name if name else translate("Arch", "Roof"),
        moduleName="ArchRoof",
        viewProviderName="_ViewProviderRoof",
    )

    # Initialize all relevant properties
    if baseobj:
        roof.Base = baseobj
        if hasattr(roof.Base, "Shape"):
            if roof.Base.Shape.Solids:
                if FreeCAD.GuiUp:
                    roof.Base.ViewObject.hide()
            else:
                if (roof.Base.Shape.Faces and roof.Face):
                    baseWire = roof.Base.Shape.Faces[roof.Face - 1].Wires[0]
                    if FreeCAD.GuiUp:
                        roof.Base.ViewObject.hide()
                elif roof.Base.Shape.Wires:
                    baseWire = roof.Base.Shape.Wires[0]
                    if FreeCAD.GuiUp:
                        roof.Base.ViewObject.hide()
        if baseWire:
            if baseWire.isClosed():
                if FreeCAD.GuiUp:
                    roof.Base.ViewObject.hide()
                edges = Part.__sortEdges__(baseWire.Edges)
                ln = len(edges)
                roof.Angles    = ArchRoof.adjust_list_len(angles, ln, angles[0])
                roof.Runs      = ArchRoof.adjust_list_len(run, ln, run[0])
                roof.IdRel     = ArchRoof.adjust_list_len(idrel, ln, idrel[0])
                roof.Thickness = ArchRoof.adjust_list_len(thickness, ln, thickness[0])
                roof.Overhang  = ArchRoof.adjust_list_len(overhang, ln, overhang[0])

    roof.Face = facenr

    return roof


def makeSchedule():
    """
    Creates a schedule object in the active document.

    Returns
    -------
    App::FeaturePython
        The created schedule object.
    """
    schedule = _initializeArchObject(
        "Part::FeaturePython",
        internalName="Schedule",
        baseClassName="_ArchSchedule",
        defaultLabel=translate("Arch", "Schedule"),
    )

    # Initialize all relevant properties
    if hasattr(schedule, "CreateSpreadsheet") and schedule.CreateSpreadsheet:
        schedule.Proxy.getSpreadSheet(schedule, force=True)

    return schedule


def makeSectionPlane(objectslist=None, name=None):
    """
    Creates a section plane object including the given objects.

    Parameters
    ----------
    objectslist : list of Part::FeaturePython, optional
        A list of objects to include in the section plane. If no object is given, the whole
        document will be considered. Defaults to None.
    name : str, optional
        The name to assign to the created section plane. Defaults to None.

    Returns
    -------
    App::FeaturePython
        The created section plane object.
    """
    import Draft
    from WorkingPlane import get_working_plane

    sectionPlane = _initializeArchObject(
        "App::FeaturePython",
        baseClassName="_SectionPlane",
        internalName="Section",
        defaultLabel=name if name else translate("Arch", "Section"),
    )

    # Initialize all relevant properties
    if objectslist:
        sectionPlane.Objects = objectslist
        boundBox = FreeCAD.BoundBox()
        for obj in Draft.get_group_contents(objectslist):
            if hasattr(obj, "Shape") and hasattr(obj.Shape, "BoundBox"):
                boundBox.add(obj.Shape.BoundBox)
        sectionPlane.Placement = get_working_plane().get_placement()
        sectionPlane.Placement.Base = boundBox.Center
        if FreeCAD.GuiUp:
            margin = boundBox.XLength * 0.1
            sectionPlane.ViewObject.DisplayLength = boundBox.XLength + margin
            sectionPlane.ViewObject.DisplayHeight = boundBox.YLength + margin
    return sectionPlane


def makeSite(objectslist=None, baseobj=None, name=None):
    """
    Creates a site object including the given objects.

    Parameters
    ----------
    objectslist : list of Part::FeaturePython, optional
        A list of objects to include in the site. Defaults to None.
    baseobj : Part::FeaturePython, optional
        The base object for the site. Defaults to None.
    name : str, optional
        The name to assign to the created site. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created site object.
    """
    site = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Site",
        internalName="Site",
        defaultLabel=name if name else translate("Arch", "Site"),
    )

    # Initialize all relevant properties
    if objectslist:
        site.Group = objectslist
    if baseobj:
        import Part
        if isinstance(baseobj, Part.Shape):
            site.Shape = baseobj
        else:
            site.Terrain = baseobj

    return site


def makeSpace(objects=None, baseobj=None, name=None):
    """Creates a space object from the given objects.

    Parameters
    ----------
    objects : object or List(<SelectionObject>) or App::PropertyLinkSubList, optional
        The object or selection set that defines the space. If a single object is given,
        it becomes the base shape for the object. If the object or selection set contains
        subelements, these will be used as the boundaries to create the space. By default None.
    baseobj : object or List(<SelectionObject>) or App::PropertyLinkSubList, optional
        Currently unimplemented, it replaces and behaves in the same way as the objects parameter
        if defined. By default None.
    name : str, optional
        The user-facing name to assign to the space object's label. By default None, in
        which case the label is set to "Space".

    Returns
    -------
    Part::FeaturePython
        The created space object.

    Notes
    -----
    The objects parameter can be passed using either of these different formats:

    1. Single object (e.g. a Part::Feature document object). Will be used as the space's base
       shape.::
            objects = <Part::Feature>
    2. List of selection objects, as provided by ``Gui.Selection.getSelectionEx()``. This
       requires the GUI to be active. The `SubObjects` property of each selection object in the
       list defines the space's boundaries. If the list contains a single selection object without
       subobjects, or with only one subobject, the object in its ``Object`` property is used as
       the base shape.::
            objects = [<SelectionObject>, ...]
    3. A list of tuples that can be assigned to an ``App::PropertyLinkSubList`` property. Each
       tuple contains a document object and a nested tuple of subobjects that define the boundaries.
       If the list contains a single tuple without a nested subobjects tuple, or a subobjects tuple
       with only one subobject, the object in the tuple is used as the base shape.::
            objects = [(obj1, ("Face1")), (obj2, ("Face1")), ...]
            objects = [(obj, ("Face1", "Face2", "Face3", "Face4"))]
    """
    space = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Space",
        internalName="Space",
        defaultLabel=name if name else translate("Arch", "Space"),
    )

    # Initialize all relevant properties
    if baseobj:
        objects = baseobj
    if objects:
        if not isinstance(objects, list):
            objects = [objects]

        isSingleObject = lambda objs: len(objs) == 1

        # We assume that the objects list is not a mixed set. The type of the first
        # object will determine the type of the set.
        # Input to this function can come into three different formats. First convert it
        # to a common format: [ (<Part::Feature>, ["Face1", ...]), ... ]
        if (hasattr(objects[0], "isDerivedFrom") and
                objects[0].isDerivedFrom("Gui::SelectionObject")):
            # Selection set: convert to common format
            # [<SelectionObject>, ...]
            objects = [(obj.Object, obj.SubElementNames) for obj in objects]
        elif (isinstance(objects[0], tuple) or isinstance(objects[0], list)):
            # Tuple or list of object with subobjects: pass unmodified
            # [ (<Part::Feature>, ["Face1", ...]), ... ]
            pass
        else:
            # Single object: assume anything else passed is a single object with no
            # boundaries.
            # [ <Part::Feature> ]
            objects = [(objects[0], [])]

        if isSingleObject(objects):
            # For a single object, having boundaries is determined by them being defined
            # as more than one subelement (e.g. two faces)
            boundaries = [obj for obj in objects if len(obj[1]) > 1]
        else:
            boundaries = [obj for obj in objects if obj[1]]

        if isSingleObject(objects) and not boundaries:
            space.Base = objects[0][0]
            if FreeCAD.GuiUp:
                objects[0][0].ViewObject.hide()
        else:
            space.Proxy.addSubobjects(space, boundaries)
    return space

def addSpaceBoundaries(space, subobjects):
    """Adds the given subobjects as defining boundaries of the given space.

    Parameters
    ----------
    space : ArchSpace._Space
        Arch space object to add the boundaries to.
    subobjects : List(<SelectionObject>) or App::PropertyLinkSubList
        List of boundaries to add to the space.

    Notes
    -----
    The subobjects parameter can be passed using either of these different formats:

    1. List of selection objects, as provided by ``Gui.Selection.getSelectionEx()``. This
       requires the GUI to be active. The `SubObjects` property of each selection object in the
       list defines the boundaries to add to the space.::
            subobjects = [<SelectionObject>, ...]
    2. A list of tuples that can be assigned to an ``App::PropertyLinkSubList`` property. Each
       tuple contains a document object and a nested tuple of subobjects that define the boundaries
       to add.::
            subobjects = [(obj1, ("Face1")), (obj2, ("Face1")), ...]
            subobjects = [(obj, ("Face1", "Face2", "Face3", "Face4"))]
    """
    import Draft
    if Draft.getType(space) == "Space":
        space.Proxy.addSubobjects(space, subobjects)

def removeSpaceBoundaries(space, subobjects):
    """Remove the given subobjects as defining boundaries of the given space.

    Parameters
    ----------
    space : ArchSpace._Space
        Arch space object to remove the boundaries from.
    subobjects : List(<SelectionObject>) or App::PropertyLinkSubList
        List of boundaries to remove from the space.

    Notes
    -----
    The subobjects parameter can be passed using either of these different formats:

    1. List of selection objects, as provided by ``Gui.Selection.getSelectionEx()``. This
       requires the GUI to be active. The `SubObjects` property of each selection object in the
       list defines the boundaries to remove from the space.::
            subobjects = [<SelectionObject>, ...]
    2. A list of tuples that can be assigned to an ``App::PropertyLinkSubList`` property. Each
       tuple contains a document object and a nested tuple of subobjects that define the boundaries
       to remove.::
            subobjects = [(obj1, ("Face1")), (obj2, ("Face1")), ...]
            subobjects = [(obj, ("Face1", "Face2", "Face3", "Face4"))]
    """
    import Draft
    if Draft.getType(space) == "Space":
        space.Proxy.removeSubobjects(space, subobjects)

def makeStairs(baseobj=None, length=None, width=None, height=None, steps=None, name=None):
    """
    Creates a stairs object with the given attributes.

    Parameters
    ----------
    baseobj : Part::FeaturePython, optional
        The base object for the stairs. Defaults to None.
    length : float, optional
        The length of the stairs. Defaults to None.
    width : float, optional
        The width of the stairs. Defaults to None.
    height : float, optional
        The height of the stairs. Defaults to None.
    steps : int, optional
        The number of steps. Defaults to None.
    name : str, optional
        The name to assign to the created stairs. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created stairs object.
    """
    import ArchStairs
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return

    stairs = []
    additions = []
    label = name if name else translate("Arch", "Stairs")

    def setProperty(obj, length, width, height, steps):
        """setProperty(obj,length,width,height,steps): sets up the basic properties for this stair
        """
        obj.Length = length if length else params.get_param_arch("StairsLength")
        obj.Width = width if width else params.get_param_arch("StairsWidth")
        obj.Height = height if height else params.get_param_arch("StairsHeight")
        obj.Structure = "Massive"
        obj.StructureThickness = 150
        obj.DownSlabThickness = 150
        obj.UpSlabThickness = 150
        if steps:
            obj.NumberOfSteps = steps

        obj.RailingOffsetLeft = 60
        obj.RailingOffsetRight = 60
        obj.RailingHeightLeft = 900
        obj.RailingHeightRight = 900

    if baseobj:
        if not isinstance(baseobj, list):
            baseobj = [baseobj]
        lenSelection = len(baseobj)
        if lenSelection > 1:
            stair = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Stairs")
            stair.Label = label
            ArchStairs._Stairs(stair)
            stairs.append(stair)
            i = 1
        else:
            i = 0

        for baseobjI in baseobj:
            stair = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Stairs")
            stair.Label = label
            ArchStairs._Stairs(stair)
            stairs.append(stair)
            stairs[i].Base = baseobjI
            if steps:
                stepsI = steps
            else:
                stepsI = 20
            setProperty(stairs[i], None, width, height, stepsI)
            if i > 1:
                additions.append(stairs[i])
                stairs[i].LastSegment = stairs[i - 1]
            else:
                if len(stairs) > 1: # i.e. length >1, have a 'master' staircase created
                    stairs[0].Base = stairs[1]
            i += 1
        if lenSelection > 1:
            stairs[0].Additions = additions
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Stairs")
        obj.Label = label
        ArchStairs._Stairs(obj)
        setProperty(obj, length, width, height, steps)
        stairs.append(obj)
    if FreeCAD.GuiUp:
        if baseobj:
            for stair in stairs:
                ArchStairs._ViewProviderStairs(stair.ViewObject)
            for bo in baseobj:
                bo.ViewObject.hide()
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
    """
    Creates railings for the given stairs.

    Parameters
    ----------
    stairs : list of Part::FeaturePython
        The stairs objects to add railings to.

    Returns
    -------
    None
    """
    def makeRailingLorR(stairs, side="L"):
        """makeRailingLorR(stairs,side="L"): Creates a railing on the given side of the stairs, L or
        R"""
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
                lrRail = makePipe(
                    baseobj=None, diameter=0, length=0, placement=None,
                    name=translate("Arch", "Railing")
                )
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

    makeRailingLorR(stairs, "L")
    makeRailingLorR(stairs, "R")


def makeTruss(baseobj=None, name=None):
    """
    Creates a truss object from the given base object.

    Parameters
    ----------
    baseobj : Part::FeaturePython, optional
        The base object for the truss. Defaults to None.
    name : str, optional
        The name to assign to the created truss. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created truss object.
    """
    truss = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="Truss",
        internalName="Truss",
        defaultLabel=name if name else translate("Arch", "Truss"),
        moduleName="ArchTruss",
        viewProviderName="ViewProviderTruss",
    )

    # Initialize all relevant properties
    if baseobj:
        truss.Base = baseobj
        if FreeCAD.GuiUp:
            baseobj.ViewObject.hide()

    return truss


def makeWall(
        baseobj=None, height=None, length=None, width=None, align=None, offset=None,
        face=None, name=None
):
    """Create a wall based on a given object, and returns the generated wall.

    TODO: It is unclear what defines which units this function uses.

    Parameters
    ----------
    baseobj: <Part::Feature>, optional
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
    1. Creates a new <Part::FeaturePython> object, and turns it into a parametric wall
       object. This <Part::FeaturePython> object does not yet have any shape.
    2. The wall then uses the baseobj.Shape as the basis to extrude out a wall shape,
       giving the new <Part::FeaturePython> object a shape.
    3. It then hides the original baseobj.
    """
    import Draft

    wall = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Wall",
        internalName="Wall",
        defaultLabel=name if name else translate("Arch", "Wall"),
        moduleName="ArchWall",
        viewProviderName="_ViewProviderWall",
    )

    # Initialize all relevant properties
    if baseobj:
        if hasattr(baseobj, 'Shape') or baseobj.isDerivedFrom("Mesh::Feature"):
            wall.Base = baseobj
        else:
            FreeCAD.Console.PrintWarning(
                str(translate("Arch", "Walls can only be based on Part or Mesh objects"))
            )
    if face:
        wall.Face = face
    if length:
        wall.Length = length
    wall.Width = width if width else params.get_param_arch("WallWidth")
    wall.Height = height if height else params.get_param_arch("WallHeight")
    wall.Align = (
        align if align else ["Center", "Left", "Right"][params.get_param_arch("WallAlignment")]
    )

    if wall.Base and FreeCAD.GuiUp:
        if Draft.getType(wall.Base) != "Space":
            wall.Base.ViewObject.hide()

    return wall


def joinWalls(walls, delete=False, deletebase=False):
    """Join the given list of walls into one sketch-based wall.

    Take the first wall in the list, and adds on the other walls in the list.
    Return the modified first wall.

    Setting delete to True, will delete the other walls. Only join walls
    if the walls have the same width, height and alignment.

    Parameters
    ----------
    walls : list of <Part::FeaturePython>
        List containing the walls to add to the first wall in the list. Walls must
        be based off a base object.
    delete : bool, optional
        If True, deletes the other walls in the list. Defaults to False.
    deletebase : bool, optional
        If True, and delete is True, the base of the other walls is also deleted
        Defaults to False.

    Returns
    -------
    Part::FeaturePython
        The joined wall object.
    """
    import Part
    import Draft
    import ArchWall
    if not walls:
        return None
    if not isinstance(walls, list):
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
                newSk = ArchSketchObject.makeArchSketch()
            except:
                if Draft.getType(base.Base) != "Sketcher::SketchObject":
                    newSk = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject", "WallTrace")
                else:
                    newSk = None
            if newSk:
                sk = Draft.makeSketch(base.Base, autoconstraints=True, addTo=newSk)
                base.Base = sk
            else:
                sk = base.Base
    for w in walls:
        if w.Base and not w.Base.Shape.Faces:
            for hostedObj in w.Proxy.getHosts(w):
                if hasattr(hostedObj, "Host"):
                    hostedObj.Host = base
                else:
                    tmp = hostedObj.Hosts
                    if delete:
                        tmp.remove(w)
                    if not base in tmp:
                        tmp.append(base)
                    hostedObj.Hosts = tmp
            tmp = []
            for add in w.Additions:
                if not add in base.Additions:
                    tmp.append(add)
            if delete:
                w.Additions = None
            base.Additions += tmp
            tmp = []
            for sub in w.Subtractions:
                if not sub in base.Subtractions:
                    tmp.append(sub)
            if delete:
                w.Subtractions = None
            base.Subtractions += tmp
            for e in w.Base.Shape.Edges:
                l = e.Curve
                if isinstance(l, Part.Line):
                    l = Part.LineSegment(e.Vertexes[0].Point, e.Vertexes[-1].Point)
                sk.addGeometry(l)
                deleteList.append(w.Name)
                if deletebase:
                    deleteList.append(w.Base.Name)
    if delete:
        for n in deleteList:
            FreeCAD.ActiveDocument.removeObject(n)
    FreeCAD.ActiveDocument.recompute()
    if base.Base and FreeCAD.GuiUp:
        base.ViewObject.show()
    return base


def makeWindow(baseobj=None, width=None, height=None, parts=None, name=None):
    """
    Creates a window object based on the given base object.

    Parameters
    ----------
    baseobj : Draft.Wire or Sketcher.Sketch, optional
        The base object for the window. It should be a well-formed, closed
        Draft.Wire or Sketcher.Sketch object. Defaults to None.
    width : float, optional
        The width of the window. Defaults to None.
    height : float, optional
        The height of the window. Defaults to None.
    parts : list, optional
        The parts of the window. Defaults to None.
    name : str, optional
        The name to assign to the created window. Defaults to None.

    Returns
    -------
    Part::FeaturePython
        The created window object.

    Notes
    -----
    1. If baseobj is not a closed shape, the tool may not create a proper solid figure.
    """
    import Draft
    import DraftGeomUtils
    from draftutils import todo

    if baseobj and Draft.getType(baseobj) == "Window" and FreeCAD.ActiveDocument:
        window = Draft.clone(baseobj)
        return window

    window = _initializeArchObject(
        "Part::FeaturePython",
        baseClassName="_Window",
        internalName="Window",
        defaultLabel=name if name else translate("Arch", "Window"),
        moduleName="ArchWindow",
        viewProviderName="_ViewProviderWindow",
    )

    # Initialize all relevant properties
    if width:
        window.Width = width
    if height:
        window.Height = height
    if baseobj:
        # 2025.5.25
        # Historically, this normal was deduced by the orientation of the Base Sketch and hardcoded
        # in the Normal property. Now with the new AutoNormalReversed property/flag, set True as
        # default, the auto Normal previously in opposite direction to is now consistent with that
        # previously hardcoded. With the normal set to 'auto', window object would not suffer weird
        # shape if the Base Sketch is rotated by some reason. Keep the property be 'auto' (0,0,0)
        # here.
        #obj.Normal = baseobj.Placement.Rotation.multVec(FreeCAD.Vector(0, 0, -1))
        window.Base = baseobj
    if parts is not None:
        window.WindowParts = parts
    else:
        if baseobj:
            linked_obj = baseobj.getLinkedObject(True)
            if (linked_obj.isDerivedFrom("Part::Part2DObject")
                or Draft.getType(linked_obj) in ["BezCurve", "BSpline", "Wire"]) \
                    and DraftGeomUtils.isPlanar(baseobj.Shape):
                # "BezCurve", "BSpline" and "Wire" objects created with < v1.1 are
                # "Part::Part2DObject" objects. In all versions these objects need not be planar.
                if baseobj.Shape.Wires:
                    part_type = "Frame"
                    if len(baseobj.Shape.Wires) == 1:
                        part_type = "Solid panel"
                    wires = []
                    for i, wire in enumerate(baseobj.Shape.Wires):
                        if wire.isClosed():
                            wires.append(f"Wire{i}")
                    wires_str = ",".join(wires)
                    part_name = "Default"
                    part_frame_thickness = "1" # mm
                    part_offset = "0" # mm
                    window.WindowParts = [
                        part_name, part_type, wires_str, part_frame_thickness, part_offset
                    ]
            else:
                # bind properties from base obj if existing
                for prop in ["Height", "Width", "Subvolume", "Tag", "Description", "Material"]:
                    for baseobj_prop in baseobj.PropertiesList:
                        if (baseobj_prop == prop) or baseobj_prop.endswith(f"_{prop}"):
                            window.setExpression(prop, f"{baseobj.Name}.{baseobj_prop}")

    if window.Base and FreeCAD.GuiUp:
        from ArchWindow import recolorize
        window.Base.ViewObject.DisplayMode = "Wireframe"
        window.Base.ViewObject.hide()
        todo.ToDo.delay(recolorize, [window.Document.Name, window.Name])

    return window


def _initializeArchObject(
    objectType,
    baseClassName=None,
    internalName=None,
    defaultLabel=None,
    moduleName=None,
    viewProviderName=None,
):
    """
    Initializes a new Arch object in the active document.

    Parameters
    ----------
    objectType : str
        The type of object to create (e.g., "Part::FeaturePython").
    baseClassName : str
        The name of the base class to initialize the object (e.g., "_ArchSchedule").
    internalName : str, optional
        The internal name to assign to the object.
    defaultLabel : str, optional
        The default label to assign to the object if no name is provided.
    moduleName : str, optional
        The name of the module containing the base class and view provider. If not provided,
        it is inferred from baseClassName.
    viewProviderName : str, optional
        The name of the view provider class to initialize the object's view. If not provided,
        it is inferred from baseClassName.

    Returns
    -------
    App.DocumentObject
        The created object, or None if no active document exists.
    """
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return None

    import importlib

    # Infer moduleName and viewProviderName if not provided
    if not moduleName:
        moduleName = "Arch" + baseClassName.lstrip("_").strip("Arch")
    if not viewProviderName:
        viewProviderName = "_ViewProvider" + baseClassName.lstrip("_")

    obj = FreeCAD.ActiveDocument.addObject(objectType, internalName)
    if not obj:
        return None

    obj.Label = defaultLabel

    try:
        # Import module and initialize base class
        module = importlib.import_module(moduleName)
        baseClass = getattr(module, baseClassName, None)
        if not baseClass:
            FreeCAD.Console.PrintError(
                f"Base class '{baseClassName}' not found in module '{moduleName}'.\n"
            )
            return None
        baseClass(obj)

        # Initialize view provider
        if FreeCAD.GuiUp:
            viewProvider = getattr(module, viewProviderName, None)
            if not viewProvider:
                FreeCAD.Console.PrintWarning(
                    f"View provider '{viewProviderName}' not found in module '{moduleName}'.\n"
                )
            else:
                viewProvider(obj.ViewObject)

    except ImportError as e:
        FreeCAD.Console.PrintError(f"Failed to import module '{moduleName}': {e}\n")
        return None

    return obj
