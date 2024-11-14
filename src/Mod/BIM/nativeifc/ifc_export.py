# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 3 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Draft
import ifcopenshell

from importers import exportIFC
from importers import exportIFCHelper
from importers import importIFCHelper

from nativeifc import ifc_tools


def get_export_preferences(ifcfile, preferred_context=None, create=None):
    """returns a preferences dict for exportIFC.
    Preferred context can either indicate a ContextType like 'Model' or 'Plan',
    or a [ContextIdentifier,ContextType,TargetView] list or tuple, for ex.
    ('Annotation','Plan') or ('Body','Model','MODEL_VIEW'). This function
    will do its best to find the most appropriate context. If create is True,
    if the exact context is not found, a new one is created"""

    prefs = exportIFC.getPreferences()
    prefs["SCHEMA"] = ifcfile.wrapped_data.schema_name()
    s = ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    # the above lines yields meter -> file unit scale factor. We need mm
    prefs["SCALE_FACTOR"] = 0.001 / s
    cids = ifc_tools.get_body_context_ids(ifcfile)
    contexts = [ifcfile[i] for i in cids]
    best_context = None
    exact_match = False
    if preferred_context:
        if isinstance(preferred_context, str):
            for context in contexts:
                if context.ContextType == preferred_context:
                    best_context = context
                    exact_match = True
                    break
        elif isinstance(preferred_context, (list, tuple)):
            second_choice = None
            for context in contexts:
                if len(preferred_context) > 2:
                    if (context.TargetView == preferred_context[2]
                        and context.ContextType == preferred_context[1]
                        and context.ContextIdentifier == preferred_context[0]):
                            best_context = context
                            exact_match = True
                if len(preferred_context) > 1:
                    if (context.ContextType == preferred_context[1]
                        and context.ContextIdentifier == preferred_context[0]):
                            if not exact_match:
                                best_context = context
                                if len(preferred_context) == 2:
                                    exact_match = True
                if context.ContextType == preferred_context[0]:
                    if not exact_match:
                        best_context = context
                        if len(preferred_context) == 1:
                            exact_match = True
            if contexts:
                if not best_context:
                    best_context = contexts[0]
        if create:
            if not exact_match:
                if isinstance(preferred_context, str):
                    best_context = ifc_tools.api_run("context.add_context",
                                                     ifcfile,
                                                     context_type = preferred_context)
                elif best_context:
                    if len(preferred_context) > 2:
                        best_context = ifc_tools.api_run("context.add_context",
                                                         ifcfile,
                                                         context_type = preferred_context[1],
                                                         context_identifier = preferred_context[0],
                                                         target_view = preferred_context[2],
                                                         parent = best_context)
                    elif len(preferred_context) > 1:
                        best_context = ifc_tools.api_run("context.add_context",
                                                         ifcfile,
                                                         context_type = preferred_context[1],
                                                         context_identifier = preferred_context[0],
                                                         parent = best_context)
                else:
                    if len(preferred_context) > 1:
                        best_context = ifc_tools.api_run("context.add_context",
                                                         ifcfile,
                                                         context_type = preferred_context[1])
                        if len(preferred_context) > 2:
                            best_context = ifc_tools.api_run("context.add_context",
                                                             ifcfile,
                                                             context_type = preferred_context[1],
                                                             context_identifier = preferred_context[0],
                                                             target_view = preferred_context[2],
                                                             parent = best_context)
                        else:
                            best_context = ifc_tools.api_run("context.add_context",
                                                             ifcfile,
                                                             context_type = preferred_context[1],
                                                             context_identifier = preferred_context[0],
                                                             parent = best_context)
                    else:
                        best_context = ifc_tools.api_run("context.add_context",
                                                         ifcfile,
                                                         context_type = preferred_context[0])
    return prefs, best_context


