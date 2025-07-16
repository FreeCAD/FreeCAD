# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
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

"""This NativeIFC module deals with layers"""

import ifcopenshell
import ifcopenshell.util.element

from . import ifc_tools


def load_layers(obj):
    """Loads all the layers of an IFC file"""

    proj = ifc_tools.get_project(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    layers = ifcfile.by_type("IfcPresentationLayerAssignment")
    for layer in layers:
        obj = get_layer(layer, proj)
        populate_layer(obj)


def has_layers(obj):
    """Returns true if the given project has layers"""

    ifcfile = ifc_tools.get_ifcfile(obj)
    layers = ifcfile.by_type("IfcPresentationLayerAssignment")
    if layers:
        return True
    return False


def get_layer(layer, project):
    """Returns (creates if necessary) a layer object in the given project"""

    group = ifc_tools.get_group(project, "IfcLayersGroup")
    if not group:
        return None
    if hasattr(project, "Document"):
        doc = project.Document
    else:
        doc = project
    exobj = ifc_tools.get_object(layer, doc)
    if exobj:
        return exobj
    obj = ifc_tools.add_object(doc, otype="layer")
    ifcfile = ifc_tools.get_ifcfile(project)
    ifc_tools.add_properties(obj, ifcfile, layer)
    group.addObject(obj)
    return obj


def populate_layer(obj):
    """Attaches all the possible objects to this layer"""

    g = []
    element = ifc_tools.get_ifc_element(obj)
    for shape in getattr(element, "AssignedItems", []):
        rep = getattr(shape, "OfProductRepresentation", None)
        for prod in getattr(rep, "ShapeOfProduct", []):
            obj = ifc_tools.get_object(prod)
            if obj:
                g.append(obj)
    obj.Group = g


def add_layers(obj, element=None, ifcfile=None, proj=None):
    """Creates necessary layers for the given object"""

    if not ifcfile:
        ifcfile = ifc_tools.get_ifcfile(obj)
    if not element:
        element = ifc_tools.get_ifc_element(obj, ifcfile)
    if not proj:
        proj = ifc_tools.get_project(obj)
    layers = ifcopenshell.util.element.get_layers(ifcfile, element)
    for layer in layers:
        lay = get_layer(layer, proj)
        if lay and not obj in lay.Group:
            lay.Proxy.addObject(lay, obj)


def add_to_layer(obj, layer):
    """Adds the given object to the given layer"""

    if hasattr(obj, "StepId"):
        obj_element = ifc_tools.get_ifc_element(obj)
    elif hasattr(obj, "id"):
        obj_element = obj
        obj = ifc_tools.get_object(obj_element)
    else:
        return
    if hasattr(layer, "StepId"):
        layer_element = ifc_tools.get_ifc_element(layer)
    elif hasattr(layer, "id"):
        layer_element = layer
        layer = ifc_tools.get_object(layer_element)
    else:
        return
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not ifcfile:
        return
    items = ()
    if layer_element.AssignedItems:
        items = layer_element.AssignedItems
    if not obj_element in items:
        cmd = "attribute.edit_attributes"
        attribs = {"AssignedItems": items + (obj_element,)}
        ifc_tools.api_run(cmd, ifcfile, product=layer_element, attributes=attribs)
    if not obj in layer.Group:
        layer.Proxy.addObject(layer, obj)


def create_layer(name, project):
    """Adds a new layer to the given project"""

    group = ifc_tools.get_group(project, "IfcLayersGroup")
    ifcfile = ifc_tools.get_ifcfile(project)
    try:
        # IfcopenShell 0.8
        layer = ifc_tools.api_run("layer.add_layer", ifcfile, name=name)
    except:
        # IfcopenShell 0.7
        layer = ifc_tools.api_run("layer.add_layer", ifcfile, Name=name)
    return get_layer(layer, project)


def transfer_layer(layer, project):
    """Transfer a non-NativeIFC layer to a project"""

    label = layer.Label
    ifclayer = create_layer(label, project)
    delete = not (ifc_tools.PARAMS.GetBool("KeepAggregated", False))
    # delete the old one if empty and delete param allows
    if delete and not layer.Group:
        layer.Document.removeObject(layer.Name)
    ifclayer.Label = label  # to avoid 001-ing the Label...
    return ifclayer
