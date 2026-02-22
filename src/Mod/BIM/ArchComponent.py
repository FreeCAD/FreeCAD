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

__title__ = "FreeCAD Arch Component"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchComponent
#  \ingroup ARCH
#  \brief The base class of all Arch objects
#
#  This module provides the base Arch component class, that
#  is shared by all of the Arch BIM objects

"""This module provides the base Arch component class, that is shared
by all of the Arch BIM objects.

Examples
--------
TODO put examples here.
"""

import math

import FreeCAD
import ArchCommands
import ArchIFC
import Draft

from draftutils import params

if FreeCAD.GuiUp:
    from PySide import QtGui, QtCore
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # \endcond


def addToComponent(compobject, addobject, mod=None):
    """Add an object to a component's properties.

    Does not run if the addobject already exists in the component's properties.
    Adds the object to the first property found of Base, Group, or Hosts.

    If mod is provided, adds the object to that property instead.

    Parameters
    ----------
    compobject: <ArchComponent.Component>
        The component object to add the object to.
    addobject: <App::DocumentObject>
        The object to add to the component.
    mod: str, optional
        The property to add the object to.
    """

    import Draft

    if compobject == addobject:
        return
    # first check zis already there
    found = False
    attribs = ["Additions", "Objects", "Components", "Subtractions", "Base", "Group", "Hosts"]
    for a in attribs:
        if hasattr(compobject, a):
            if a == "Base":
                if addobject == getattr(compobject, a):
                    found = True
            else:
                if addobject in getattr(compobject, a):
                    found = True
    if not found:
        if mod:
            if hasattr(compobject, mod):
                if mod == "Base":
                    setattr(compobject, mod, addobject)
                    addobject.ViewObject.hide()
                elif mod == "Axes":
                    if Draft.getType(addobject) == "Axis":
                        l = getattr(compobject, mod)
                        l.append(addobject)
                        setattr(compobject, mod, l)
                else:
                    l = getattr(compobject, mod)
                    l.append(addobject)
                    setattr(compobject, mod, l)
                    if mod != "Objects":
                        addobject.ViewObject.hide()
                        if Draft.getType(compobject) == "PanelSheet":
                            addobject.Placement.move(compobject.Placement.Base.negative())
        else:
            for a in attribs[:3]:
                if hasattr(compobject, a):
                    l = getattr(compobject, a)
                    l.append(addobject)
                    setattr(compobject, a, l)
                    addobject.ViewObject.hide()
                    break


def removeFromComponent(compobject, subobject):
    """Remove the object from the given component.

    Try to find the object in the component's properties. If found, remove the
    object.

    If the object is not found, add the object in the component's Subtractions
    property.

    Parameters
    ----------
    compobject: <ArchComponent.Component>
        The component to remove the object from.
    subobject: <App::DocumentObject>
        The object to remove from the component.
    """

    if compobject == subobject:
        return
    found = False
    attribs = [
        "Additions",
        "Subtractions",
        "Objects",
        "Components",
        "Base",
        "Axes",
        "Fixtures",
        "Group",
        "Hosts",
    ]
    for a in attribs:
        if hasattr(compobject, a):
            if a == "Base":
                if subobject == getattr(compobject, a):
                    setattr(compobject, a, None)
                    subobject.ViewObject.show()
                    found = True
            else:
                if subobject in getattr(compobject, a):
                    l = getattr(compobject, a)
                    l.remove(subobject)
                    setattr(compobject, a, l)
                    subobject.ViewObject.show()
                    if Draft.getType(compobject) == "PanelSheet":
                        subobject.Placement.move(compobject.Placement.Base)
                    found = True
    if not found:
        if hasattr(compobject, "Subtractions"):
            l = compobject.Subtractions
            l.append(subobject)
            compobject.Subtractions = l
            if (Draft.getType(subobject) != "Window") and (
                not Draft.isClone(subobject, "Window", True)
            ):
                ArchCommands.setAsSubcomponent(subobject)