def create_product(obj, parent, ifcfile, ifcclass=None):
    """Creates an IFC product out of a FreeCAD object"""

    name = obj.Label
    description = getattr(obj, "Description", None)
    if not ifcclass:
        ifcclass = ifc_tools.get_ifctype(obj)
    representation, placement = create_representation(obj, ifcfile)
    product = ifc_tools.api_run("root.create_entity", ifcfile, ifc_class=ifcclass, name=name)
    ifc_tools.set_attribute(ifcfile, product, "Description", description)
    ifc_tools.set_attribute(ifcfile, product, "ObjectPlacement", placement)
    # TODO below cannot be used at the moment because the ArchIFC exporter returns an
    # IfcProductDefinitionShape already and not an IfcShapeRepresentation
    # ifc_tools.api_run("geometry.assign_representation", ifcfile, product=product, representation=representation)
    ifc_tools.set_attribute(ifcfile, product, "Representation", representation)
    # TODO treat subtractions/additions
    return product


def create_representation(obj, ifcfile):
    """Creates a geometry representation for the given object"""

    # TEMPORARY use the Arch exporter
    # TODO this is temporary. We should rely on ifcopenshell for this with:
    # https://blenderbim.org/docs-python/autoapi/ifcopenshell/api/root/create_entity/index.html
    # a new FreeCAD 'engine' should be added to:
    # https://blenderbim.org/docs-python/autoapi/ifcopenshell/api/geometry/index.html
    # that should contain all typical use cases one could have to convert FreeCAD geometry
    # to IFC.

    # setup exporter - TODO do that in the module init
    exportIFC.clones = {}
    exportIFC.profiledefs = {}
    exportIFC.surfstyles = {}
    exportIFC.shapedefs = {}
    exportIFC.ifcopenshell = ifcopenshell
    exportIFC.ifcbin = exportIFCHelper.recycler(ifcfile, template=False)
    prefs, context = get_export_preferences(ifcfile)
    representation, placement, shapetype = exportIFC.getRepresentation(
        ifcfile, context, obj, preferences=prefs
    )
    return representation, placement


def get_object_type(ifcentity, objecttype=None):
    """Determines a creation type for this object"""

    if not objecttype:
        if ifcentity.is_a("IfcAnnotation"):
            if get_sectionplane(ifcentity):
                objecttype = "sectionplane"
            elif get_dimension(ifcentity):
                objecttype = "dimension"
            elif get_text(ifcentity):
                objecttype = "text"
        elif ifcentity.is_a("IfcGridAxis"):
            objecttype = "axis"
    return objecttype


def is_annotation(obj):
    """Determines if the given FreeCAD object should be saved as an IfcAnnotation"""

    if getattr(obj, "IfcClass", None) in ["IfcAnnotation", "IfcGridAxis"]:
        return True
    if getattr(obj, "IfcType", None) == "Annotation":
        return True
    if obj.isDerivedFrom("Part::Part2DObject"):
        return True
    elif obj.isDerivedFrom("App::Annotation"):
        return True
    elif Draft.getType(obj) in ["DraftText",
                                "Text",
                                "Dimension",
                                "LinearDimension",
                                "AngularDimension",
                                "SectionPlane"]:
        return True
    elif obj.isDerivedFrom("Part::Feature"):
        if obj.Shape and (not obj.Shape.Solids) and obj.Shape.Edges:
            if not obj.Shape.Faces:
                return True
            elif (obj.Shape.BoundBox.XLength < 0.0001) \
                or (obj.Shape.BoundBox.YLength < 0.0001) \
                or (obj.Shape.BoundBox.ZLength < 0.0001):
                return True
    return False


def get_text(annotation):
    """Determines if an IfcAnnotation contains an IfcTextLiteral.
    Returns the IfcTextLiteral or None"""

    if annotation.is_a("IfcAnnotation"):
        for rep in annotation.Representation.Representations:
            for item in rep.Items:
                if item.is_a("IfcTextLiteral"):
                    return item
    return None


def get_dimension(annotation):
    """Determines if an IfcAnnotation is representing a dimension.
    Returns a list containing the representation, two points indicating
    the mesured points, and optionally a third point indicating where
    the dimension line is located, if available"""

    if annotation.is_a("IfcAnnotation"):
        if annotation.ObjectType == "DIMENSION":
            s = ifcopenshell.util.unit.calculate_unit_scale(annotation.file) * 1000
            for rep in annotation.Representation.Representations:
                shape = importIFCHelper.get2DShape(rep, s, notext=True)
                pl = get_placement(annotation.ObjectPlacement, scale=s)
                if pl:
                    shape[0].Placement = pl
                if shape and len(shape) == 1:
                    if len(shape[0].Vertexes) >= 2:
                        # two-point polyline (BBIM)
                        res = [rep, shape[0].Vertexes[0].Point, shape[0].Vertexes[-1].Point]
                        if len(shape[0].Vertexes) > 2:
                            # 4-point polyline (FreeCAD)
                            res.append(shape[0].Vertexes[1].Point)
                        return res
                else:
                    print(annotation,"NOT A DIMENSION")
    return None


