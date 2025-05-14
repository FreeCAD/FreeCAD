# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains all the tools to create FreeCAD geometry from IFC objects.
The only entry point in this module is the generate_geometry() function which is
used by the execute() method of ifc_objects"""

import multiprocessing
import re

import ifcopenshell
import ifcopenshell.util.element
from pivy import coin
from PySide import QtCore

import FreeCAD
import FreeCADGui
import Part

from FreeCAD import Base

from . import ifc_tools
from . import ifc_export


def generate_geometry(obj, cached=False):
    """Sets the geometry of the given object from a corresponding IFC element.
    This is the main function called by the execute method of FreeCAD document objects
    It is only meant to be called form there, as it is always the responsibility of the
    NativeIFC document object to know when it needs to regenerate its geometry.

    The generate_geometry will call either generate_shape or generate_coin, depending
    on the type of representation it wants.

    obj: FreeCAD document object
    [cached]: If we should try to use the cached version. The document object knows when
    to use this.
    """

    # setup
    if not obj:
        return
    colors = None

    ifcfile = ifc_tools.get_ifcfile(obj)

    # annotations
    if ifc_export.is_annotation(obj):
        element = ifc_tools.get_ifc_element(obj)
        if not element:
            return
        if obj.ShapeMode == "Shape":
            shape, placement = get_annotation_shape(element, ifcfile)
            if shape:
                obj.Shape = shape
                if placement:
                    obj.Placement = placement
                    return
        elif obj.ViewObject and obj.ShapeMode == "Coin":
            done = False
            node, placement = get_annotation_shape(element, ifcfile, coin=True)
            if node:
                set_representation(obj.ViewObject, node)
                colors = node[0]
                done = True
            else:
                set_representation(obj.ViewObject, None)
                print_debug(obj)
            if placement:
                obj.Placement = placement
            if done:
                return

    # generate the shape or coin node
    elements = get_decomposition(obj)
    if obj.ShapeMode == "Shape":
        shape, colors = generate_shape(ifcfile, elements, cached)
        if shape:
            placement = shape.Placement
            obj.Shape = shape
            obj.Placement = placement
        else:
            obj.Shape = Part.Shape()
            print_debug(obj)
    elif obj.ViewObject and obj.ShapeMode == "Coin":
        node, placement = generate_coin(ifcfile, elements, cached)
        if node:
            # TODO this still needs to be fixed
            #QtCore.QTimer.singleShot(0, lambda: set_representation(obj.ViewObject, node))
            set_representation(obj.ViewObject, node)
            colors = node[0]
        else:
            set_representation(obj.ViewObject, None)
            print_debug(obj)
        if placement:
            obj.Placement = placement

    # set shape and diffuse colors
    if colors:
        QtCore.QTimer.singleShot(0, lambda: ifc_tools.set_colors(obj, colors))  # TODO migrate here?


def generate_shape(ifcfile, elements, cached=False):
    """Returns a Part shape and a list of colors for a list of elements"""

    # setup
    if not elements:
        return None, None
    shapes = []
    colors = []
    cache = get_cache(ifcfile)

    # get cached elements
    if cached:
        rest = []
        for element in elements:
            if element.id in cache["Shape"]:
                shape = cache["Shape"][element.id]
                shapes.append(shape.copy())
                if element.id in cache["Color"]:
                    color = cache["Color"][element.id]
                else:
                    color = (0.8, 0.8, 0.8)
                if len(color) <= 4:
                    for face in shape.Faces:
                        colors.append(color)
                else:
                    colors = color
            else:
                rest.append(element)
        if not rest:
            # all elements have been taken from cache, nothing more to do
            return shapes, colors
        elements = rest

    # prepare the iterator
    iterator = get_geom_iterator(ifcfile, elements, brep_mode=True)
    if iterator is None:
        return None, None
    total = len(elements)
    progressbar = Base.ProgressIndicator()
    progressbar.start("Generating " + str(total) + " shapes...", total)
    done = []

    # iterate
    while True:
        item = iterator.get()
        if item and item.id not in done:
            done.append(item.id)
            # get and transfer brep data
            brep = item.geometry.brep_data
            shape = Part.Shape()
            shape.importBrepFromString(brep, False)
            if hasattr(item.transformation.matrix, "data"):
                # IfcOpenShell 0.7
                mat = ifc_tools.get_freecad_matrix(item.transformation.matrix.data)
            else:
                # IfcOpenShell 0.8
                mat = ifc_tools.get_freecad_matrix(item.transformation.matrix)
            shape.scale(ifc_tools.SCALE)
            shape.transformShape(mat)
            shapes.append(shape)

            # get colors
            sstyle = item.geometry.surface_styles
            scolors = []
            if (
                (len(sstyle) > 4)
                and len(shape.Solids) > 1
                and len(sstyle) // 4 == len(shape.Solids)
            ):
                # multiple colors
                for i in range(len(shape.Solids)):
                    for j in range(len(shape.Solids[i].Faces)):
                        scolors.append(
                            (
                                sstyle[i * 4],
                                sstyle[i * 4 + 1],
                                sstyle[i * 4 + 2],
                                1.0 - sstyle[i * 4 + 3],
                            )
                        )
                if len(colors) < len(shape.Faces):
                    for i in range(len(shape.Faces) - len(colors)):
                        scolors.append(
                            (sstyle[0], sstyle[1], sstyle[2], 1.0 - sstyle[3])
                        )
            else:
                color = (sstyle[0], sstyle[1], sstyle[2], 1.0 - sstyle[3])
                for f in shape.Faces:
                    scolors.append(color)

            # update the cache
            cache["Shape"][item.id] = shape
            cache["Color"][item.id] = scolors
            colors.extend(scolors)
            progressbar.next(True)
        if not iterator.next():
            break

    # write the cache
    set_cache(ifcfile, cache)

    # compound the shape if needed
    if len(shapes) == 1:
        shape = shapes[0]
    else:
        shape = Part.makeCompound(shapes)

    progressbar.stop()
    return shape, colors


def generate_coin(ifcfile, elements, cached=False):
    """Returns coin node data (verts,face and edge index) and a Placement
    from a list of ifc elements"""

    # setup
    # strip out elements without representation, as they can't generate a node anyway
    elements = [e for e in elements if getattr(e, "Representation", None)]
    if not elements:
        return None, None
    # if we have more than one element, placements will need to be applied on subnodes
    grouping = bool(len(elements) > 1)
    nodes = []

    # process cached elements
    placement = None
    cache = get_cache(ifcfile)
    if cached:
        rest = []
        for element in elements:
            if element.id() in cache["Placement"]:
                placement = cache["Placement"][element.id()]
            if element.id() in cache["Coin"]:
                node = cache["Coin"][element.id()]
                if grouping:
                    node = apply_placement(node, placement)
                nodes.append(node)
            else:
                rest.append(element)
        if grouping:
            placement = None
        if not rest:
            # all elements have been taken from cache, nothing more to do
            return unify(nodes), placement
        elements = rest

    # prepare the iterator
    iterator = get_geom_iterator(ifcfile, elements, brep_mode=False)
    if iterator is None:
        return None, None
    total = len(elements)
    progressbar = Base.ProgressIndicator()
    progressbar.start("Generating " + str(total) + " shapes...", total)
    done = []

    # iterate
    while True:
        item = iterator.get()
        if item and item.id not in done:
            done.append(item.id)

            # colors
            if item.geometry.materials:
                color = item.geometry.materials[0].diffuse
                if hasattr(color, "r") and hasattr(color, "g"):
                    # IfcOpenShell 0.8
                    color = (color.r(), color.g(), color.b())
                else:
                    # IfcOpenShell 0.7
                    color = (float(color[0]), float(color[1]), float(color[2]))
                trans = item.geometry.materials[0].transparency
                if trans >= 0:
                    color += (float(trans),)
            else:
                color = (0.85, 0.85, 0.85)

            # verts
            if hasattr(item.transformation.matrix, "data"):
                # IfcOpenShell 0.7
                matrix = ifc_tools.get_freecad_matrix(item.transformation.matrix.data)
            else:
                # IfcOpenShell 0.8
                matrix = ifc_tools.get_freecad_matrix(item.transformation.matrix)
            placement = FreeCAD.Placement(matrix)
            verts = item.geometry.verts
            verts = [FreeCAD.Vector(verts[i : i + 3]) for i in range(0, len(verts), 3)]
            verts = [tuple(v.multiply(ifc_tools.SCALE)) for v in verts]

            # faces
            faces = list(item.geometry.faces)
            faces = [
                f for i in range(0, len(faces), 3) for f in faces[i : i + 3] + [-1]
            ]

            # edges
            edges = list(item.geometry.edges)
            edges = [
                e for i in range(0, len(edges), 2) for e in edges[i : i + 2] + [-1]
            ]

            # update cache
            node = [color, verts, faces, edges]
            cache["Coin"][item.id] = node
            cache["Placement"][item.id] = placement

            if grouping:
                # if we are joining nodes together, their placement
                # must be baked in
                node = apply_placement(node, placement)
            nodes.append(node)
            progressbar.next(True)
        if not iterator.next():
            break

    # unify nodes
    nodes = unify(nodes)

    # nullify placement if already applied
    if grouping:
        placement = None

    # write cache
    set_cache(ifcfile, cache)

    progressbar.stop()
    return nodes, placement


def get_decomposition(obj):
    """Gets the elements we need to render this object"""

    # stime = time.time()
    obj_ids = [c.StepId for c in obj.OutListRecursive if hasattr(c, "StepId")]
    element = ifc_tools.get_ifc_element(obj)
    elements = get_decomposed_elements(element, obj)
    elements = filter_types(elements, obj_ids)
    # print("decomposition:", "%02d:%02d" % (divmod(round(time.time() - stime, 1), 60)))
    return elements


def filter_types(elements, obj_ids=[]):
    """Remove unrenderable (for now) elements from the given list"""

    elements = [e for e in elements if e.is_a("IfcProduct")]
    elements = [e for e in elements if not e.is_a("IfcFeatureElement")]
    elements = [e for e in elements if not e.is_a("IfcOpeningElement")]
    elements = [e for e in elements if not e.is_a("IfcSpace")]
    elements = [e for e in elements if not e.is_a("IfcFurnishingElement")]
    elements = [e for e in elements if not e.id() in obj_ids]
    return elements


def get_decomposed_elements(element, obj=None):
    """Returns a list of renderable elements form a base element"""

    result = []
    if getattr(element, "Representation", None):
        if element not in result:
            result.append(element)
    if not obj or not hasattr(obj, "Group"):
        child_ids = []
    else:
        # add child elements that are not yet rendered
        child_ids = [c.StepId for c in obj.Group if hasattr(c, "StepId")]
    try:
        dec = ifcopenshell.util.element.get_decomposition(element, is_recursive=False)
    except:
        # older version of IfcOpenShell
        dec = ifcopenshell.util.element.get_decomposition(element)
    for child in dec:
        if child.id() not in child_ids:
            if child not in result:
                result.append(child)
            # for el in get_decomposed_elements(child, obj):
            for el in ifcopenshell.util.element.get_decomposition(child):
                if el not in result:
                    result.append(el)
    return result


def get_geom_iterator(ifcfile, elements, brep_mode):
    """Prepares and returns an ifcopenshell iterator instance
    from the given ifcfile and elements list. brep_mode indicates
    if we want brep data or not"""

    settings = ifcopenshell.geom.settings()
    body_contexts = ifc_tools.get_body_context_ids(ifcfile)  # TODO migrate here?
    if brep_mode:
        if hasattr(settings, "DISABLE_TRIANGULATION"):
            # IfcOpenShell 0.7
            settings.set(settings.DISABLE_TRIANGULATION, True)
            settings.set(settings.USE_BREP_DATA, True)
            settings.set(settings.SEW_SHELLS, True)
            if body_contexts:
                settings.set_context_ids(body_contexts)
        elif hasattr(settings, "ITERATOR_OUTPUT"):
            # IfcOpenShell 0.8
            settings.set("ITERATOR_OUTPUT", ifcopenshell.ifcopenshell_wrapper.SERIALIZED)
            if body_contexts:
                # Is this the right way? It works, but not sure.
                settings.set("CONTEXT_IDENTIFIERS", [str(s) for s in body_contexts])
        else:
            # We print a debug message but we continue
            print("DEBUG: ifc_tools.get_geom_iterator: Iterator could not be set up correctly")
    cores = multiprocessing.cpu_count()
    iterator = ifcopenshell.geom.iterator(settings, ifcfile, cores, include=elements)
    if not iterator.initialize():
        print("DEBUG: ifc_tools.get_geom_iterator: Invalid iterator")
        return None
    return iterator


def get_cache(ifcfile):
    """Returns the shape cache dictionary associated with this ifc file"""

    for d in FreeCAD.listDocuments().values():
        if hasattr(d, "Proxy") and hasattr(d.Proxy, "ifcfile"):
            if d.Proxy.ifcfile == ifcfile:
                if hasattr(d.Proxy, "ifccache") and d.Proxy.ifccache:
                    return d.Proxy.ifccache
        for o in d.Objects:
            if hasattr(o, "Proxy") and hasattr(o.Proxy, "ifcfile"):
                if o.Proxy.ifcfile == ifcfile:
                    if hasattr(o.Proxy, "ifccache") and o.Proxy.ifccache:
                        return o.Proxy.ifccache
    # init a new cache
    return {"Shape": {}, "Color": {}, "Coin": {}, "Placement": {}}


def set_cache(ifcfile, cache):
    """Sets the given dictionary as shape cache for the given ifc file"""

    for d in FreeCAD.listDocuments().values():
        if hasattr(d, "Proxy") and hasattr(d.Proxy, "ifcfile"):
            if d.Proxy.ifcfile == ifcfile:
                d.Proxy.ifccache = cache
                return
        for o in d.Objects:
            if hasattr(o, "Proxy") and hasattr(o.Proxy, "ifcfile"):
                if o.Proxy.ifcfile == ifcfile:
                    o.Proxy.ifccache = cache
                    return


def set_representation(vobj, node):
    """Sets the correct coin nodes for the given Part object"""

    def find_node(parent, nodetype):
        for i in range(parent.getNumChildren()):
            if isinstance(parent.getChild(i), nodetype):
                return parent.getChild(i)
        return None

    # node = [colors, verts, faces, edges, parts]
    if not vobj.RootNode:
        return
    if vobj.RootNode.getNumChildren() < 3:
        return
    coords = find_node(vobj.RootNode, coin.SoCoordinate3)
    if not coords:
        return
    switch = find_node(vobj.RootNode, coin.SoSwitch)
    if not switch:
        return
    num_modes = switch.getNumChildren()
    if num_modes < 3:
        return
    # the number of display modes under switch can vary.
    # the last 4 ones are the ones that are defined for
    # Part features
    faces = switch.getChild(num_modes-3)
    edges = switch.getChild(num_modes-2)
    fset = None
    if faces.getNumChildren() >= 7:
        fset = faces.getChild(6)  # SoBrepFaceSet
    eset = None
    if edges.getNumChildren() >= 1:
        if edges.getChild(0).getNumChildren() >= 4:
            eset = edges.getChild(0).getChild(3)  # SoBrepEdgeSet
    # reset faces and edges
    if fset:
        fset.coordIndex.deleteValues(0)
    if eset:
        eset.coordIndex.deleteValues(0)
    coords.point.deleteValues(0)
    if not node:
        return
    if node[1] and node[3] and eset:
        coords.point.setValues(node[1])
        eset.coordIndex.setValues(node[3])
        if node[2] and node[4] and fset:
            fset.coordIndex.setValues(node[2])
            fset.partIndex.setValues(node[4])


def print_debug(obj):
    """Prints some debug info when an element could not be rendered"""

    element = ifc_tools.get_ifc_element(obj)
    if not element:
        return
    if element.is_a("IfcContext"):
        return
    if element.is_a("IfcSpatialStructureElement"):
        return
    FreeCAD.Console.PrintLog(
        "DEBUG: No Shape returned for element {}, {}, {}\n".format(
            element.id(), element.is_a(), getattr(element, "Name", "")
        )
    )


def apply_placement(node, placement):
    """Applies the given placement to the verts in the given node"""

    verts = [tuple(placement.multVec(FreeCAD.Vector(v))) for v in node[1]]
    return [node[0], verts, node[2], node[3]]


def unify(nodes):
    """group the subcomponents of a node into one single set of verts, faces, edges"""

    colors = []
    verts = []
    faces = []
    edges = []
    parts = []
    for node in nodes:
        vindex = len(verts)
        colors.append(node[0])
        verts.extend(node[1])
        faces.extend([i + vindex if i >= 0 else i for i in node[2]])
        edges.extend([i + vindex if i >= 0 else i for i in node[3]])
        parts.append(len(node[2]) // 4)
    return [colors, verts, faces, edges, parts]


def create_ghost(document, ifcfile, project):
    """Creates a coin representation of the given ifcfile in the given document"""

    if not FreeCAD.GuiUp:
        return
    if not document:
        return
    delete_ghost(document)
    sg = FreeCADGui.getDocument(document.Name).ActiveView.getSceneGraph()
    elements = get_decomposed_elements(project)
    elements = filter_types(elements)
    node, placement = generate_coin(ifcfile, elements)[0]
    document.Proxy.ghost = node
    sg.addChild(document.Proxy.ghost)


def delete_ghost(document):
    """Deletes the associated ghost of the document"""

    if hasattr(document, "Proxy"):
        if hasattr(document.Proxy, "ghost"):
            sg = FreeCADGui.getDocument(document.Name).ActiveView.getSceneGraph()
            sg.removeChild(document.Proxy.ghost)
            del document.Proxy.ghost


def get_annotation_shape(annotation, ifcfile, coin=False):
    """Returns a shape or a coin node form an IFC annotation.
    Returns [colors, verts, faces, edges], colors and faces
    being normally None for 2D shapes."""

    import Part
    from importers import importIFCHelper

    shape = None
    placement = None
    ifcscale = importIFCHelper.getScaling(ifcfile)
    shapes2d = []
    if hasattr(annotation, "Representation"):
        for rep in annotation.Representation.Representations:
            if rep.RepresentationIdentifier in ["Annotation", "FootPrint", "Axis"]:
                sh = importIFCHelper.get2DShape(rep, ifcscale, notext=True)
                if sh:
                    shapes2d.extend(sh)
    elif hasattr(annotation, "AxisCurve"):
        sh = importIFCHelper.get2DShape(annotation.AxisCurve, ifcscale, notext=True)
        shapes2d.extend(sh)
    if shapes2d:
        shape = Part.makeCompound(shapes2d)
        if hasattr(annotation, "ObjectPlacement"):
            placement = importIFCHelper.getPlacement(annotation.ObjectPlacement, ifcscale)
        else:
            placement = None
        if coin:
            iv = shape.writeInventor()
            iv = iv.replace("\n", "")
            segs = re.findall(r"point \[.*?\]",iv)
            segs = [s.replace("point [","").replace("]","").strip() for s in segs]
            segs = [s.split("    ") for s in segs]
            verts = []
            edges = []
            for pair in segs:
                v1 = tuple([float(v) for v in pair[0].split()])
                v2 = tuple([float(v) for v in pair[1].split()])
                if not v1 in verts:
                    verts.append(v1)
                edges.append(verts.index(v1))
                if not v2 in verts:
                    verts.append(v2)
                edges.append(verts.index(v2))
                edges.append(-1)
            shape = [[None, verts, [], edges]]
            # unify nodes
            shape = unify(shape)
    return shape, placement