class Component(ArchIFC.IfcProduct):
    """The Arch Component object.

    Acts as a base for all other Arch objects, such as Arch walls and Arch
    structures. Its properties and behaviours are common to all Arch objects.

    You can learn more about Arch Components, and the purpose of Arch
    Components here: https://wiki.freecad.org/Arch_Component

    Parameters
    ----------
    obj: <App::FeaturePython>
        The object to turn into an Arch Component
    """

    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "Component"
        Component.setProperties(self, obj)

    def setProperties(self, obj):
        """Give the component its component specific properties, such as material.

        You can learn more about properties here:
        https://wiki.freecad.org/property
        """

        ArchIFC.IfcProduct.setProperties(self, obj)

        pl = obj.PropertiesList
        if not "Base" in pl:
            obj.addProperty(
                "App::PropertyLink",
                "Base",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "The base object this component is built upon"),
                locked=True,
            )
        if not "CloneOf" in pl:
            obj.addProperty(
                "App::PropertyLink",
                "CloneOf",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "The object this component is cloning"),
                locked=True,
            )
        if not "Additions" in pl:
            obj.addProperty(
                "App::PropertyLinkList",
                "Additions",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "Other shapes that are appended to this object"),
                locked=True,
            )
        if not "Subtractions" in pl:
            obj.addProperty(
                "App::PropertyLinkList",
                "Subtractions",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Other shapes that are subtracted from this object"
                ),
                locked=True,
            )
        if not "Description" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Description",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "An optional description for this component"),
                locked=True,
            )
        if not "Tag" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Tag",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "An optional tag for this component"),
                locked=True,
            )
        if not "StandardCode" in pl:
            obj.addProperty(
                "App::PropertyString",
                "StandardCode",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "An optional standard (OmniClass, etc.) code for this component",
                ),
                locked=True,
            )
        if not "Material" in pl:
            obj.addProperty(
                "App::PropertyLink",
                "Material",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "A material for this object"),
                locked=True,
            )
        if "BaseMaterial" in pl:
            obj.Material = obj.BaseMaterial
            obj.removeProperty("BaseMaterial")
            FreeCAD.Console.PrintMessage(
                "Upgrading " + obj.Label + " BaseMaterial property to Material\n"
            )
        if not "MoveBase" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "MoveBase",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Specifies if moving this object moves its base instead"
                ),
                locked=True,
            )
            obj.MoveBase = params.get_param_arch("MoveBase")
        if not "MoveWithHost" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "MoveWithHost",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Specifies if this object must move together when its host is moved",
                ),
                locked=True,
            )
            obj.MoveWithHost = params.get_param_arch("MoveWithHost")
        if not "VerticalArea" in pl:
            obj.addProperty(
                "App::PropertyArea",
                "VerticalArea",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "The area of all vertical faces of this object"),
                locked=True,
            )
            obj.setEditorMode("VerticalArea", 1)
        if not "HorizontalArea" in pl:
            obj.addProperty(
                "App::PropertyArea",
                "HorizontalArea",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The area of the projection of this object onto the XY plane"
                ),
                locked=True,
            )
            obj.setEditorMode("HorizontalArea", 1)
        if not "PerimeterLength" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "PerimeterLength",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "The perimeter length of the horizontal area"),
                locked=True,
            )
            obj.setEditorMode("PerimeterLength", 1)
        if not "HiRes" in pl:
            obj.addProperty(
                "App::PropertyLink",
                "HiRes",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property", "An optional higher-resolution mesh or shape for this object"
                ),
                locked=True,
            )
        if not "Axis" in pl:
            obj.addProperty(
                "App::PropertyLink",
                "Axis",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "An optional axis or axis system on which this object should be duplicated",
                ),
                locked=True,
            )

        self.Subvolume = None
        # self.MoveWithHost = False

    def onDocumentRestored(self, obj):
        """Method run when the document is restored. Re-add the Arch component properties.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        """
        Component.setProperties(self, obj)

    def execute(self, obj):
        """Method run when the object is recomputed.

        If the object is a clone, just copy the shape it's cloned from.

        Process subshapes of the object to add additions, and subtract
        subtractions from the object's shape.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        """
        import Part

        if self.clone(obj):
            return
        if self.ensureBase(obj) is False:
            # This will fall through if the Component object has no base, allowing the base shapeto
            # be cleared
            return

        # Only proceed if a Base object is linked and contains valid geometry.
        if obj.Base and hasattr(obj.Base, "Shape") and not obj.Base.Shape.isNull():
            # Create a standalone shape as a deep copy of the base geometry, to avoid modifying
            # the original source.
            base_shape = Part.Shape(obj.Base.Shape)

            # Reset the shape's internal placement to Identity. This strips the placement
            # inherited from the Base object, ensuring the geometry is centered at (0,0,0) for
            # Boolean operations in processSubShapes. This also prevents the shape's placement from
            # overwriting the Component's own Placement property during assignment in applyShape.
            base_shape.Placement = FreeCAD.Placement()

            # Localize the CSG shapes: pass the object's placement to processSubShapes, so that the
            # placements of any additions and subtractions are also localized to the local origin of
            # the Arch Component.
            final_shape = self.processSubShapes(obj, base_shape, obj.Placement)
            self.applyShape(obj, final_shape, obj.Placement, allownosolid=True)
        else:
            # Clear the shape if the base has been removed. This avoids leaving a stale shape that
            # is not updated when the base is removed.
            obj.Shape = Part.Shape()

    def dumps(self):
        return None

    def loads(self, state):
        self.Type = "Component"

    def onBeforeChange(self, obj, prop):
        """Method called before the object has a property changed.

        Specifically, this method is called before the value changes.

        If "Placement" has changed, record the old placement, so that
        .onChanged() can compare between the old and new placement, and move
        its children accordingly.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        prop: string
            The name of the property that has changed.
        """
        if prop == "Placement":
            self.oldPlacement = FreeCAD.Placement(obj.Placement)

    def onChanged(self, obj, prop):
        """Method called when the object has a property changed.

        If "Placement" has changed, move any children components that have been
        set to move with their host, such that they stay in the same location
        to this component.

        Also call ArchIFC.IfcProduct.onChanged().

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        prop: string
            The name of the property that has changed.
        """

        ArchIFC.IfcProduct.onChanged(self, obj, prop)

        if prop == "Placement":
            if hasattr(self, "oldPlacement") and self.oldPlacement != obj.Placement:
                deltap = obj.Placement.Base.sub(self.oldPlacement.Base)
                if deltap.Length == 0:
                    deltap = None
                deltar = obj.Placement.Rotation * self.oldPlacement.Rotation.inverted()
                if deltar.Angle < 0.0001:
                    deltar = None
                for child in self.getMovableChildren(obj):
                    if deltar:
                        child.Placement.rotate(
                            self.oldPlacement.Base,
                            deltar.Axis,
                            math.degrees(deltar.Angle),
                            comp=True,
                        )
                    if deltap:
                        child.Placement.move(deltap)

    def getMovableChildren(self, obj):
        """Find the component's children set to move with their host.

        In this case, children refer to Additions, Subtractions, and objects
        linked to this object that refer to it as a host in the "Host" or
        "Hosts" properties. Objects are set to move with their host via the
        MoveWithHost property.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.

        Returns
        -------
        list of <App::FeaturePython>
            List of child objects set to move with their host.
        """

        ilist = obj.Additions + obj.Subtractions
        for o in obj.InList:
            if hasattr(o, "Hosts"):
                if obj in o.Hosts:
                    ilist.append(o)
            elif hasattr(o, "Host"):
                if obj == o.Host:
                    ilist.append(o)

        # Stairs railings should be considered as children
        # (RailingLeft and RailingRight property)
        if hasattr(obj, "RailingLeft") and obj.RailingLeft:
            ilist.append(obj.RailingLeft)
        if hasattr(obj, "RailingRight") and obj.RailingRight:
            ilist.append(obj.RailingRight)

        ilist2 = []
        for o in ilist:
            if hasattr(o, "MoveWithHost"):
                if o.MoveWithHost:
                    ilist2.append(o)
            else:
                ilist2.append(o)
        return ilist2

    def getParentHeight(self, obj):
        """Get a height value from hosts.

        Recursively crawl hosts until a Floor or BuildingPart is found, then
        return the value of its Height property.

        Parameters
        ---------
        obj: <App::FeaturePython>
            The component object.

        Returns
        -------
        <App::PropertyLength>
            The Height value of the found Floor or BuildingPart.
        """

        for parent in obj.InList:
            if Draft.getType(parent) in ["Floor", "BuildingPart"]:
                if obj in parent.Group:
                    if parent.HeightPropagate:
                        if parent.Height.Value:
                            return parent.Height.Value
        # not found? get one level higher
        for parent in obj.InList:
            if hasattr(parent, "Group"):
                if obj in parent.Group:
                    return self.getParentHeight(parent)
        # still not found? check if we are embedded
        for parent in obj.InList:
            if hasattr(parent, "Additions"):
                if obj in parent.Additions:
                    return self.getParentHeight(parent)
        return 0

    def clone(self, obj):
        """If the object is a clone, copy the shape.

        If the object is a clone according to the "CloneOf" property, copy the
        object's shape and several properties relating to shape, such as
        "Length" and "Thickness".

        Only perform the copy if this object and the object it's a clone of are
        of the same type, or if the object has the type "Component" or
        "BuildingPart".

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.

        Returns
        -------
        bool
            True if the copy occurs, False if otherwise.
        """

        if hasattr(obj, "CloneOf"):
            if obj.CloneOf:
                if (Draft.getType(obj.CloneOf) == Draft.getType(obj)) or (
                    Draft.getType(obj) in ["Component", "BuildingPart"]
                ):
                    pl = obj.Placement
                    ## TODO use Part.Shape() instead?
                    obj.Shape = obj.CloneOf.Shape.copy()
                    obj.Placement = pl
                    for prop in [
                        "Length",
                        "Width",
                        "Height",
                        "Thickness",
                        "Area",
                        "PerimeterLength",
                        "HorizontalArea",
                        "VerticalArea",
                    ]:
                        if hasattr(obj, prop) and hasattr(obj.CloneOf, prop):
                            setattr(obj, prop, getattr(obj.CloneOf, prop))
                    return True
        return False

    def getSiblings(self, obj):
        """Find objects that have the same Base object, and type.

        Look to base object, and find other objects that are based off this
        base object. If these objects are the same type, return them.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.

        Returns
        -------
        list of <App::FeaturePython>
            List of objects that have the same Base and type as this component.
        """

        if not hasattr(obj, "Base"):
            return []
        if not obj.Base:
            return []
        siblings = []
        for o in obj.Base.InList:
            if hasattr(o, "Base"):
                if o.Base:
                    if o.Base.Name == obj.Base.Name:
                        if o.Name != obj.Name:
                            if Draft.getType(o) == Draft.getType(obj):
                                siblings.append(o)
        return siblings

    def getExtrusionData(self, obj):
        """Get the object's extrusion data.

        Recursively scrape the Bases of the object, until a Base that is
        derived from a <Part::Extrusion> is found. From there, copy the
        extrusion to the (0,0,0) origin.

        With this copy, get the <Part.Face> the shape was originally
        extruded from, the <Base.Vector> of the extrusion, and the
        <Base.Placement> needed to move the copy back to its original
        location/orientation. Return this data as a tuple.

        If an object derived from a <Part::Multifuse> is encountered, return
        this data as a tuple containing lists. The lists will contain the same
        data as above, from each of the objects within the multifuse.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.

        Returns
        -------
        tuple
            Tuple containing:

            1) The <Part.Face> the object was extruded from.
            2) The <Base.Vector> of the extrusion.
            3) The <Base.Placement> of the extrusion.
        """

        if hasattr(obj, "CloneOf"):
            if obj.CloneOf:
                if hasattr(obj.CloneOf, "Proxy"):
                    if hasattr(obj.CloneOf.Proxy, "getExtrusionData"):
                        data = obj.CloneOf.Proxy.getExtrusionData(obj.CloneOf)
                        if data:
                            return data

        if obj.Base:
            # the base is another arch object which can provide extrusion data
            if (
                hasattr(obj.Base, "Proxy")
                and hasattr(obj.Base.Proxy, "getExtrusionData")
                and (not obj.Additions)
                and (not obj.Subtractions)
            ):
                if obj.Base.Base:
                    if obj.Placement.Rotation.Angle < 0.0001:
                        # if the final obj is rotated, this will screw all our IFC orientation. Better leave it like that then...
                        data = obj.Base.Proxy.getExtrusionData(obj.Base)
                        if data:
                            return data
                            # TODO above doesn't work if underlying shape is not at (0,0,0). But code below doesn't work well yet
                            # add the displacement of the final object
                            disp = obj.Shape.CenterOfMass.sub(obj.Base.Shape.CenterOfMass)
                            if isinstance(data[2], (list, tuple)):
                                ndata2 = []
                                for p in data[2]:
                                    p.move(disp)
                                    ndata2.append(p)
                                return (data[0], data[1], ndata2)
                            else:
                                ndata2 = data[2]
                                ndata2.move(disp)
                                return (data[0], data[1], ndata2)

            # the base is a Part Extrusion
            elif obj.Base.isDerivedFrom("Part::Extrusion"):
                if obj.Base.Base and len(obj.Base.Base.Shape.Wires) == 1:
                    base, placement = self.rebase(obj.Base.Base.Shape)
                    extrusion = FreeCAD.Vector(obj.Base.Dir).normalize()
                    if extrusion.Length == 0:
                        extrusion = FreeCAD.Vector(0, 0, 1)
                    else:
                        extrusion = placement.inverse().Rotation.multVec(extrusion)
                    if hasattr(obj.Base, "LengthFwd"):
                        if obj.Base.LengthFwd.Value:
                            extrusion = extrusion.multiply(obj.Base.LengthFwd.Value)
                    if not self.isIdentity(obj.Base.Placement):
                        placement = placement.multiply(obj.Base.Placement)
                    return (base, extrusion, placement)

            elif obj.Base.isDerivedFrom("Part::MultiFuse"):
                rshapes = []
                revs = []
                rpls = []
                for sub in obj.Base.Shapes:
                    if sub.isDerivedFrom("Part::Extrusion"):
                        if sub.Base:
                            base, placement = self.rebase(sub.Base.Shape)
                            extrusion = FreeCAD.Vector(sub.Dir).normalize()
                            if extrusion.Length == 0:
                                extrusion = FreeCAD.Vector(0, 0, 1)
                            else:
                                extrusion = placement.inverse().Rotation.multVec(extrusion)
                            if hasattr(sub, "LengthFwd"):
                                if sub.LengthFwd.Value:
                                    extrusion = extrusion.multiply(sub.LengthFwd.Value)
                            placement = obj.Placement.multiply(placement)
                            rshapes.append(base)
                            revs.append(extrusion)
                            rpls.append(placement)
                    else:
                        exdata = ArchCommands.getExtrusionData(sub.Shape)
                        if exdata:
                            base, placement = self.rebase(exdata[0])
                            extrusion = placement.inverse().Rotation.multVec(exdata[1])
                            placement = obj.Placement.multiply(placement)
                            rshapes.append(base)
                            revs.append(extrusion)
                            rpls.append(placement)
                if rshapes and revs and rpls:
                    return (rshapes, revs, rpls)
        return None

    def rebase(self, shape, hint=None):
        """Copy a shape to the (0,0,0) origin.

        Create a copy of a shape, such that its center of mass is in the
        (0,0,0) origin.

        TODO Determine the way the shape is rotated by this method.

        Return the copy of the shape, and the <Base.Placement> needed to move
        the copy back to its original location/orientation.

        Parameters
        ----------
        shape: <Part.Shape>
            The shape to copy.
        hint: <Base.Vector>, optional
            If the angle between the normal vector of the shape, and the hint
            vector is greater than 90 degrees, the normal will be reversed
            before being rotated.
        """

        import DraftGeomUtils

        # Get the object's center.
        if not isinstance(shape, list):
            shape = [shape]
        if hasattr(shape[0], "CenterOfMass"):
            v = shape[0].CenterOfMass
        else:
            v = shape[0].BoundBox.Center

        # Get the object's normal.
        n = DraftGeomUtils.getNormal(shape[0])
        if (not n) or (not n.Length):
            n = FreeCAD.Vector(0, 0, 1)

        # Reverse the normal if the hint vector and the normal vector have more
        # than a 90 degree angle between them.
        if hint and hint.getAngle(n) > 1.58:
            n = n.negative()

        r = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), n)
        if round(abs(r.Angle), 8) == round(math.pi, 8):
            r = FreeCAD.Rotation()

        shapes = []
        for s in shape:
            ## TODO use Part.Shape() instead?
            s = s.copy()
            s.translate(v.negative())
            s.rotate(FreeCAD.Vector(0, 0, 0), r.Axis, math.degrees(-r.Angle))
            shapes.append(s)
        p = FreeCAD.Placement()
        p.Base = v
        p.Rotation = r
        if len(shapes) == 1:
            return (shapes[0], p)
        else:
            return (shapes, p)

    def hideSubobjects(self, obj, prop):
        """Hides Additions and Subtractions of this Component when that list changes.

        Intended to be used in conjunction with the .onChanged() method, to
        access the property that has changed.

        When an object loses or gains an Addition, this method hides all
        Additions.  When it gains or loses a Subtraction, this method hides all
        Subtractions.

        Does not effect objects of type Window, or clones of Windows.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        prop: string
            The name of the property that has changed.
        """

        if FreeCAD.GuiUp:
            if prop in ["Additions", "Subtractions"]:
                if hasattr(obj, prop):
                    for o in getattr(obj, prop):
                        if (Draft.getType(o) != "Window") and (
                            not Draft.isClone(o, "Window", True)
                        ):
                            if Draft.getType(obj) == "Wall":
                                if Draft.getType(o) == "Roof":
                                    continue
                            o.ViewObject.hide()
            elif prop == "HiRes":
                if hasattr(obj, prop):
                    o = getattr(obj, prop)
                    if o:
                        o.ViewObject.hide()

    def handleComponentRemoval(self, obj, subobject):
        """
        Default handler for when a component is removed via the Task Panel.
        Subclasses can override this to provide special behavior.
        """
        removeFromComponent(obj, subobject)

    def processSubShapes(self, obj, base, placement=None):
        """Add Additions and Subtractions to a base shape.

        If Additions exist, fuse them to the base shape. If no base is
        provided, just fuse other additions to the first addition.

        If Subtractions exist, cut them from the base shape. Roofs and Windows
        are treated uniquely, as they define their own Shape to subtract from
        parent shapes using their .getSubVolume() methods.

        TODO determine what the purpose of the placement argument is.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        base: <Part.Shape>, optional
            The base shape to add Additions and Subtractions to.
        placement: <Base.Placement>, optional
            Prior to adding or subtracting subshapes, the <Base.Placement> of
            the subshapes are multiplied by the inverse of this parameter.

        Returns
        -------
        <Part.Shape>
            The base shape, with the additions and subtractions performed.
        """

        import Draft
        import Part

        # print("Processing subshapes of ",obj.Label, " : ",obj.Additions)

        if placement:
            if self.isIdentity(placement):
                placement = None
            else:
                placement = FreeCAD.Placement(placement)
                placement = placement.inverse()

        # treat additions
        for o in obj.Additions:

            # Arch Objects can have no Base, but Additions only
            # If there is no base/base isNull, 1st Addition becomes 'base',
            # placement should be treated as rest of Additions.
            # if not base:
            if not base or base.isNull():
                if hasattr(o, "Shape"):
                    base = Part.Shape(o.Shape)  # base = o.Shape
                    # Base is first Addition, treat placement as other Additions
                    if placement:
                        # see https://forum.freecad.org/viewtopic.php?p=579754#p579754
                        base.Placement = placement.multiply(base.Placement)
            else:
                # base.isNull() case grouped into if condition above, no need
                # if/else below.  Remarked out 2025.9.2
                #
                # if base.isNull():
                #    if hasattr(o,'Shape'):
                #        base = o.Shape
                # else:
                # special case, both walls with coinciding endpoints
                import ArchWall

                js = ArchWall.mergeShapes(o, obj)
                if js:
                    add = js.cut(base)
                    if placement:
                        # see https://forum.freecad.org/viewtopic.php?p=579754#p579754
                        add.Placement = placement.multiply(add.Placement)
                    base = base.fuse(add)
                elif hasattr(o, "Shape"):
                    if o.Shape and not o.Shape.isNull() and o.Shape.Solids:
                        # TODO use Part.Shape() instead?
                        s = o.Shape.copy()
                        if placement:
                            # see https://forum.freecad.org/viewtopic.php?p=579754#p579754
                            s.Placement = placement.multiply(s.Placement)
                        if base:
                            if base.Solids:
                                try:
                                    base = base.fuse(s)
                                except Part.OCCError:
                                    print(
                                        "Arch: unable to fuse object ", obj.Name, " with ", o.Name
                                    )
                        else:
                            base = s

        # treat subtractions
        subs = obj.Subtractions
        for link in obj.InListRecursive:
            if hasattr(link, "Host"):
                if (
                    Draft.getType(link) != "Rebar"
                    and link.Host == obj
                    and not self._objectInInternalLinkgroup(link)
                ):
                    subs.append(link)
            elif hasattr(link, "Hosts"):
                if obj in link.Hosts and not self._objectInInternalLinkgroup(link):
                    subs.append(link)
        for o in subs:
            if base:
                if base.isNull():
                    base = None

            if base:
                subvolume = None

                if (Draft.getType(o.getLinkedObject()) == "Window") or (
                    Draft.isClone(o, "Window", True)
                ):
                    # windows can be additions or subtractions, treated the same way
                    subvolume = o.getLinkedObject().Proxy.getSubVolume(
                        o, host=obj
                    )  # pass host obj (mostly Wall)
                elif (Draft.getType(o) == "Roof") or (Draft.isClone(o, "Roof")):
                    # roofs define their own special subtraction volume
                    subvolume = o.Proxy.getSubVolume(o).copy()
                elif hasattr(o, "Subvolume") and hasattr(o.Subvolume, "Shape"):
                    # Any other object with a Subvolume property
                    ## TODO - Part.Shape() instead?
                    subvolume = o.Subvolume.Shape.copy()
                    if hasattr(o, "Placement"):
                        # see https://forum.freecad.org/viewtopic.php?p=579754#p579754
                        subvolume.Placement = o.Placement.multiply(subvolume.Placement)

                if subvolume:
                    if base.Solids and subvolume.Solids:
                        if placement:
                            # see https://forum.freecad.org/viewtopic.php?p=579754#p579754
                            subvolume.Placement = placement.multiply(subvolume.Placement)
                        if len(base.Solids) > 1:
                            base = Part.makeCompound([sol.cut(subvolume) for sol in base.Solids])
                        else:
                            base = base.cut(subvolume)
                elif hasattr(o, "Shape"):
                    # no subvolume, we subtract the whole shape
                    if o.Shape:
                        if not o.Shape.isNull():
                            if o.Shape.Solids and base.Solids:
                                ## TODO use Part.Shape() instead?
                                s = o.Shape.copy()
                                if placement:
                                    # see https://forum.freecad.org/viewtopic.php?p=579754#p579754
                                    s.Placement = placement.multiply(s.Placement)
                                try:
                                    if len(base.Solids) > 1:
                                        base = Part.makeCompound(
                                            [sol.cut(s) for sol in base.Solids]
                                        )
                                    else:
                                        base = base.cut(s)
                                except Part.OCCError:
                                    print("Arch: unable to cut object ", o.Name, " from ", obj.Name)
        return base

    def spread(self, obj, shape, placement=None):
        """Copy the object to its Axis's points.

        If the object has the "Axis" property assigned, create a copy of the
        shape for each point on the object assigned as the "Axis".  Translate
        each of these copies equal to the displacement of the points from the
        (0,0,0) origin.

        If the object's "Axis" is unassigned, return the original shape
        unchanged.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        shape: <Part.Shape>
            The shape to copy.
        placement:
            Does nothing.

        Returns
        -------
        <Part.Shape>
            The shape, either spread to the axis points, or unchanged.
        """

        points = None
        if hasattr(obj, "Axis"):
            if obj.Axis:
                if hasattr(obj.Axis, "Proxy"):
                    if hasattr(obj.Axis.Proxy, "getPoints"):
                        points = obj.Axis.Proxy.getPoints(obj.Axis)
                if not points:
                    if hasattr(obj.Axis, "Shape"):
                        points = [v.Point for v in obj.Axis.Shape.Vertexes]
        if points:
            shps = []
            for p in points:
                ## TODO use Part.Shape() instead?
                sh = shape.copy()
                sh.translate(p)
                shps.append(sh)
            import Part

            shape = Part.makeCompound(shps)
        return shape

    def isIdentity(self, placement):
        """Check if a placement is *almost* zero.

        Check if a <Base.Placement>'s displacement from (0,0,0) is almost zero,
        and if the angle of its rotation about its axis is almost zero.

        Parameters
        ----------
        placement: <Base.Placement>
            The placement to examine.

        Returns
        -------
        bool
            Returns true if angle and displacement are almost zero, false it
            otherwise.
        """

        if (placement.Base.Length < 0.000001) and (placement.Rotation.Angle < 0.000001):
            return True
        return False

    def applyShape(self, obj, shape, placement, allowinvalid=False, allownosolid=False):
        """Check the given shape, then assign it to the object.

        Check if the shape is valid, isn't null, and if it has volume. Remove
        redundant edges from the shape. Spread the shape to the "Axis" with
        method .spread().

        Set the object's Shape and Placement to the values given, if
        successful.

        Finally, run .computeAreas() method, to calculate the horizontal and
        vertical area of the shape.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.
        shape: <Part.Shape>
            The shape to check and apply to the object.
        placement: <Base.Placement>
            The placement to apply to the object.
        allowinvalid: bool, optional
            Whether to allow invalid shapes, or to throw an error.
        allownosolid: bool, optional
            Whether to allow non-solid shapes, or to throw an error.
        """

        if shape:
            if not shape.isNull():
                if shape.isValid():
                    if shape.Solids:
                        if shape.Volume < 0:
                            shape.reverse()
                        if shape.Volume < 0:
                            FreeCAD.Console.PrintError(
                                translate("Arch", "Error computing the shape of this object") + "\n"
                            )
                            return
                        import Part

                        try:
                            r = shape.removeSplitter()
                        except Part.OCCError:
                            pass
                        else:
                            shape = r
                        p = self.spread(
                            obj, shape, placement
                        ).Placement.copy()  # for some reason this gets zeroed in next line
                        obj.Shape = self.spread(obj, shape, placement)
                        if not self.isIdentity(placement):
                            obj.Placement = placement
                        else:
                            obj.Placement = p
                    else:
                        if allownosolid:
                            obj.Shape = self.spread(obj, shape, placement)
                            if not self.isIdentity(placement):
                                obj.Placement = placement
                        else:
                            FreeCAD.Console.PrintWarning(
                                obj.Label + " " + translate("Arch", "has no solid") + "\n"
                            )
                else:
                    if allowinvalid:
                        obj.Shape = self.spread(obj, shape, placement)
                        if not self.isIdentity(placement):
                            obj.Placement = placement
                    else:
                        FreeCAD.Console.PrintWarning(
                            obj.Label + " " + translate("Arch", "has an invalid shape") + "\n"
                        )
            else:
                FreeCAD.Console.PrintWarning(
                    obj.Label + " " + translate("Arch", "has a null shape") + "\n"
                )
        self.computeAreas(obj)

    def computeAreas(self, obj):
        """Compute the area properties of the object's shape.

        This function calculates and assigns the following properties to the object:
        - **VerticalArea**: The total area of all vertical faces of the object.
        - **HorizontalArea**: The area of the object's projection onto the XY plane.
        - **PerimeterLength**: The perimeter of the horizontal area.

        The function uses the `AreaCalculator` helper class to perform these calculations.
        Refer to that class for more details on the calculation.

        Parameters
        ----------
        obj : App::FeaturePython
            The component object whose area properties are to be computed.
        """
        calculator = AreaCalculator(obj)
        calculator.compute()

    def isStandardCase(self, obj):
        """Determine if the component is a standard case of its IFC type.

        Not all IFC types have a standard case.

        If an object is a standard case or not varies between the different
        types. Each type has its own rules to define what is a standard case.

        Rotated objects, or objects with Additions or Subtractions are not
        standard cases.

        All objects whose IfcType is suffixed with the string " Sandard Case"
        are automatically a standard case.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The component object.

        Returns
        -------
        bool
            Whether the object is a standard case or not.
        """

        # Standard Case has been set manually by the user
        if obj.IfcType.endswith("Standard Case"):
            return True
        # Try to guess
        if obj.IfcType + " Standard Case" in ArchIFC.IfcTypes:
            # this type has a standard case
            if obj.Additions or obj.Subtractions:
                return False
            if obj.Placement.Rotation.Axis.getAngle(FreeCAD.Vector(0, 0, 1)) > 0.01:
                # reject rotated objects
                return False
            if obj.CloneOf:
                return obj.CloneOf.Proxy.isStandardCase(obj.CloneOf)
            if obj.IfcType == "Wall":
                # rules:
                # - vertically extruded
                # - single baseline or no baseline
                if (not obj.Base) or (len(obj.Base.Shape.Edges) == 1):
                    if hasattr(obj, "Normal"):
                        if obj.Normal in [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1)]:
                            return True
            elif obj.IfcType in ["Beam", "Column", "Slab"]:
                # rules:
                # - have a single-wire profile or no profile
                # - extrusion direction is perpendicular to the profile
                if obj.Base and (len(obj.Base.Shape.Wires) != 1):
                    return False
                if not hasattr(obj, "Normal"):
                    return False
                if hasattr(obj, "Tool") and obj.Tool:
                    return False
                if obj.Normal == FreeCAD.Vector(0, 0, 0):
                    return True
                elif len(obj.Base.Shape.Wires) == 1:
                    import DraftGeomUtils

                    n = DraftGeomUtils.getNormal(obj.Base.Shape)
                    if n:
                        if (n.getAngle(obj.Normal) < 0.01) or (
                            abs(n.getAngle(obj.Normal) - 3.14159) < 0.01
                        ):
                            return True
            # TODO: Support windows and doors
            # rules:
            # - must have a rectangular shape
            # - must have a host
            # - must be parallel to the host plane
            # - must have an IfcWindowType and IfcRelFillsElement (to be implemented in IFC exporter)
            return False

    def getHosts(self, obj):
        """Return the objects that have this one as host,
        that is, objects with a "Host" property pointing
        at this object, or a "Hosts" property containing
        this one.

        Returns
        -------
        list of <Arch._Structure>
            The BIM Structures hosting this component.
        """

        hosts = []

        for link in obj.InListRecursive:
            if hasattr(link, "Host"):
                if link.Host == obj and not self._objectInInternalLinkgroup(link):
                    hosts.append(link)
            elif hasattr(link, "Hosts"):
                if obj in link.Hosts and not self._objectInInternalLinkgroup(link):
                    hosts.append(link)
        return hosts

    def ensureBase(self, obj):
        """Returns False if the object has a Base but of the wrong type.
        Either returns True"""
        # TODO: this method has a third undocumented state: None, which is returned if the object
        # has no Base. This should either be fixed if unintended, or documented if intended.

        if getattr(obj, "Base", None):
            if obj.Base.isDerivedFrom("Part::Feature"):
                return True
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                return True
            else:
                import Part

                if isinstance(getattr(obj.Base, "Shape", None), Part.Shape):
                    return True
                else:
                    t = translate("Arch", "Wrong base type")
                    FreeCAD.Console.PrintError(obj.Label + ": " + t + "\n")
                    return False

    def _isInternalLinkgroup(self, obj):
        """Returns True if obj is an internal LinkGroup. Such a group is used to
        store hidden objects used for variant Links that should not be hosted."""

        # based on code by bdm
        # https://forum.freecad.org/viewtopic.php?p=820487#p820428
        if obj.TypeId != "App::LinkGroup":
            return False
        for inObj in obj.InList:
            if getattr(inObj, "LinkCopyOnChangeGroup", None) is obj:
                return True
        return False

    def _objectInInternalLinkgroup(self, obj):
        """Returns True if obj is a hidden object in an internal LinkGroup."""

        for inObj in obj.InList:
            if self._isInternalLinkgroup(inObj):
                return True
        return False