def get_sectionplane(annotation):
    """Determines if an IfcAnnotation is representing a section plane.
    Returns a list containing a placement, and optionally an X dimension,
    an Y dimension and a  depth dimension"""

    if annotation.is_a("IfcAnnotation"):
        if annotation.ObjectType == "DRAWING":
            s = ifcopenshell.util.unit.calculate_unit_scale(annotation.file) * 1000
            result = [get_placement(annotation.ObjectPlacement, scale=s)]
            for rep in annotation.Representation.Representations:
                for item in rep.Items:
                    if item.is_a("IfcCsgSolid"):
                        if item.TreeRootExpression.is_a("IfcBlock"):
                            result.append(item.TreeRootExpression.XLength*s)
                            result.append(item.TreeRootExpression.YLength*s)
                            result.append(item.TreeRootExpression.ZLength*s)
            return result
    return None


def get_axis(obj):
    """Determines if a given IFC entity is an IfcGridAxis. Returns a tuple
    containing a Placement, a length value in millimeters, and a tag"""

    if obj.is_a("IfcGridAxis"):
        tag = obj.AxisTag
        s = ifcopenshell.util.unit.calculate_unit_scale(obj.file) * 1000
        shape = importIFCHelper.get2DShape(obj.AxisCurve, s, notext=True)
        if shape:
            edge = shape[0].Edges[0]  # we suppose here the axis shape is a single straight line
            if obj.SameSense:
                p0 = edge.Vertexes[0].Point
                p1 = edge.Vertexes[-1].Point
            else:
                p0 = edge.Vertexes[-1].Point
                p1 = edge.Vertexes[0].Point
            length = edge.Length
            placement = FreeCAD.Placement()
            placement.Base = p0
            placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0,1,0), p1.sub(p0))
            return (placement, length, tag)
    return None


def create_annotation(obj, ifcfile):
    """Adds an IfcAnnotation from the given object to the given IFC file"""

    exportIFC.clones = {}
    exportIFC.profiledefs = {}
    exportIFC.surfstyles = {}
    exportIFC.shapedefs = {}
    exportIFC.curvestyles = {}
    exportIFC.ifcopenshell = ifcopenshell
    exportIFC.ifcbin = exportIFCHelper.recycler(ifcfile, template=False)
    if is_annotation(obj) and Draft.getType(obj) != "SectionPlane":
        context_type = "Plan"
    else:
        context_type = "Model"
    prefs, context = get_export_preferences(ifcfile, preferred_context=context_type, create=True)
    prefs["BBIMDIMS"] = True # Save dimensions as 2-point polylines
    history = get_history(ifcfile)
    # TODO The following prints each edge as a separate IfcGeometricCurveSet
    # It should be refined to create polylines instead
    anno = exportIFC.create_annotation(obj, ifcfile, context, history, prefs)
    return anno


def get_history(ifcfile):
    """Returns the owner history or None"""

    history = ifcfile.by_type("IfcOwnerHistory")
    if history:
        history = history[0]
    else:
        # IFC4 allows to not write any history
        history = None
    return history


def get_placement(ifcelement, ifcfile=None, scale=None):
    """Returns a FreeCAD placement from an IFC placement"""

    if not scale:
        if not ifcfile:
            ifcfile = ifcelement.file
        scale = 0.001 / ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    return importIFCHelper.getPlacement(ifcelement, scaling=scale)


def get_scaled_point(point, ifcfile=None, is2d=False):
    """Returns a scaled 2d or 3d point tuple form a FreeCAD point"""

    if not ifcfile:
        ifcfile = ifcelement.file
    s = 0.001 / ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    v = FreeCAD.Vector(point)
    v.multiply(s)
    v = tuple(v)
    if is2d:
        v = v[:2]
    return v

def get_scaled_value(value, ifcfile):
    """Returns a scaled dimension value"""

    s = 0.001 / ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    return value * s