class AreaCalculator:
    """Helper class to compute vertical area, horizontal area, and perimeter length.

    This class encapsulates the logic for calculating the following properties:
    - **VerticalArea**: The total area of all vertical faces of the object. See the
      `isFaceVertical` method for the criteria used to determine vertical faces.
    - **HorizontalArea**: The area of the object's projection onto the XY plane.
    - **PerimeterLength**: The perimeter of the horizontal area.

    The class provides methods to validate the object's shape, identify vertical and
    horizontal faces, and compute the required properties.
    """

    def __init__(self, obj):
        self.obj = obj

    def isShapeInvalid(self):
        """Check if the object's shape is invalid."""
        return (
            not self.obj.Shape
            or self.obj.Shape.isNull()
            or not self.obj.Shape.isValid()
            or not self.obj.Shape.Faces
        )

    def tooManyFaces(self):
        """Check if the object's shape has too many faces to process."""
        return len(self.obj.Shape.Faces) > params.get_param_arch("MaxComputeAreas")

    def resetAreas(self):
        """Reset the area properties of the object to zero. Generally called when
        there is an error.
        """
        for prop in ["VerticalArea", "HorizontalArea", "PerimeterLength"]:
            setattr(self.obj, prop, 0)

    def isFaceVertical(self, face, face_index=None):
        """Determine if a face is vertical.

        A face is considered vertical if:
        - Its normal vector forms an angle close to 90 degrees with the Z-axis.
        - The projected face has an area of zero.

        Parameters
        ----------
        face : Part.Face
            The face object to be checked.
        face_index : str, optional
            The face's 1-based index identifier, used for debugging error messages.
            Defaults to None.

        Notes
        -----
        The check whether the projected face has an area of zero means that roof-like
        (sloped) and domed faces alike will not be counted as vertical faces.
        Vertically-extruded curved edges (for instance from a slab) will be classified
        as vertical and be counted. This is an improvement over the fix for
        https://github.com/FreeCAD/FreeCAD/issues/14687.
        """
        import Part
        import DraftGeomUtils
        import TechDraw

        face_name = f" Face{face_index}" if face_index is not None else ""

        if face.Surface.TypeId == "Part::GeomCylinder":
            angle = face.Surface.Axis.getAngle(FreeCAD.Vector(0, 0, 1))
            return self.isZeroAngle(angle)
        elif face.Surface.TypeId == "Part::GeomSurfaceOfExtrusion":
            angle = face.Surface.Direction.getAngle(FreeCAD.Vector(0, 0, 1))
            return self.isZeroAngle(angle)
        elif face.Surface.TypeId == "Part::GeomPlane":
            projectedArea = 0  # dummy value, isRightAngle check is sufficient here
        elif face.findPlane() is not None:
            projectedArea = 0  # dummy value, idem
        else:
            try:
                edges = TechDraw.project(face, FreeCAD.Vector(0, 0, 1))[0].Edges
                wires = DraftGeomUtils.findWires(edges)
                if len(wires) == 1 and not wires[0].isClosed():
                    projectedArea = 0
                else:
                    projectedArea = Part.Face(wires).Area
            except Part.OCCError:
                FreeCAD.Console.PrintWarning(
                    translate("Arch", f"Could not project face{face_name} from {self.obj.Label}\n")
                )
                return False

        try:
            angle = face.normalAt(0, 0).getAngle(FreeCAD.Vector(0, 0, 1))
            return self.isRightAngle(angle) and projectedArea < 0.0001
        except Part.OCCError:
            FreeCAD.Console.PrintWarning(
                translate(
                    "Arch",
                    f"Could not determine if face{face_name} from {self.obj.Label}"
                    " is vertical: normalAt() failed\n",
                )
            )
            return False

    def isRightAngle(self, angle):
        """Check if the angle is close to 90 degrees."""
        return math.isclose(angle, math.pi / 2, abs_tol=0.0005)

    def isZeroAngle(self, angle):
        """Check if the angle is close to 0 or 180 degrees."""
        if math.isclose(angle, 0, abs_tol=0.0005):
            return True
        return math.isclose(angle, math.pi, abs_tol=0.0005)

    def compute(self):
        """Compute the vertical area, horizontal area, and perimeter length.

        This method performs the following steps:
        1. Identifies the object's vertical and horizontal faces.
        2. Computes the total vertical area by adding areas of all vertical faces.
        3. Projects horizontal faces onto the XY plane and computes their total horizontal area.
        4. Computes the perimeter length of the horizontal area.

        The computed values are assigned to the object's properties:
        - VerticalArea
        - HorizontalArea
        - PerimeterLength
        """
        if self.isShapeInvalid() or self.tooManyFaces():
            self.resetAreas()
            return

        verticalArea = 0
        horizontalAreaFaces = []

        # Compute vertical area and collect faces to be projected for the horizontal area
        for i, face in enumerate(self.obj.Shape.Faces, start=1):
            if self.isFaceVertical(face, face_index=i):
                verticalArea += face.Area
            else:
                horizontalAreaFaces.append(face)

        # Update vertical area
        if hasattr(self.obj, "VerticalArea") and self.obj.VerticalArea.Value != verticalArea:
            self.obj.VerticalArea = verticalArea

        # Compute horizontal area and perimeter length
        if horizontalAreaFaces and hasattr(self.obj, "HorizontalArea"):
            self._computeHorizontalAreaAndPerimeter(horizontalAreaFaces)

    def _computeHorizontalAreaAndPerimeter(self, horizontalAreaFaces):
        """Compute the horizontal area and perimeter length.

        Projects the given faces onto the XY plane, fuses them, and calculates:
        - The total horizontal area.
        - The perimeter length of the fused horizontal area.

        Parameters
        ----------
        horizontalAreaFaces: list of Part.Face
            The faces to process.
        """

        import Part
        import TechDraw
        import DraftGeomUtils

        # In TechDraw edges longer than 9999.9 (ca. 10m) are considered 'crazy'.
        # See also Draft/draftobjects/hatch.py.
        param_grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/TechDraw/debug")
        if "allowCrazyEdge" not in param_grp.GetBools():
            old_allow_crazy_edge = None
        else:
            old_allow_crazy_edge = param_grp.GetBool("allowCrazyEdge")
        param_grp.SetBool("allowCrazyEdge", True)

        direction = FreeCAD.Vector(0, 0, 1)
        projectedFaces = []
        for face in horizontalAreaFaces:
            try:
                if face.findPlane() is None:
                    if len(face.Wires) > 1:
                        # Non-planar faces with holes are not handled properly
                        FreeCAD.Console.PrintWarning(
                            translate(
                                "Arch",
                                f"Error computing areas for {self.obj.Label}: unable to project "
                                "non-planar faces with holes. Area values will be reset to 0.\n",
                            )
                        )
                        self.resetAreas()
                        return
                    wire = TechDraw.findShapeOutline(face, 1, direction)
                    projectedFace = Part.makeFace([wire], "Part::FaceMakerSimple")
                else:
                    edges = TechDraw.project(face, direction)[0].Edges
                    wires = DraftGeomUtils.findWires(edges)
                    # Using "Part::FaceMakerCheese" as the face can have holes
                    projectedFace = Part.makeFace(wires, "Part::FaceMakerCheese")
                # Part.show(projectedFace)
                projectedFaces.append(projectedFace)
            except Part.OCCError:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        f"Error computing areas for {self.obj.Label}: unable to project or "
                        f"make face with normal {face.normalAt(0, 0)}. "
                        "Area values will be reset to 0.\n",
                    )
                )
                self.resetAreas()
                return

        if old_allow_crazy_edge is None:
            param_grp.RemBool("allowCrazyEdge")
        else:
            param_grp.SetBool("allowCrazyEdge", old_allow_crazy_edge)

        if projectedFaces:
            fusedFace = projectedFaces.pop()
            for face in projectedFaces:
                fusedFace = fusedFace.fuse(face)
            fusedFace = fusedFace.removeSplitter()
            # Part.show(fusedFace)

            if self.obj.HorizontalArea.Value != fusedFace.Area:
                self.obj.HorizontalArea = fusedFace.Area

            if hasattr(self.obj, "PerimeterLength") and len(fusedFace.Faces) == 1:
                perimeterLength = fusedFace.Faces[0].OuterWire.Length
                if self.obj.PerimeterLength.Value != perimeterLength:
                    self.obj.PerimeterLength = perimeterLength


class ViewProviderComponent:
    """A default View Provider for Component objects.

    Acts as a base for all other Arch view providers. It's properties and
    behaviours are common to all Arch view providers.

    Parameters
    ----------
    vobj: <Gui.ViewProviderDocumentObject>
        The view provider to turn into a component view provider.
    """

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object
        self.setProperties(vobj)

    def setProperties(self, vobj):
        """Give the component view provider its component view provider specific properties.

        You can learn more about properties here:
        https://wiki.freecad.org/property
        """

        if not "UseMaterialColor" in vobj.PropertiesList:
            vobj.addProperty(
                "App::PropertyBool",
                "UseMaterialColor",
                "Component",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Use the material color as this object's shape color, if available",
                ),
                locked=True,
            )
            vobj.UseMaterialColor = params.get_param_arch("UseMaterialColor")

    def updateData(self, obj, prop):
        """Method called when the host object has a property changed.

        If the object has a Material associated with it, match the view
        object's ShapeColor and Transparency to match the Material.

        If the object is now cloned, or is part of a compound, have the view
        object inherit the DiffuseColor.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The host object that has changed.
        prop: string
            The name of the property that has changed.
        """

        # print(obj.Name," : updating ",prop)
        if prop == "Material":
            if obj.Material and getattr(obj.ViewObject, "UseMaterialColor", True):
                if hasattr(obj.Material, "Material"):
                    if "DiffuseColor" in obj.Material.Material:
                        c = tuple(
                            [
                                float(f)
                                for f in obj.Material.Material["DiffuseColor"]
                                .strip("()")
                                .strip("[]")
                                .split(",")
                            ]
                        )
                        if obj.ViewObject.ShapeColor != c:
                            obj.ViewObject.ShapeColor = c
                        # Overwrite DiffuseColor (required if it does not match number of faces):
                        if obj.ViewObject.DiffuseColor != [c]:
                            obj.ViewObject.DiffuseColor = [c]
                    if "Transparency" in obj.Material.Material:
                        t = int(obj.Material.Material["Transparency"])
                        if obj.ViewObject.Transparency != t:
                            obj.ViewObject.Transparency = t
        elif prop == "Shape":
            if obj.Base:
                if obj.Base.isDerivedFrom("Part::Compound"):
                    if obj.ViewObject.DiffuseColor != obj.Base.ViewObject.DiffuseColor:
                        if len(obj.Base.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.Base.ViewObject.DiffuseColor
                            obj.ViewObject.update()
        elif prop == "CloneOf":
            if obj.CloneOf:
                if (not getattr(obj, "Material", None)) and hasattr(
                    obj.CloneOf.ViewObject, "DiffuseColor"
                ):
                    if obj.ViewObject.DiffuseColor != obj.CloneOf.ViewObject.DiffuseColor:
                        if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
                            obj.ViewObject.update()
        return

    def getIcon(self):
        """Return the path to the appropriate icon.

        If a clone, return the cloned component icon path. Otherwise return the
        Arch Component icon.

        Returns
        -------
        str
            Path to the appropriate icon .svg file.
        """

        import Arch_rc

        if hasattr(self, "Object"):
            if hasattr(self.Object, "CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Component_Clone.svg"
        return ":/icons/Arch_Component_Tree.svg"

    def onChanged(self, vobj, prop):
        """Method called when the view provider has a property changed.

        If DiffuseColor changes, change DiffuseColor to copy the host object's
        clone, if it exists.

        If ShapeColor changes, overwrite it with DiffuseColor.

        If Visibility changes, propagate the change to all view objects that
        are also hosted by this view object's host.

        Parameters
        ----------
        vobj: <Gui.ViewProviderDocumentObject>
            The component's view provider object.
        prop: string
            The name of the property that has changed.
        """

        obj = vobj.Object
        if prop == "DiffuseColor":
            if hasattr(obj, "CloneOf"):
                if obj.CloneOf and hasattr(obj.CloneOf, "DiffuseColor"):
                    if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                        if vobj.DiffuseColor != obj.CloneOf.ViewObject.DiffuseColor:
                            vobj.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
                            vobj.update()
        elif prop == "ShapeColor":
            # restore DiffuseColor after overridden by ShapeColor
            if hasattr(vobj, "DiffuseColor"):
                if len(vobj.DiffuseColor) > 1:
                    d = vobj.DiffuseColor
                    vobj.DiffuseColor = d
        elif prop == "Visibility":
            # do nothing if object is an addition
            if not [parent for parent in obj.InList if obj in getattr(parent, "Additions", [])]:
                hostedObjs = obj.Proxy.getHosts(obj)
                # add objects hosted by additions
                for addition in getattr(obj, "Additions", []):
                    if hasattr(addition, "Proxy") and hasattr(addition.Proxy, "getHosts"):
                        hostedObjs.extend(addition.Proxy.getHosts(addition))
                for hostedObj in hostedObjs:
                    if hasattr(hostedObj, "ViewObject"):
                        hostedObj.ViewObject.Visibility = vobj.Visibility
        return

    def attach(self, vobj):
        """Add display modes' data to the coin scenegraph.

        Add each display mode as a coin node, whose parent is this view
        provider.

        Each display mode's node includes the data needed to display the object
        in that mode. This might include colors of faces, or the draw style of
        lines. This data is stored as additional coin nodes which are children
        of the display mode node.

        Add the HiRes display mode.

        Parameters
        ----------
        vobj: <Gui.ViewProviderDocumentObject>
            The component's view provider object.
        """

        from pivy import coin

        self.Object = vobj.Object
        self.hiresgroup = coin.SoSeparator()
        self.meshcolor = coin.SoBaseColor()
        self.hiresgroup.addChild(self.meshcolor)
        self.hiresgroup.setName("HiRes")
        vobj.addDisplayMode(self.hiresgroup, "HiRes")
        return

    def getDisplayModes(self, vobj):
        """Define the display modes unique to the Arch Component.

        Define mode HiRes, which displays the component as a mesh, intended as
        a more visually appealing version of the component.

        Parameters
        ----------
        vobj: <Gui.ViewProviderDocumentObject>
            The component's view provider object.

        Returns
        -------
        list of str
            List containing the names of the new display modes.
        """

        modes = ["HiRes"]
        return modes

    def setDisplayMode(self, mode):
        """Method called when the display mode changes.

        Called when the display mode changes, this method can be used to set
        data that wasn't available when .attach() was called.

        When HiRes is set as display mode, display the component as a copy of
        the mesh associated as the HiRes property of the host object. See
        ArchComponent.Component's properties.

        If no shape is set in the HiRes property, just display the object as
        the Flat Lines display mode.

        Parameters
        ----------
        vobj: <Gui.ViewProviderDocumentObject>
            The component's view provider object.
        mode: str
            The name of the display mode the view provider has switched to.

        Returns
        -------
        str:
            The name of the display mode the view provider has switched to.
        """

        if hasattr(self, "meshnode"):
            if self.meshnode:
                self.hiresgroup.removeChild(self.meshnode)
                del self.meshnode
        if mode == "HiRes":
            from pivy import coin

            m = None
            if hasattr(self, "Object"):
                if hasattr(self.Object, "HiRes"):
                    if self.Object.HiRes:
                        # if the file was recently loaded, the node is not present yet
                        self.Object.HiRes.ViewObject.show()
                        self.Object.HiRes.ViewObject.hide()
                        m = self.Object.HiRes.ViewObject.RootNode
                if not m:
                    if hasattr(self.Object, "CloneOf"):
                        if self.Object.CloneOf:
                            if hasattr(self.Object.CloneOf, "HiRes"):
                                if self.Object.CloneOf.HiRes:
                                    # if the file was recently loaded, the node is not present yet
                                    self.Object.CloneOf.HiRes.ViewObject.show()
                                    self.Object.CloneOf.HiRes.ViewObject.hide()
                                    m = self.Object.CloneOf.HiRes.ViewObject.RootNode
            if m:
                self.meshnode = m.copy()
                for c in self.meshnode.getChildren():
                    # switch the first found SoSwitch on
                    if isinstance(c, coin.SoSwitch):
                        num = 0
                        if c.getNumChildren() > 0:
                            if c.getChild(0).getName() == "HiRes":
                                num = 1
                        # print "getting node ",num," for ",self.Object.Label
                        c.whichChild = num
                        break
                self.hiresgroup.addChild(self.meshnode)
            else:
                return "Flat Lines"
        return mode

    def dumps(self):

        return None

    def loads(self, state):

        return None

    def claimChildren(self):
        """Define which objects will appear as children in the tree view.

        Set the host object's Base object as a child, and set any additions or
        subtractions as children.

        Parameters
        ----------
        vobj: <Gui.ViewProviderDocumentObject>
            The component's view provider object.

        Returns
        -------
        list of <App::DocumentObject>s:
            The objects claimed as children.
        """

        if hasattr(self, "Object"):
            c = []
            if hasattr(self.Object, "Base"):
                if not (
                    Draft.getType(self.Object) == "Wall"
                    and Draft.getType(self.Object.Base) == "Space"
                ):
                    c = [self.Object.Base]
            if hasattr(self.Object, "Additions"):
                c.extend(self.Object.Additions)
            if hasattr(self.Object, "Subtractions"):
                for s in self.Object.Subtractions:
                    if Draft.getType(self.Object) == "Wall":
                        if Draft.getType(s) == "Roof":
                            continue
                    c.append(s)

            for link in ["Armatures", "Group"]:
                if hasattr(self.Object, link):
                    objlink = getattr(self.Object, link)
                    c.extend(objlink)
            for link in ["Tool", "Subvolume", "HiRes"]:
                if hasattr(self.Object, link):
                    objlink = getattr(self.Object, link)
                    if objlink:
                        c.append(objlink)
            if params.get_param_arch("ClaimHosted"):
                for link in self.Object.Proxy.getHosts(self.Object):
                    c.append(link)

            return c
        return []

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = ComponentTaskPanel()
        taskd.obj = self.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        FreeCADGui.Control.closeDialog()
        return True

    def setupContextMenu(self, vobj, menu):
        """Add the component specific options to the context menu.

        The context menu is the drop down menu that opens when the user right
        clicks on the component in the tree view.

        Parameters
        ----------
        vobj: <Gui.ViewProviderDocumentObject>
            The component's view provider object.
        menu: <PySide2.QtWidgets.QMenu>
            The context menu already assembled prior to this method being
            called.
        """
        if FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
            return
        self.contextMenuAddEdit(menu)
        self.contextMenuAddToggleSubcomponents(menu)

    def contextMenuAddEdit(self, menu):
        actionEdit = QtGui.QAction(translate("Arch", "Edit"), menu)
        QtCore.QObject.connect(actionEdit, QtCore.SIGNAL("triggered()"), self.edit)
        menu.addAction(actionEdit)

    def contextMenuAddToggleSubcomponents(self, menu):
        actionToggleSubcomponents = QtGui.QAction(
            QtGui.QIcon(":/icons/Arch_ToggleSubs.svg"),
            translate("Arch", "Toggle Subcomponents"),
            menu,
        )
        QtCore.QObject.connect(
            actionToggleSubcomponents, QtCore.SIGNAL("triggered()"), self.toggleSubcomponents
        )
        menu.addAction(actionToggleSubcomponents)

    def edit(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)

    def toggleSubcomponents(self):
        FreeCADGui.runCommand("Arch_ToggleSubs")

    def areDifferentColors(self, a, b):
        """Check if two diffuse colors are almost the same.

        Parameters
        ----------
        a: tuple
            The first DiffuseColor value to compare.
        a: tuple
            The second DiffuseColor value to compare.

        Returns
        -------
        bool:
            True if colors are different, false if they are similar.
        """

        if len(a) != len(b):
            return True
        for i in range(len(a)):
            if abs(sum(a[i]) - sum(b[i])) > 0.00001:
                return True
        return False

    def colorize(self, obj, force=False):
        """If an object is a clone, set it to copy the color of its parent.

        Only change the color of the clone if the clone and its parent have
        colors that are distinguishably different from each other.

        Parameters
        ----------
        obj: <Part::Feature>
            The object to change the color of.
        force: bool
            If true, forces the colourisation even if the two objects have very
            similar colors.
        """

        if obj.CloneOf:
            if (
                self.areDifferentColors(
                    obj.ViewObject.DiffuseColor, obj.CloneOf.ViewObject.DiffuseColor
                )
                or force
            ):

                obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor


class ArchSelectionObserver:
    """Selection observer used throughout the Arch module.

    When a nextCommand is specified, the observer fires a Gui command when
    anything is selected.

    When a watched object is specified, the observer will only fire when this
    watched object is selected.

    TODO: This could probably use a rework. Most of the functionality isn't
    used. It does not work correctly to reset the appearance of parent object
    in ComponentTaskPanel.editObject(), for example.

    Parameters
    ----------
    watched: <App::DocumentObject>, optional
        If no watched value is provided, functionality relating to origin
        and hide parameters will not occur. Only the nextCommand will fire.

        When a watched value is provided, the selection observer will only
        fire when the watched object has been selected.
    hide: bool
        Sets if the watched object should be hidden.
    origin: <App::DocumentObject, optional
        If provided, and hide is True, will make the origin object
        selectable, and opaque (set transparency to 0).
    nextCommand: str
        Name of Gui command to run when the watched object is selected, (if
        one is specified), or when anything is selected (if no watched
        object is specified).
    """

    def __init__(self, origin=None, watched=None, hide=True, nextCommand=None):
        self.origin = origin
        self.watched = watched
        self.hide = hide
        self.nextCommand = nextCommand

    def addSelection(self, document, object, element, position):
        """Method called when a selection is made on the Gui.

        When a nextCommand is specified, fire a Gui command when anything is
        selected.

        When a watched object is specified, only fire when this watched object
        is selected.

        Parameters
        ----------
        document: str
            The document's Name.
        object: str
            The selected object's Name.
        element: str
            The element on the object that was selected, such as an edge or
            face.
        position:
            The location in XYZ space the selection was made.
        """

        if not self.watched:
            FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
            if self.nextCommand:
                FreeCADGui.runCommand(self.nextCommand)
            del FreeCAD.ArchObserver
        elif object == self.watched.Name:
            if not element:
                FreeCAD.Console.PrintMessage(translate("Arch", "Closing Sketch edit"))
                if self.hide:
                    if self.origin:
                        self.origin.ViewObject.Transparency = 0
                        self.origin.ViewObject.Selectable = True
                    self.watched.ViewObject.hide()
                FreeCADGui.activateWorkbench("BIMWorkbench")
                if hasattr(FreeCAD, "ArchObserver"):
                    FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
                    del FreeCAD.ArchObserver
                if self.nextCommand:
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(self.watched)
                    FreeCADGui.runCommand(self.nextCommand)


class SelectionTaskPanel:
    """A simple TaskPanel to wait for a selection.

    Typically used in conjunction with ArchComponent.ArchSelectionObserver.
    """

    def __init__(self):
        self.baseform = QtGui.QLabel()
        self.baseform.setText(QtGui.QApplication.translate("Arch", "Select a base object", None))

    def getStandardButtons(self):
        """Adds the cancel button."""
        return QtGui.QDialogButtonBox.Cancel

    def reject(self):
        """The method run when the user selects the cancel button."""

        if hasattr(FreeCAD, "ArchObserver"):
            FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
            del FreeCAD.ArchObserver
        return True


class ComponentTaskPanel:
    """The default TaskPanel for all Arch components.

    TODO: outline the purpose of this taskpanel.
    """

    def __init__(self):
        """
        Initializes the task panel. The transaction context is implicitly opened by the C++ layer
        when entering edit mode.
        """
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.

        self.obj = None
        self.attribs = [
            "Base",
            "Additions",
            "Subtractions",
            "Objects",
            "Components",
            "Axes",
            "Fixtures",
            "Group",
            "Hosts",
        ]
        self.baseform = QtGui.QWidget()
        self.baseform.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.baseform)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.title, 0, 0, 1, 2)
        self.form = self.baseform

        # tree
        self.tree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(1)
        self.tree.header().hide()

        # buttons
        self.addButton = QtGui.QPushButton(self.baseform)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        self.addButton.setEnabled(False)

        self.delButton = QtGui.QPushButton(self.baseform)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)
        self.delButton.setEnabled(False)

        self.ifcButton = QtGui.QPushButton(self.baseform)
        self.ifcButton.setObjectName("ifcButton")
        self.ifcButton.setIcon(QtGui.QIcon(":/icons/IFC.svg"))
        self.grid.addWidget(self.ifcButton, 4, 0, 1, 2)
        self.ifcButton.hide()

        self.classButton = QtGui.QPushButton(self.baseform)
        self.classButton.setObjectName("classButton")
        self.grid.addWidget(self.classButton, 5, 0, 1, 2)
        try:
            import BimClassification
        except Exception:
            self.classButton.hide()
        else:
            import os

            # the BIM_Classification command needs to be added before it can be used
            if not "BIM_Classification" in FreeCADGui.listCommands():
                FreeCADGui.activateWorkbench("BIMWorkbench")
            self.classButton.setIcon(
                QtGui.QIcon(
                    os.path.join(
                        os.path.dirname(BimClassification.__file__),
                        "icons",
                        "BIM_Classification.svg",
                    )
                )
            )

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.ifcButton, QtCore.SIGNAL("clicked()"), self.editIfcProperties)
        QtCore.QObject.connect(self.classButton, QtCore.SIGNAL("clicked()"), self.editClass)
        QtCore.QObject.connect(
            self.tree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check
        )
        QtCore.QObject.connect(
            self.tree, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem *,int)"), self.editObject
        )
        self.update()

        self.doc = FreeCAD.ActiveDocument

    def isAllowedAlterSelection(self):
        """Indicate whether this task dialog allows other commands to modify
        the selection while it is open.

        Returns
        -------
        bool
            If alteration of the selection should be allowed.
        """

        return True

    def isAllowedAlterView(self):
        """Indicate whether this task dialog allows other commands to modify
        the 3D view while it is open.

        Returns
        -------
        bool
            If alteration of the 3D view should be allowed.
        """

        return True

    def getStandardButtons(self):
        """Add the standard Ok/Cancel buttons."""

        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def check(self, wid, col):
        """This method is run as the callback when the user selects an item in the tree.

        Enable and disable the add and remove buttons depending on what the
        user has selected.

        If they have selected one of the root attribute folders, disable the
        remove button. If they have separately selected an object in the 3D
        view, enable the add button, allowing the user to add that object to
        the root attribute folder.

        If they have selected one of the items inside a root attribute folder,
        enable the remove button, allowing the user to remove the object from
        that attribute.

        Parameters
        ----------
        wid: <PySide2.QtWidgets.QTreeWidgetItem>
            Qt object the user has selected in the tree widget.
        """

        if not wid.parent():
            self.delButton.setEnabled(False)
            if self.obj:
                sel = FreeCADGui.Selection.getSelection()
                if sel:
                    if not (self.obj in sel):
                        self.addButton.setEnabled(True)
        else:
            self.delButton.setEnabled(True)
            self.addButton.setEnabled(False)

    def getIcon(self, obj):
        """Get the path to the icons, of the items that fill the tree widget.

        Parameters
        ---------
        obj: <App::DocumentObject>
            The object being edited.
        """

        if hasattr(obj.ViewObject, "Proxy"):
            if hasattr(obj.ViewObject.Proxy, "getIcon"):
                return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            return QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        elif hasattr(obj.ViewObject, "Icon"):
            return QtGui.QIcon(obj.ViewObject.Icon)
        return QtGui.QIcon(":/icons/Part_3D_object.svg")

    def update(self):
        """Populate the treewidget with its various items.

        Check if the object being edited has attributes relevant to subobjects.
        IE: Additions, Subtractions, etc.

        Populate the tree with these subobjects, under folders named after the
        attributes they are listed in.

        Finally, run method .retranslateUi().
        """

        self.tree.clear()
        dirIcon = QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        for a in self.attribs:
            setattr(self, "tree" + a, QtGui.QTreeWidgetItem(self.tree))
            c = getattr(self, "tree" + a)
            c.setIcon(0, dirIcon)
            c.ChildIndicatorPolicy = 2
            if self.obj:
                if not hasattr(self.obj, a):
                    c.setHidden(True)
            else:
                c.setHidden(True)
        if self.obj:
            for attrib in self.attribs:
                if hasattr(self.obj, attrib):
                    Oattrib = getattr(self.obj, attrib)
                    Tattrib = getattr(self, "tree" + attrib)
                    if Oattrib:
                        if attrib == "Base":
                            Oattrib = [Oattrib]
                        for o in Oattrib:
                            item = QtGui.QTreeWidgetItem()
                            item.setText(0, o.Label)
                            item.setToolTip(0, o.Name)
                            item.setIcon(0, self.getIcon(o))
                            Tattrib.addChild(item)
                        self.tree.expandItem(Tattrib)
            if hasattr(self.obj, "IfcProperties"):
                if isinstance(self.obj.IfcProperties, dict):
                    self.ifcButton.show()
        self.retranslateUi(self.baseform)

    def addElement(self):
        """This method is run as a callback when the user selects the add button.

        Get the object selected in the 3D view, and get the attribute folder
        selected in the tree widget.

        Add the object selected in the 3D view to the attribute associated with
        the selected folder, by using function addToComponent().
        """

        it = self.tree.currentItem()
        if it:
            mod = None
            for a in self.attribs:
                if it.text(0) == getattr(self, "tree" + a).text(0):
                    mod = a
            for o in FreeCADGui.Selection.getSelection():
                addToComponent(self.obj, o, mod)
            self.obj.recompute()
        self.update()

    def removeElement(self):
        """
        This method is run as a callback when the user selects the remove button.
        It calls a handler on the object's proxy to perform the removal.
        """
        element_selected = self.tree.currentItem()
        if not element_selected:
            return

        element_to_remove = FreeCAD.ActiveDocument.getObject(str(element_selected.toolTip(0)))

        # Call the polymorphic handler on the object's proxy.
        # This is generic and works for any Arch object.
        if hasattr(self.obj.Proxy, "handleComponentRemoval"):
            self.obj.Proxy.handleComponentRemoval(self.obj, element_to_remove)
        else:
            # Fallback for older proxies that might not have the method
            removeFromComponent(self.obj, element_to_remove)

        self.obj.recompute()
        self.update()

    def accept(self):
        """This method runs as a callback when the user selects the ok button.

        Recomputes the document, and leave edit mode.

        The transaction is implicitly committed by the C++ layer during resetEdit.
        """
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        """
        Aborts the edit session. An explicit abort is required to prevent the C++ layer from
        committing changes during resetEdit.
        """
        self.doc.abortTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def editObject(self, wid, col):
        """This method is run when the user double clicks on an item in the tree widget.

        If the item in the tree has a corresponding object in the document,
        enter edit mode for that associated object.

        At the same time, make the object this task panel was opened for
        transparent and unselectable.

        Parameters
        ----------
        wid: <PySide2.QtWidgets.QTreeWidgetItem>
            Qt object the user has selected in the tree widget.
        """

        if wid.parent():
            obj = FreeCAD.ActiveDocument.getObject(str(wid.toolTip(0)))
            if obj:
                self.obj.ViewObject.Transparency = 80
                self.obj.ViewObject.Selectable = False
                obj.ViewObject.show()
                self.accept()
                if obj.isDerivedFrom("Sketcher::SketchObject"):
                    FreeCADGui.activateWorkbench("SketcherWorkbench")
                FreeCAD.ArchObserver = ArchSelectionObserver(self.obj, obj)
                FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
                FreeCADGui.ActiveDocument.setEdit(obj.Name, 0)

    def retranslateUi(self, TaskPanel):
        """Add the text of the task panel, in translated form."""

        self.baseform.setWindowTitle(QtGui.QApplication.translate("Arch", "Component", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Components of This Object", None))
        self.treeBase.setText(0, QtGui.QApplication.translate("Arch", "Base component", None))
        self.treeAdditions.setText(0, QtGui.QApplication.translate("Arch", "Additions", None))
        self.treeSubtractions.setText(0, QtGui.QApplication.translate("Arch", "Subtractions", None))
        self.treeObjects.setText(0, QtGui.QApplication.translate("Arch", "Objects", None))
        self.treeAxes.setText(0, QtGui.QApplication.translate("Arch", "Axes", None))
        self.treeComponents.setText(0, QtGui.QApplication.translate("Arch", "Components", None))
        self.treeFixtures.setText(0, QtGui.QApplication.translate("Arch", "Fixtures", None))
        self.treeGroup.setText(0, QtGui.QApplication.translate("Arch", "Group", None))
        self.treeHosts.setText(0, QtGui.QApplication.translate("Arch", "Hosts", None))
        self.ifcButton.setText(QtGui.QApplication.translate("Arch", "Edit IFC Properties", None))
        self.classButton.setText(QtGui.QApplication.translate("Arch", "Edit Standard Code", None))

    def editIfcProperties(self):
        """Open the IFC editor dialog box.

        This is the method that runs as a callback when the Edit IFC properties
        button is selected by the user.

        Defines the editor's structure, fill it with data, add a callback for
        the buttons and other interactions, and show it.
        """

        if hasattr(self, "ifcEditor"):
            if self.ifcEditor:
                self.ifcEditor.hide()
            del self.ifcEditor
        if not self.obj:
            return
        if not hasattr(self.obj, "IfcProperties"):
            return
        if not isinstance(self.obj.IfcProperties, dict):
            return
        import csv
        import os
        import Arch_rc
        import ArchIFCSchema

        # get presets
        self.ptypes = list(ArchIFCSchema.IfcTypes)
        self.plabels = [
            "".join(map(lambda x: x if x.islower() else " " + x, t[3:]))[1:] for t in self.ptypes
        ]
        self.psetdefs = {}
        psetspath = os.path.join(
            FreeCAD.getResourceDir(), "Mod", "Arch", "Presets", "pset_definitions.csv"
        )
        if os.path.exists(psetspath):
            with open(psetspath, "r") as csvfile:
                reader = csv.reader(csvfile, delimiter=";")
                for row in reader:
                    self.psetdefs[row[0]] = row[1:]
        self.psetkeys = [
            "".join(map(lambda x: x if x.islower() else " " + x, t[5:]))[1:]
            for t in self.psetdefs.keys()
        ]
        self.psetkeys.sort()
        self.ifcEditor = FreeCADGui.PySideUic.loadUi(":/ui/dialogIfcPropertiesRedux.ui")

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.ifcEditor.move(
            mw.frameGeometry().topLeft() + mw.rect().center() - self.ifcEditor.rect().center()
        )
        self.ifcModel = QtGui.QStandardItemModel()
        self.ifcEditor.treeProperties.setModel(self.ifcModel)
        # self.ifcEditor.treeProperties.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.ifcEditor.treeProperties.setUniformRowHeights(True)
        self.ifcEditor.treeProperties.setItemDelegate(
            IfcEditorDelegate(dialog=self, ptypes=self.ptypes, plabels=self.plabels)
        )
        self.ifcModel.setHorizontalHeaderLabels(
            [
                QtGui.QApplication.translate("Arch", "Property", None),
                QtGui.QApplication.translate("Arch", "Type", None),
                QtGui.QApplication.translate("Arch", "Value", None),
            ]
        )

        # set combos
        self.ifcEditor.comboProperty.addItems(
            [QtGui.QApplication.translate("Arch", "Add property", None)] + self.plabels
        )
        self.ifcEditor.comboPset.addItems(
            [
                QtGui.QApplication.translate("Arch", "Add property set", None),
                QtGui.QApplication.translate("Arch", "New...", None),
            ]
            + self.psetkeys
        )

        # set UUID
        if "IfcUID" in self.obj.IfcData:
            self.ifcEditor.labelUUID.setText(self.obj.IfcData["IfcUID"])

        # fill the tree
        psets = {}
        for pname, value in self.obj.IfcProperties.items():
            # properties in IfcProperties dict are stored as "key":"pset;;type;;value" or "key":"type;;value"
            value = value.split(";;")
            if len(value) == 3:
                pset = value[0]
                ptype = value[1]
                pvalue = value[2]
            elif len(value) == 2:
                pset = "Default property set"
                ptype = value[0]
                pvalue = value[1]
            else:
                continue
            plabel = ptype
            if ptype in self.ptypes:
                plabel = self.plabels[self.ptypes.index(ptype)]
            psets.setdefault(pset, []).append([pname, plabel, pvalue])
        for pset, plists in psets.items():
            top = QtGui.QStandardItem(pset)
            top.setDragEnabled(False)
            top.setToolTip("PropertySet")
            self.ifcModel.appendRow([top, QtGui.QStandardItem(), QtGui.QStandardItem()])
            for plist in plists:
                it1 = QtGui.QStandardItem(plist[0])
                it1.setDropEnabled(False)
                it2 = QtGui.QStandardItem(plist[1])
                it2.setDropEnabled(False)
                it3 = QtGui.QStandardItem(plist[2])
                it3.setDropEnabled(False)
                top.appendRow([it1, it2, it3])
            top.sortChildren(0)

        # span top levels
        idx = self.ifcModel.invisibleRootItem().index()
        for i in range(self.ifcModel.rowCount()):
            if self.ifcModel.item(i, 0).hasChildren():
                self.ifcEditor.treeProperties.setFirstColumnSpanned(i, idx, True)
        self.ifcEditor.treeProperties.expandAll()

        # Add callbacks
        QtCore.QObject.connect(
            self.ifcEditor.buttonBox, QtCore.SIGNAL("accepted()"), self.acceptIfcProperties
        )
        QtCore.QObject.connect(
            self.ifcEditor.comboProperty,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.addIfcProperty,
        )
        QtCore.QObject.connect(
            self.ifcEditor.comboPset, QtCore.SIGNAL("currentIndexChanged(int)"), self.addIfcPset
        )
        QtCore.QObject.connect(
            self.ifcEditor.buttonDelete, QtCore.SIGNAL("clicked()"), self.removeIfcProperty
        )
        self.ifcEditor.treeProperties.setSortingEnabled(True)

        # set checkboxes
        if "FlagForceBrep" in self.obj.IfcData:
            self.ifcEditor.checkBrep.setChecked(self.obj.IfcData["FlagForceBrep"] == "True")
        if "FlagParametric" in self.obj.IfcData:
            self.ifcEditor.checkParametric.setChecked(self.obj.IfcData["FlagParametric"] == "True")
        self.ifcEditor.show()

    def acceptIfcProperties(self):
        """This method runs as a callback when the user selects the ok button in the IFC editor.

        Scrape through the rows of the IFC editor's items, and compare them to
        the object being edited's .IfcData. If the two are different, change
        the object's .IfcData to match the editor's items.
        """

        if hasattr(self, "ifcEditor") and self.ifcEditor:
            self.ifcEditor.hide()
            ifcdict = {}
            for row in range(self.ifcModel.rowCount()):
                pset = self.ifcModel.item(row, 0).text()
                if self.ifcModel.item(row, 0).hasChildren():
                    for childrow in range(self.ifcModel.item(row, 0).rowCount()):
                        prop = self.ifcModel.item(row, 0).child(childrow, 0).text()
                        ptype = self.ifcModel.item(row, 0).child(childrow, 1).text()
                        if not ptype.startswith("Ifc"):
                            ptype = self.ptypes[self.plabels.index(ptype)]
                        pvalue = self.ifcModel.item(row, 0).child(childrow, 2).text()
                        ifcdict[prop] = pset + ";;" + ptype + ";;" + pvalue
            ifcData = self.obj.IfcData
            ifcData["IfcUID"] = self.ifcEditor.labelUUID.text()
            ifcData["FlagForceBrep"] = str(self.ifcEditor.checkBrep.isChecked())
            ifcData["FlagParametric"] = str(self.ifcEditor.checkParametric.isChecked())
            if (ifcdict != self.obj.IfcProperties) or (ifcData != self.obj.IfcData):
                FreeCAD.ActiveDocument.openTransaction("Change Ifc Properties")
                if ifcdict != self.obj.IfcProperties:
                    self.obj.IfcProperties = ifcdict
                if ifcData != self.obj.IfcData:
                    self.obj.IfcData = ifcData
                FreeCAD.ActiveDocument.commitTransaction()
            del self.ifcEditor

    def addIfcProperty(self, idx=0, pset=None, prop=None, ptype=None):
        """Add an IFC property to the object, within the IFC editor.

        This method runs as a callback when the user selects an option in the
        Add Property dropdown box in the IFC editor. When used via the
        dropdown, it adds a property with the property type selected in the
        dropdown.

        This method can also be run standalone, outside its function as a
        callback.

        Unless otherwise specified, the property will be called "New property".
        The property will have no value assigned.

        Parameters
        ----------
        idx: int, optional
            The index number of the property type selected in the dropdown. If
            idx is not specified, the property's type must be specified in the
            ptype parameter.
        pset: str, optional
            The name of the property set this property will be assigned to. If
            not provided, the pset will be determined by which property set has
            been selected within the IFC editor widget.
        prop: str, optional
            The name of the property to be created. If left blank, will be set to
            "New Property".
        ptype: str, optional
            The name of the property type the new property will be set as. If
            not specified, the property's type will be determined using the
            idx parameter.
        """

        if hasattr(self, "ifcEditor") and self.ifcEditor:
            if not pset:
                sel = self.ifcEditor.treeProperties.selectedIndexes()
                if sel:
                    item = self.ifcModel.itemFromIndex(sel[0])
                    if item.toolTip() == "PropertySet":
                        pset = item
            if pset:
                if not prop:
                    prop = QtGui.QApplication.translate("Arch", "New property", None)
                if not ptype:
                    if idx > 0:
                        ptype = self.plabels[idx - 1]
                if prop and ptype:
                    if ptype in self.ptypes:
                        ptype = self.plabels[self.ptypes.index(ptype)]
                    it1 = QtGui.QStandardItem(prop)
                    it1.setDropEnabled(False)
                    it2 = QtGui.QStandardItem(ptype)
                    it2.setDropEnabled(False)
                    it3 = QtGui.QStandardItem()
                    it3.setDropEnabled(False)
                    pset.appendRow([it1, it2, it3])
            if idx != 0:
                self.ifcEditor.comboProperty.setCurrentIndex(0)

    def addIfcPset(self, idx=0):
        """Add an IFC property set to the object, within the IFC editor.

        This method runs as a callback when the user selects a property set
        within the Add property set dropdown.

        Add the property set selected in the dropdown, then check the property
        set definitions for the property set's standard properties, and add
        them with blank values to the new property set.

        Parameters
        ----------
        idx: int
            The index of the options selected from the Add property set
            dropdown.
        """

        if hasattr(self, "ifcEditor") and self.ifcEditor:
            if idx == 1:
                top = QtGui.QStandardItem(
                    QtGui.QApplication.translate("Arch", "New property set", None)
                )
                top.setDragEnabled(False)
                top.setToolTip("PropertySet")
                self.ifcModel.appendRow([top, QtGui.QStandardItem(), QtGui.QStandardItem()])
            elif idx > 1:
                psetlabel = self.psetkeys[idx - 2]
                psetdef = "Pset_" + psetlabel.replace(" ", "")
                if psetdef in self.psetdefs:
                    top = QtGui.QStandardItem(psetdef)
                    top.setDragEnabled(False)
                    top.setToolTip("PropertySet")
                    self.ifcModel.appendRow([top, QtGui.QStandardItem(), QtGui.QStandardItem()])
                    for i in range(0, len(self.psetdefs[psetdef]), 2):
                        self.addIfcProperty(
                            pset=top,
                            prop=self.psetdefs[psetdef][i],
                            ptype=self.psetdefs[psetdef][i + 1],
                        )
            if idx != 0:
                # span top levels
                idx = self.ifcModel.invisibleRootItem().index()
                for i in range(self.ifcModel.rowCount()):
                    if self.ifcModel.item(i, 0).hasChildren():
                        self.ifcEditor.treeProperties.setFirstColumnSpanned(i, idx, True)
                self.ifcEditor.treeProperties.expandAll()
                self.ifcEditor.comboPset.setCurrentIndex(0)

    def removeIfcProperty(self):
        """Remove an IFC property or property set from the object, within the IFC editor.

        This method runs as a callback when the user selects the Delete
        selected property/set button within the IFC editor.

        Find the selected property, and delete it. If it's a property set,
        delete the children properties as well.
        """

        if hasattr(self, "ifcEditor") and self.ifcEditor:
            sel = self.ifcEditor.treeProperties.selectedIndexes()
            if sel:
                if self.ifcModel.itemFromIndex(sel[0]).toolTip() == "PropertySet":
                    self.ifcModel.takeRow(sel[0].row())
                else:
                    pset = self.ifcModel.itemFromIndex(sel[0].parent())
                    pset.takeRow(sel[0].row())

    def editClass(self):
        """Simple wrapper for BIM_Classification Gui command.

        This method is called when the Edit class button is selected
        in the IFC editor. It relies on the presence of the BIM module.
        """

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.obj)
        FreeCADGui.runCommand("BIM_Classification")


if FreeCAD.GuiUp:

    class IfcEditorDelegate(QtGui.QStyledItemDelegate):
        """This class manages the editing of the individual table cells in the IFC editor.

        Parameters
        ----------
        parent: <PySide2.QtWidgets.QWidget>
            Unclear.
        dialog: <ArchComponent.ComponentTaskPanel>
            The dialog box this delegate was created in.
        ptypes: list of str
            A list of the names of IFC property types.
        plables: list of str
            A list of the human readable names of IFC property types.
        """

        def __init__(self, parent=None, dialog=None, ptypes=[], plabels=[], *args):
            self.dialog = dialog
            QtGui.QStyledItemDelegate.__init__(self, parent, *args)
            self.ptypes = ptypes
            self.plabels = plabels

        def createEditor(self, parent, option, index):
            """Return the widget used to change data.

            Return a text line editor if editing the property name.  Return a
            dropdown to change property type if editing property type.  If
            editing the property's value, return an appropriate widget
            depending on the datatype of the value.

            Parameters
            ----------
            parent: <pyside2.qtwidgets.qwidget>
                The table cell that is being edited.
            option:
                Unused?
            index: <PySide2.QtCore.QModelIndex>
                The index object of the table of the IFC editor.

            Returns
            -------
            <pyside2.qtwidgets.qwidget>
                The editor widget this method has created.
            """

            if index.column() == 0:  # property name
                editor = QtGui.QLineEdit(parent)
            elif index.column() == 1:  # property type
                editor = QtGui.QComboBox(parent)
            else:  # property value
                ptype = index.sibling(index.row(), 1).data()
                if "Integer" in ptype:
                    editor = QtGui.QSpinBox(parent)
                elif "Real" in ptype:
                    editor = QtGui.QDoubleSpinBox(parent)
                    editor.setDecimals(params.get_param("Decimals", path="Units"))
                elif ("Boolean" in ptype) or ("Logical" in ptype):
                    editor = QtGui.QComboBox(parent)
                    editor.addItems(["True", "False"])
                elif "Measure" in ptype:
                    editor = FreeCADGui.UiLoader().createWidget("Gui::InputField")
                    editor.setParent(parent)
                else:
                    editor = QtGui.QLineEdit(parent)
                editor.setObjectName("editor_" + ptype)
            return editor

        def setEditorData(self, editor, index):
            """Give data to the edit widget.

            Extract the data already present in the table, and write it to the
            editor. This means the user starts the editor with their previous
            data already present, instead of a blank slate.

            Parameters
            ----------
            editor: <pyside2.qtwidgets.qwidget>
                The editor widget.
            index: <PySide2.QtCore.QModelIndex>
                The index object of the table, of the IFC editor
            """

            if index.column() == 0:
                editor.setText(index.data())
            elif index.column() == 1:
                editor.addItems(self.plabels)
                if index.data() in self.plabels:
                    idx = self.plabels.index(index.data())
                    editor.setCurrentIndex(idx)
            else:
                if "Integer" in editor.objectName():
                    try:
                        editor.setValue(int(index.data()))
                    except Exception:
                        editor.setValue(0)
                elif "Real" in editor.objectName():
                    try:
                        editor.setValue(float(index.data()))
                    except Exception:
                        editor.setValue(0)
                elif ("Boolean" in editor.objectName()) or ("Logical" in editor.objectName()):
                    try:
                        editor.setCurrentIndex(["true", "false"].index(index.data().lower()))
                    except Exception:
                        editor.setCurrentIndex(1)
                elif "Measure" in editor.objectName():
                    try:
                        editor.setText(index.data())
                    except Exception:
                        editor.setValue(0)
                else:
                    editor.setText(index.data())

        def setModelData(self, editor, model, index):
            """Write the data in the editor to the IFC editor's table.

            Parameters
            ----------
            editor: <pyside2.qtwidgets.qwidget>
                The editor widget.
            model:
                The table object of the IFC editor.
            index: <PySide2.QtCore.QModelIndex>
                The index object of the table, of the IFC editor
            """

            if index.column() == 0:
                model.setData(index, editor.text())
            elif index.column() == 1:
                if editor.currentIndex() > -1:
                    idx = editor.currentIndex()
                    data = self.plabels[idx]
                    model.setData(index, data)
            else:
                if ("Integer" in editor.objectName()) or ("Real" in editor.objectName()):
                    model.setData(index, str(editor.value()))
                elif ("Boolean" in editor.objectName()) or ("Logical" in editor.objectName()):
                    model.setData(index, editor.currentText())
                elif "Measure" in editor.objectName():
                    model.setData(index, editor.property("text"))
                else:
                    model.setData(index, editor.text())
            self.dialog.update()
