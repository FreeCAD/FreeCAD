# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
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

"""This is the main NativeIFC module"""

import os

# heavyweight libraries - ifc_tools should always be lazy loaded

import FreeCAD
import Draft
import Arch
from importers import exportIFC
from importers import exportIFCHelper

import ifcopenshell
from ifcopenshell import geom
from ifcopenshell import api
from ifcopenshell import template
from ifcopenshell.util import element
from ifcopenshell.util import attribute
from ifcopenshell.util import schema
from ifcopenshell.util import placement
from ifcopenshell.util import unit

from nativeifc import ifc_objects
from nativeifc import ifc_viewproviders
from nativeifc import ifc_import
from nativeifc import ifc_layers
from nativeifc import ifc_status

SCALE = 1000.0  # IfcOpenShell works in meters, FreeCAD works in mm
SHORT = False  # If True, only Step ID attribute is created
ROUND = 8  # rounding value for placements
DEFAULT_SHAPEMODE = "Coin"  # Can be Shape, Coin or None
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")


def create_document(document, filename=None, shapemode=0, strategy=0, silent=False):
    """Creates a IFC document object in the given FreeCAD document or converts that
    document into an IFC document, depending on the state of the statusbar lock button.

    filename:  If not given, a blank IFC document is created
    shapemode: 0 = full shape
               1 = coin only
               2 = no representation
    strategy:  0 = only root object
               1 = only bbuilding structure,
               2 = all children
    """

    if ifc_status.get_lock_status():
        return convert_document(document, filename, shapemode, strategy, silent)
    else:
        return create_document_object(document, filename, shapemode, strategy, silent)


def create_document_object(
    document, filename=None, shapemode=0, strategy=0, silent=False
):
    """Creates a IFC document object in the given FreeCAD document.

    filename:  If not given, a blank IFC document is created
    shapemode: 0 = full shape
               1 = coin only
               2 = no representation
    strategy:  0 = only root object
               1 = only bbuilding structure,
               2 = all children
    """

    obj = add_object(document, otype="project")
    ifcfile, project, full = setup_project(obj, filename, shapemode, silent)
    # populate according to strategy
    if strategy == 0:
        pass
    elif strategy == 1:
        create_children(obj, ifcfile, recursive=True, only_structure=True)
    elif strategy == 2:
        create_children(obj, ifcfile, recursive=True, assemblies=False)
    # create default structure
    if full:
        site = aggregate(Arch.makeSite(), obj)
        building = aggregate(Arch.makeBuilding(), site)
        storey = aggregate(Arch.makeFloor(), building)
    return obj


def convert_document(document, filename=None, shapemode=0, strategy=0, silent=False):
    """Converts the given FreeCAD document to an IFC document.

    filename:  If not given, a blank IFC document is created
    shapemode: 0 = full shape
               1 = coin only
               2 = no representation
    strategy:  0 = only root object
               1 = only bbuilding structure
               2 = all children
               3 = no children
    """

    if not "Proxy" in document.PropertiesList:
        document.addProperty("App::PropertyPythonObject", "Proxy")
    document.setPropertyStatus("Proxy", "Transient")
    document.Proxy = ifc_objects.document_object()
    ifcfile, project, full = setup_project(document, filename, shapemode, silent)
    if strategy == 0:
        create_children(document, ifcfile, recursive=False)
    elif strategy == 1:
        create_children(document, ifcfile, recursive=True, only_structure=True)
    elif strategy == 2:
        create_children(document, ifcfile, recursive=True, assemblies=False)
    elif strategy == 3:
        pass
    # create default structure
    if full:
        site = aggregate(Arch.makeSite(), document)
        building = aggregate(Arch.makeBuilding(), site)
        storey = aggregate(Arch.makeFloor(), building)
    return document


def setup_project(proj, filename, shapemode, silent):
    """Sets up a project (common operations between single doc/not single doc modes)
    Returns the ifcfile object, the project ifc entity, and full (True/False)"""

    full = False
    d = "The path to the linked IFC file"
    if not "IfcFilePath" in proj.PropertiesList:
        proj.addProperty("App::PropertyFile", "IfcFilePath", "Base", d)
    if not "Modified" in proj.PropertiesList:
        proj.addProperty("App::PropertyBool", "Modified", "Base")
    proj.setPropertyStatus("Modified", "Hidden")
    if filename:
        # opening existing file
        proj.IfcFilePath = filename
        ifcfile = ifcopenshell.open(filename)
    else:
        # creating a new file
        if not silent:
            full = ifc_import.get_project_type()
        ifcfile = create_ifcfile()
    project = ifcfile.by_type("IfcProject")[0]
    # TODO configure version history
    # https://blenderbim.org/docs-python/autoapi/ifcopenshell/api/owner/create_owner_history/index.html
    # In IFC4, history is optional. What should we do here?
    proj.Proxy.ifcfile = ifcfile
    add_properties(proj, ifcfile, project, shapemode=shapemode)
    if not "Schema" in proj.PropertiesList:
        proj.addProperty("App::PropertyEnumeration", "Schema", "Base")
    # bug in FreeCAD - to avoid a crash, pre-populate the enum with one value
    proj.Schema = [ifcfile.wrapped_data.schema_name()]
    proj.Schema = ifcfile.wrapped_data.schema_name()
    proj.Schema = ifcopenshell.ifcopenshell_wrapper.schema_names()
    return ifcfile, project, full


def create_ifcfile():
    """Creates a new, empty IFC document"""

    ifcfile = api_run("project.create_file")
    project = api_run("root.create_entity", ifcfile, ifc_class="IfcProject")
    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document")
    user = param.GetString("prefAuthor", "")
    user = user.split("<")[0].strip()
    if user:
        person = api_run("owner.add_person", ifcfile, family_name=user)
    org = param.GetString("prefCompany", "")
    if org:
        organisation = api_run("owner.add_organisation", ifcfile, name=org)
    if user and org:
        api_run(
            "owner.add_person_and_organisation",
            ifcfile,
            person=person,
            organisation=organisation,
        )
    application = "FreeCAD"
    version = FreeCAD.Version()
    version = ".".join([str(v) for v in version[0:3]])
    application = api_run(
        "owner.add_application",
        ifcfile,
        application_full_name=application,
        version=version,
    )
    # context
    model3d = api_run("context.add_context", ifcfile, context_type="Model")
    plan = api_run("context.add_context", ifcfile, context_type="Plan")
    body = api_run(
        "context.add_context",
        ifcfile,
        context_type="Model",
        context_identifier="Body",
        target_view="MODEL_VIEW",
        parent=model3d,
    )
    api_run(
        "context.add_context",
        ifcfile,
        context_type="Model",
        context_identifier="Axis",
        target_view="GRAPH_VIEW",
        parent=model3d,
    )
    # unit
    # for now, assign a default metre unit, as per https://blenderbim.org/docs-python/autoapi/ifcopenshell/api/unit/assign_unit/index.html
    # TODO allow to set this at creation, from the current FreeCAD units schema
    api_run("unit.assign_unit", ifcfile)
    return ifcfile


def api_run(*args, **kwargs):
    """Runs an IfcOpenShell API call and flags the ifcfile as modified"""

    result = ifcopenshell.api.run(*args, **kwargs)
    # *args are typically command, ifcfile
    if len(args) > 1:
        ifcfile = args[1]
        for d in FreeCAD.listDocuments().values():
            for o in d.Objects:
                if hasattr(o, "Proxy") and hasattr(o.Proxy, "ifcfile"):
                    if o.Proxy.ifcfile == ifcfile:
                        o.Modified = True
    return result


def create_object(ifcentity, document, ifcfile, shapemode=0):
    """Creates a FreeCAD object from an IFC entity"""

    exobj = get_object(ifcentity, document)
    if exobj:
        return exobj
    s = "IFC: Created #{}: {}, '{}'\n".format(
        ifcentity.id(), ifcentity.is_a(), ifcentity.Name
    )
    FreeCAD.Console.PrintLog(s)
    obj = add_object(document)
    add_properties(obj, ifcfile, ifcentity, shapemode=shapemode)
    ifc_layers.add_layers(obj, ifcentity, ifcfile)
    if FreeCAD.GuiUp:
        if ifcentity.is_a("IfcSpace") or ifcentity.is_a("IfcOpeningElement"):
            obj.ViewObject.DisplayMode = "Wireframe"
    elements = [ifcentity]
    return obj


def create_children(
    obj,
    ifcfile=None,
    recursive=False,
    only_structure=False,
    assemblies=True,
    expand=False,
):
    """Creates a hierarchy of objects under an object"""

    def get_parent_objects(parent):
        proj = get_project(parent)
        if hasattr(proj, "OutListRecursive"):
            return proj.OutListRecursive
        elif hasattr(proj, "Objects"):
            return proj.Objects

    def create_child(parent, element):
        subresult = []
        # do not create if a child with same stepid already exists
        if not element.id() in [
            getattr(c, "StepId", 0) for c in get_parent_objects(parent)
        ]:
            doc = getattr(parent, "Document", parent)
            mode = getattr(parent, "ShapeMode", "Coin")
            child = create_object(element, doc, ifcfile, mode)
            subresult.append(child)
            if isinstance(parent, FreeCAD.DocumentObject):
                parent.Proxy.addObject(parent, child)
            if element.is_a("IfcSite"):
                # force-create contained buildings too if we just created a site
                buildings = [
                    o for o in get_children(child, ifcfile) if o.is_a("IfcBuilding")
                ]
                for building in buildings:
                    subresult.extend(create_child(child, building))
            elif element.is_a("IfcOpeningElement"):
                # force-create contained windows too if we just created an opening
                windows = [
                    o
                    for o in get_children(child, ifcfile)
                    if o.is_a() in ("IfcWindow", "IfcDoor")
                ]
                for window in windows:
                    subresult.extend(create_child(child, window))
            if recursive:
                subresult.extend(
                    create_children(
                        child, ifcfile, recursive, only_structure, assemblies
                    )
                )
        return subresult

    if not ifcfile:
        ifcfile = get_ifcfile(obj)
    result = []
    children = get_children(obj, ifcfile, only_structure, assemblies, expand)
    for child in children:
        result.extend(create_child(obj, child))
    assign_groups(children)
    return result


def assign_groups(children):
    """Fill the groups inthis list"""

    for child in children:
        if child.is_a("IfcGroup"):
            mode = "IsGroupedBy"
        elif child.is_a("IfcElementAssembly"):
            mode = "IsDecomposedBy"
        else:
            mode = None
        if mode:
            grobj = get_object(child)
            for rel in getattr(child, mode):
                for elem in rel.RelatedObjects:
                    elobj = get_object(elem)
                    if elobj:
                        if len(elobj.InList) == 1:
                            p = elobj.InList[0]
                            if elobj in p.Group:
                                g = p.Group
                                g.remove(elobj)
                                p.Group = g
                        g = grobj.Group
                        g.append(elobj)
                        grobj.Group = g


def get_children(
    obj, ifcfile=None, only_structure=False, assemblies=True, expand=False, iftype=None
):
    """Returns the direct descendants of an object"""

    if not ifcfile:
        ifcfile = get_ifcfile(obj)
    ifcentity = ifcfile[obj.StepId]
    children = []
    if assemblies or not ifcentity.is_a("IfcElement"):
        for rel in getattr(ifcentity, "IsDecomposedBy", []):
            children.extend(rel.RelatedObjects)
    if not only_structure:
        for rel in getattr(ifcentity, "ContainsElements", []):
            children.extend(rel.RelatedElements)
        for rel in getattr(ifcentity, "HasOpenings", []):
            children.extend([rel.RelatedOpeningElement])
        for rel in getattr(ifcentity, "HasFillings", []):
            children.extend([rel.RelatedBuildingElement])
    result = filter_elements(
        children, ifcfile, expand=expand, spaces=True, assemblies=assemblies
    )
    if iftype:
        result = [r for r in result if r.is_a(ifctype)]
    return result


def get_object(element, document=None):
    """Returns the object that references this element, if any"""

    if document:
        ldocs = {"document": document}
    else:
        ldocs = FreeCAD.listDocuments()
    for n, d in ldocs.items():
        for obj in d.Objects:
            if hasattr(obj, "StepId"):
                if obj.StepId == element.id():
                    if get_ifc_element(obj) == element:
                        return obj
    return None


def get_ifcfile(obj):
    """Returns the ifcfile that handles this object"""

    project = get_project(obj)
    if project:
        if getattr(project, "Proxy", None):
            if hasattr(project.Proxy, "ifcfile"):
                return project.Proxy.ifcfile
        if project.IfcFilePath:
            ifcfile = ifcopenshell.open(project.IfcFilePath)
            if hasattr(project, "Proxy"):
                if project.Proxy is None:
                    if not isinstance(project, FreeCAD.DocumentObject):
                        project.Proxy = ifc_objects.document_object()
            if getattr(project, "Proxy", None):
                project.Proxy.ifcfile = ifcfile
            return ifcfile
        else:
            FreeCAD.Console.PrintError("Error: No IFC file attached to this project")
    return None


def get_project(obj):
    """Returns the ifc document this object belongs to.
    obj can be either a document object, an ifcfile or ifc element instance"""

    proj_types = ("IfcProject", "IfcProjectLibrary")
    if isinstance(obj, ifcopenshell.file):
        for d in FreeCAD.listDocuments().values():
            for o in d.Objects:
                if hasattr(o, "Proxy") and hasattr(o.Proxy, "ifcfile"):
                    if o.Proxy.ifcfile == obj:
                        return o
        return None
    if isinstance(obj, ifcopenshell.entity_instance):
        obj = get_object(obj)
    if hasattr(obj, "IfcFilePath"):
        return obj
    if hasattr(getattr(obj, "Document", None), "IfcFilePath"):
        return obj.Document
    if getattr(obj, "Class", None) in proj_types:
        return obj
    if hasattr(obj, "InListRecursive"):
        for parent in obj.InListRecursive:
            if getattr(parent, "Class", None) in proj_types:
                return parent
    return None


def can_expand(obj, ifcfile=None):
    """Returns True if this object can have any more child extracted"""

    if not ifcfile:
        ifcfile = get_ifcfile(obj)
    children = get_children(obj, ifcfile, expand=True)
    group = [o.StepId for o in obj.Group if hasattr(o, "StepId")]
    for child in children:
        if child.id() not in group:
            return True
    return False


def add_object(document, otype=None, oname="IfcObject"):
    """adds a new object to a FreeCAD document.
    otype can be 'project', 'group', 'material', 'layer' or None (normal object)"""

    if not document:
        return None
    proxy = ifc_objects.ifc_object(otype)
    if otype == "group":
        proxy = None
        ftype = "App::DocumentObjectGroupPython"
    elif otype == "material":
        ftype = "App::MaterialObjectPython"
    elif otype == "layer":
        ftype = "App::FeaturePython"
    else:
        ftype = "Part::FeaturePython"
    if otype == "project":
        vp = ifc_viewproviders.ifc_vp_document()
    elif otype == "group":
        vp = ifc_viewproviders.ifc_vp_group()
    elif otype == "material":
        vp = ifc_viewproviders.ifc_vp_material()
    elif otype == "layer":
        vp = None
    else:
        vp = ifc_viewproviders.ifc_vp_object()
    obj = document.addObject(ftype, oname, proxy, vp, False)
    if obj.ViewObject and otype == "layer":
        from draftviewproviders import view_layer  # lazy import

        view_layer.ViewProviderLayer(obj.ViewObject)
        obj.ViewObject.addProperty("App::PropertyBool", "HideChildren", "Layer")
        obj.ViewObject.HideChildren = True
    return obj


def add_properties(
    obj, ifcfile=None, ifcentity=None, links=False, shapemode=0, short=SHORT
):
    """Adds the properties of the given IFC object to a FreeCAD object"""

    if not ifcfile:
        ifcfile = get_ifcfile(obj)
    if not ifcentity:
        ifcentity = get_ifc_element(obj)
    if getattr(ifcentity, "Name", None):
        obj.Label = ifcentity.Name
    elif getattr(obj, "IfcFilePath", ""):
        obj.Label = os.path.splitext(os.path.basename(obj.IfcFilePath))[0]
    else:
        obj.Label = "_" + ifcentity.is_a()
    if isinstance(obj, FreeCAD.DocumentObject) and "Group" not in obj.PropertiesList:
        obj.addProperty("App::PropertyLinkList", "Group", "Base")
    if "ShapeMode" not in obj.PropertiesList:
        obj.addProperty("App::PropertyEnumeration", "ShapeMode", "Base")
        shapemodes = [
            "Shape",
            "Coin",
            "None",
        ]  # possible shape modes for all IFC objects
        if isinstance(shapemode, int):
            shapemode = shapemodes[shapemode]
        obj.ShapeMode = shapemodes
        obj.ShapeMode = shapemode
        if not obj.isDerivedFrom("Part::Feature"):
            obj.setPropertyStatus("ShapeMode", "Hidden")
    if ifcentity.is_a("IfcProduct"):
        obj.addProperty("App::PropertyLink", "Type", "IFC")
    attr_defs = ifcentity.wrapped_data.declaration().as_entity().all_attributes()
    try:
        info_ifcentity = ifcentity.get_info()
    except:
        # slower but no errors
        info_ifcentity = get_elem_attribs(ifcentity)
    for attr, value in info_ifcentity.items():
        if attr == "type":
            attr = "Class"
        elif attr == "id":
            attr = "StepId"
        elif attr == "Name":
            continue
        if short and attr not in ("Class", "StepId"):
            continue
        attr_def = next((a for a in attr_defs if a.name() == attr), None)
        data_type = (
            ifcopenshell.util.attribute.get_primitive_type(attr_def)
            if attr_def
            else None
        )
        if attr == "Class":
            # main enum property, not saved to file
            if attr not in obj.PropertiesList:
                obj.addProperty("App::PropertyEnumeration", attr, "IFC")
                obj.setPropertyStatus(attr, "Transient")
            # to avoid bug/crash: we populate first the property with only the
            # class, then we add the sibling classes
            setattr(obj, attr, [value])
            setattr(obj, attr, value)
            setattr(obj, attr, get_ifc_classes(obj, value))
            # companion hidden propertym that gets saved to file
            if "IfcClass" not in obj.PropertiesList:
                obj.addProperty("App::PropertyString", "IfcClass", "IFC")
                obj.setPropertyStatus("IfcClass", "Hidden")
            setattr(obj, "IfcClass", value)
        elif attr_def and "IfcLengthMeasure" in str(attr_def.type_of_attribute()):
            obj.addProperty("App::PropertyDistance", attr, "IFC")
            if value:
                setattr(obj, attr, value * (1 / get_scale(ifcfile)))
        elif isinstance(value, int):
            if attr not in obj.PropertiesList:
                obj.addProperty("App::PropertyInteger", attr, "IFC")
                if attr == "StepId":
                    obj.setPropertyStatus(attr, "ReadOnly")
            setattr(obj, attr, value)
        elif isinstance(value, float):
            if attr not in obj.PropertiesList:
                obj.addProperty("App::PropertyFloat", attr, "IFC")
            setattr(obj, attr, value)
        elif data_type == "boolean":
            if attr not in obj.PropertiesList:
                obj.addProperty("App::PropertyBool", attr, "IFC")
            if not value or value in ["UNKNOWN", "FALSE"]:
                value = False
            elif not isinstance(value, bool):
                print("DEBUG: attempting to set boolean value:", attr, value)
                value = bool(value)
            setattr(obj, attr, value)  # will trigger error. TODO: Fix this
        elif isinstance(value, ifcopenshell.entity_instance):
            if links:
                if attr not in obj.PropertiesList:
                    # value = create_object(value, obj.Document)
                    obj.addProperty("App::PropertyLink", attr, "IFC")
                # setattr(obj, attr, value)
        elif isinstance(value, (list, tuple)) and value:
            if isinstance(value[0], ifcopenshell.entity_instance):
                if links:
                    if attr not in obj.PropertiesList:
                        # nvalue = []
                        # for elt in value:
                        #    nvalue.append(create_object(elt, obj.Document))
                        obj.addProperty("App::PropertyLinkList", attr, "IFC")
                    # setattr(obj, attr, nvalue)
        elif data_type == "enum":
            if attr not in obj.PropertiesList:
                obj.addProperty("App::PropertyEnumeration", attr, "IFC")
            items = ifcopenshell.util.attribute.get_enum_items(attr_def)
            if value not in items:
                for v in ("UNDEFINED", "NOTDEFINED", "USERDEFINED"):
                    if v in items:
                        value = v
                        break
            if value in items:
                # to prevent bug/crash, we first need to populate the
                # enum with the value about to be used, then
                # add the alternatives
                setattr(obj, attr, [value])
                setattr(obj, attr, value)
                setattr(obj, attr, items)
        else:
            if attr not in obj.PropertiesList:
                obj.addProperty("App::PropertyString", attr, "IFC")
            if value is not None:
                setattr(obj, attr, str(value))
    # link Label2 and Description
    if "Description" in obj.PropertiesList and hasattr(obj, "setExpression"):
        obj.setExpression("Label2", "Description")


def remove_unused_properties(obj):
    """Remove IFC properties if they are not part of the current IFC class"""

    elt = get_ifc_element(obj)
    props = list(elt.get_info().keys())
    props[props.index("id")] = "StepId"
    props[props.index("type")] = "Class"
    for prop in obj.PropertiesList:
        if obj.getGroupOfProperty(prop) == "IFC":
            if prop not in props:
                obj.removeProperty(prop)


def get_ifc_classes(obj, baseclass):
    """Returns a list of sibling classes from a given FreeCAD object"""

    # this function can become pure IFC

    if baseclass in ("IfcProject", "IfcProjectLibrary"):
        return ("IfcProject", "IfcProjectLibrary")
    ifcfile = get_ifcfile(obj)
    if not ifcfile:
        return [baseclass]
    classes = []
    schema = ifcfile.wrapped_data.schema_name()
    schema = ifcopenshell.ifcopenshell_wrapper.schema_by_name(schema)
    declaration = schema.declaration_by_name(baseclass)
    if "StandardCase" in baseclass:
        declaration = declaration.supertype()
    if declaration.supertype():
        # include sibling classes
        classes = [sub.name() for sub in declaration.supertype().subtypes()]
        # include superclass too so one can "navigate up"
        classes.append(declaration.supertype().name())
    # also include subtypes of the current class (ex, StandardCases)
    classes.extend([sub.name() for sub in declaration.subtypes()])
    if baseclass not in classes:
        classes.append(baseclass)
    return classes


def get_ifc_element(obj, ifcfile=None):
    """Returns the corresponding IFC element of an object"""

    if not ifcfile:
        ifcfile = get_ifcfile(obj)
    if ifcfile and hasattr(obj, "StepId"):
        try:
            return ifcfile.by_id(obj.StepId)
        except RuntimeError:
            # entity not found
            pass
    return None


def has_representation(element):
    """Tells if an elements has an own representation"""

    # This function can become pure IFC

    if hasattr(element, "Representation") and element.Representation:
        return True
    return False


def filter_elements(elements, ifcfile, expand=True, spaces=False, assemblies=True):
    """Filter elements list of unwanted classes"""

    # This function can become pure IFC

    # gather decomposition if needed
    if not isinstance(elements, (list, tuple)):
        elements = [elements]
    openings = False
    if assemblies and any([e.is_a("IfcOpeningElement") for e in elements]):
        openings = True
    if expand and (len(elements) == 1):
        elem = elements[0]
        if elem.is_a("IfcSpace"):
            spaces = True
        if not has_representation(elem):
            if elem.is_a("IfcProject"):
                elements = ifcfile.by_type("IfcElement")
                elements.extend(ifcfile.by_type("IfcSite"))
            else:
                decomp = ifcopenshell.util.element.get_decomposition(elem)
                if decomp:
                    # avoid replacing elements if decomp is empty
                    elements = decomp
        else:
            if elem.Representation.Representations:
                rep = elem.Representation.Representations[0]
                if (
                    rep.Items
                    and rep.Items[0].is_a() == "IfcPolyline"
                    and elem.IsDecomposedBy
                ):
                    # only use the decomposition and not the polyline
                    # happens for multilayered walls exported by VectorWorks
                    # the Polyline is the wall axis
                    # see https://github.com/yorikvanhavre/FreeCAD-NativeIFC/issues/28
                    elements = ifcopenshell.util.element.get_decomposition(elem)
    if not openings:
        # Never load feature elements by default, they can be lazy loaded
        elements = [e for e in elements if not e.is_a("IfcFeatureElement")]
    # do load spaces when required, otherwise skip computing their shapes
    if not spaces:
        elements = [e for e in elements if not e.is_a("IfcSpace")]
    # skip projects
    elements = [e for e in elements if not e.is_a("IfcProject")]
    # skip furniture for now, they can be lazy loaded probably
    elements = [e for e in elements if not e.is_a("IfcFurnishingElement")]
    # skip annotations for now
    elements = [e for e in elements if not e.is_a("IfcAnnotation")]
    return elements


def set_attribute(ifcfile, element, attribute, value):
    """Sets the value of an attribute of an IFC element"""

    # This function can become pure IFC

    def differs(val1, val2):
        if val1 == val2:
            return False
        if not val1 and not val2:
            return False
        if val1 is None and "NOTDEFINED" in str(val2).upper():
            return False
        if val1 is None and "UNDEFINED" in str(val2).upper():
            return False
        if val2 is None and "NOTDEFINED" in str(val1).upper():
            return False
        if val2 is None and "UNDEFINED" in str(val1).upper():
            return False
        return True

    if not ifcfile or not element:
        return False
    if isinstance(value, FreeCAD.Units.Quantity):
        f = get_scale(ifcfile)
        value = value.Value * f
    if attribute == "Class":
        if value != element.is_a():
            if value and value.startswith("Ifc"):
                cmd = "root.reassign_class"
                FreeCAD.Console.PrintLog(
                    "Changing IFC class value: "
                    + element.is_a()
                    + " to "
                    + str(value)
                    + "\n"
                )
                product = api_run(cmd, ifcfile, product=element, ifc_class=value)
                # TODO fix attributes
                return product
    cmd = "attribute.edit_attributes"
    attribs = {attribute: value}
    if hasattr(element, attribute):
        if (
            attribute == "Name"
            and getattr(element, attribute) is None
            and value.startswith("_")
        ):
            # do not consider default FreeCAD names given to unnamed alements
            return False
        if differs(getattr(element, attribute, None),value):
            FreeCAD.Console.PrintLog(
                "Changing IFC attribute value of "
                + str(attribute)
                + ": "
                + str(value)
                + "\n"
            )
            api_run(cmd, ifcfile, product=element, attributes=attribs)
            return True
    return False


def set_colors(obj, colors):
    """Sets the given colors to an object"""

    if FreeCAD.GuiUp and colors:
        try:
            vobj = obj.ViewObject
        except ReferenceError:
            # Object was probably deleted
            return
        # ifcopenshell issues (-1,-1,-1) colors if not set
        if isinstance(colors[0], (tuple, list)):
            colors = [tuple([abs(d) for d in c]) for c in colors]
        else:
            colors = [abs(c) for c in colors]
        if hasattr(vobj, "ShapeColor"):
            # 1.0 materials
            if not isinstance(colors[0], (tuple, list)):
                colors = [colors]
            # set the first color to opaque otherwise it spoils object transparency
            if len(colors) > 1:
                #colors[0] = colors[0][:3] + (0.0,)
                # TEMP HACK: if multiple colors, set everything to opaque because it looks wrong
                colors = [color[:3] + (0.0,) for color in colors]
            sapp = []
            for color in colors:
                sapp_mat = FreeCAD.Material()
                if len(color) < 4:
                    sapp_mat.DiffuseColor = color + (1.0,)
                else:
                    sapp_mat.DiffuseColor = color[:3] + (1.0 - color[3],)
                sapp_mat.Transparency = color[3] if len(color) > 3 else 0.0
                sapp.append(sapp_mat)
            #print(vobj.Object.Label,[[m.DiffuseColor,m.Transparency] for m in sapp])
            vobj.ShapeAppearance = sapp


def get_body_context_ids(ifcfile):
    # This function can become pure IFC

    # Facetation is to accommodate broken Revit files
    # See https://forums.buildingsmart.org/t/suggestions-on-how-to-improve-clarity\
    # -of-representation-context-usage-in-documentation/3663/6?u=moult
    body_contexts = [
        c.id()
        for c in ifcfile.by_type("IfcGeometricRepresentationSubContext")
        if c.ContextIdentifier in ["Body", "Facetation"]
    ]
    # Ideally, all representations should be in a subcontext, but some BIM apps don't do this
    # correctly, so we add main contexts too
    body_contexts.extend(
        [
            c.id()
            for c in ifcfile.by_type(
                "IfcGeometricRepresentationContext", include_subtypes=False
            )
            if c.ContextType == "Model"
        ]
    )
    return body_contexts


def get_plan_contexts_ids(ifcfile):
    # This function can become pure IFC

    # Annotation is to accommodate broken Revit files
    # See https://github.com/Autodesk/revit-ifc/issues/187
    return [
        c.id()
        for c in ifcfile.by_type("IfcGeometricRepresentationContext")
        if c.ContextType in ["Plan", "Annotation"]
    ]


def get_freecad_matrix(ios_matrix):
    """Converts an IfcOpenShell matrix tuple into a FreeCAD matrix"""

    # https://github.com/IfcOpenShell/IfcOpenShell/issues/1440
    # https://pythoncvc.net/?cat=203
    # https://github.com/IfcOpenShell/IfcOpenShell/issues/4832#issuecomment-2158583873
    m_l = list()
    for i in range(3):
        if len(ios_matrix) == 16:
            # IfcOpenShell 0.8
            line = list(ios_matrix[i::4])
        else:
            # IfcOpenShell 0.7
            line = list(ios_matrix[i::3])
        line[-1] *= SCALE
        m_l.extend(line)
    return FreeCAD.Matrix(*m_l)


def get_ios_matrix(m):
    """Converts a FreeCAD placement or matrix into an IfcOpenShell matrix tuple"""

    if isinstance(m, FreeCAD.Placement):
        m = m.Matrix
    mat = [
        [m.A11, m.A12, m.A13, m.A14],
        [m.A21, m.A22, m.A23, m.A24],
        [m.A31, m.A32, m.A33, m.A34],
        [m.A41, m.A42, m.A42, m.A44],
    ]
    # apply rounding because OCCT often changes 1.0 to 0.99999999999 or something
    rmat = []
    for row in mat:
        rmat.append([round(e, ROUND) for e in row])
    return rmat


def get_scale(ifcfile):
    """Returns the scale factor to convert any file length to mm"""

    scale = ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    # the above lines yields meter -> file unit scale factor. We need mm
    return 0.001 / scale


def set_placement(obj):
    """Updates the internal IFC placement according to the object placement"""

    # This function can become pure IFC

    ifcfile = get_ifcfile(obj)
    if not ifcfile:
        print("DEBUG: No ifc file for object", obj.Label, "Aborting")
    if obj.Class in ["IfcProject", "IfcProjectLibrary"]:
        return
    element = get_ifc_element(obj)
    placement = FreeCAD.Placement(obj.Placement)
    placement.Base = FreeCAD.Vector(placement.Base).multiply(get_scale(ifcfile))
    new_matrix = get_ios_matrix(placement)
    old_matrix = ifcopenshell.util.placement.get_local_placement(
        element.ObjectPlacement
    )
    # conversion from numpy array
    old_matrix = old_matrix.tolist()
    old_matrix = [[round(c, ROUND) for c in r] for r in old_matrix]
    if new_matrix != old_matrix:
        FreeCAD.Console.PrintLog(
            "IFC: placement changed for "
            + obj.Label
            + " old: "
            + str(old_matrix)
            + " new: "
            + str(new_matrix)
            + "\n"
        )
        api = "geometry.edit_object_placement"
        api_run(api, ifcfile, product=element, matrix=new_matrix, is_si=False)
        return True
    return False


def save_ifc(obj, filepath=None):
    """Saves the linked IFC file of a project, but does not mark it as saved"""

    if not filepath:
        if getattr(obj, "IfcFilePath", None):
            filepath = obj.IfcFilePath
    if filepath:
        ifcfile = get_ifcfile(obj)
        if not ifcfile:
            ifcfile = create_ifcfile()
        ifcfile.write(filepath)
        FreeCAD.Console.PrintMessage("Saved " + filepath + "\n")


def save(obj, filepath=None):
    """Saves the linked IFC file of a project and set its saved status"""

    save_ifc(obj, filepath)
    obj.Modified = False


def aggregate(obj, parent, mode=None):
    """Takes any FreeCAD object and aggregates it to an existing IFC object.
    Mode can be 'opening' to force-create a subtraction"""

    proj = get_project(parent)
    if not proj:
        FreeCAD.Console.PrintError("The parent object is not part of an IFC project\n")
        return
    ifcfile = get_ifcfile(proj)
    if not ifcfile:
        return
    product = None
    stepid = getattr(obj, "StepId", None)
    if stepid:
        # obj might be dragging at this point and has no project anymore
        try:
            elem = ifcfile[stepid]
            if obj.GlobalId == elem.GlobalId:
                product = elem
        except:
            pass
    if product:
        # this object already has an associated IFC product
        print("DEBUG:", obj.Label, "is already part of the IFC document")
        newobj = obj
        new = False
    else:
        ifcclass = None
        if mode == "opening":
            ifcclass = "IfcOpeningElement"
        product = create_product(obj, parent, ifcfile, ifcclass)
        shapemode = getattr(parent, "ShapeMode", DEFAULT_SHAPEMODE)
        newobj = create_object(product, obj.Document, ifcfile, shapemode)
        new = True
    create_relationship(obj, newobj, parent, product, ifcfile, mode)
    base = getattr(obj, "Base", None)
    if base:
        # make sure the base is used only by this object before deleting
        if base.InList != [obj]:
            base = None
    # handle layer
    if FreeCAD.GuiUp:
        import FreeCADGui

        autogroup = getattr(
            getattr(FreeCADGui, "draftToolBar", None), "autogroup", None
        )
        if autogroup is not None:
            layer = FreeCAD.ActiveDocument.getObject(autogroup)
            if hasattr(layer, "StepId"):
                ifc_layers.add_to_layer(newobj, layer)
    # aggregate dependent objects
    for child in obj.InList:
        if hasattr(child,"Host") and child.Host == obj:
            aggregate(child, newobj)
        elif hasattr(child,"Hosts") and obj in child.Hosts:
            #op = create_product(child, newobj, ifcfile, ifcclass="IfcOpeningElement")
            aggregate(child, newobj)
    delete = not (PARAMS.GetBool("KeepAggregated", False))
    if new and delete and base:
        obj.Document.removeObject(base.Name)
    label = obj.Label
    if new and delete:
        obj.Document.removeObject(obj.Name)
    if new:
        newobj.Label = label  # to avoid 001-ing the Label...
    return newobj


def deaggregate(obj, parent):
    """Removes a FreeCAD object form its parent"""

    ifcfile = get_ifcfile(obj)
    element = get_ifc_element(obj)
    if not element:
        return
    try:
        api_run("aggregate.unassign_object", ifcfile, products=[element])
    except:
        # older version of ifcopenshell
        api_run("aggregate.unassign_object", ifcfile, product=element)
    parent.Proxy.removeObject(parent, obj)


def create_product(obj, parent, ifcfile, ifcclass=None):
    """Creates an IFC product out of a FreeCAD object"""

    name = obj.Label
    description = getattr(obj, "Description", None)
    if not ifcclass:
        ifcclass = get_ifctype(obj)
    representation, placement, shapetype = create_representation(obj, ifcfile)
    product = api_run("root.create_entity", ifcfile, ifc_class=ifcclass, name=name)
    set_attribute(ifcfile, product, "Description", description)
    set_attribute(ifcfile, product, "ObjectPlacement", placement)
    # TODO below cannot be used at the moment because the ArchIFC exporter returns an
    # IfcProductDefinitionShape already and not an IfcShapeRepresentation
    # api_run("geometry.assign_representation", ifcfile, product=product, representation=representation)
    set_attribute(ifcfile, product, "Representation", representation)
    # additions
    if hasattr(obj,"Additions") and shapetype in ["extrusion","no shape"]:
        for addobj in obj.Additions:
            r2,p2,c2 = create_representation(addobj, ifcfile)
            cl2 = get_ifctype(addobj)
            addprod = api_run("root.create_entity", ifcfile, ifc_class=cl2, name=addobj.Label)
            set_attribute(ifcfile, addprod, "Description", getattr(addobj, "Description", ""))
            set_attribute(ifcfile, addprod, "ObjectPlacement", p2)
            set_attribute(ifcfile, addprod, "Representation", r2)
            create_relationship(None, addobj, product, addprod, ifcfile)
    # subtractions
    if hasattr(obj,"Subtractions") and shapetype in ["extrusion","no shape"]:
        for subobj in obj.Subtractions:
            r3,p3,c3 = create_representation(subobj, ifcfile)
            cl3 = "IfcOpeningElement"
            subprod = api_run("root.create_entity", ifcfile, ifc_class=cl3, name=subobj.Label)
            set_attribute(ifcfile, subprod, "Description", getattr(subobj, "Description", ""))
            set_attribute(ifcfile, subprod, "ObjectPlacement", p3)
            set_attribute(ifcfile, subprod, "Representation", r3)
            create_relationship(None, subobj, product, subprod, ifcfile)
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
    return representation, placement, shapetype


def get_ifctype(obj):
    """Returns a valid IFC type from an object"""

    if hasattr(obj, "Class"):
        if "ifc" in str(obj.Class).lower():
            return obj.Class
    if hasattr(obj,"IfcType") and obj.IfcType != "Undefined":
        return "Ifc" + obj.IfcType.replace(" ","")
    dtype = Draft.getType(obj)
    if dtype in ["App::Part","Part::Compound","Array"]:
        return "IfcElementAssembly"
    if dtype in ["App::DocumentObjectGroup"]:
        ifctype = "IfcGroup"
    return "IfcBuildingElementProxy"


def get_export_preferences(ifcfile):
    """returns a preferences dict for exportIFC"""

    prefs = exportIFC.getPreferences()
    prefs["SCHEMA"] = ifcfile.wrapped_data.schema_name()
    s = ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    # the above lines yields meter -> file unit scale factor. We need mm
    prefs["SCALE_FACTOR"] = 0.001 / s
    context = ifcfile[
        get_body_context_ids(ifcfile)[0]
    ]  # we take the first one (first found subcontext)
    return prefs, context


def get_subvolume(obj):
    """returns a subface + subvolume from a window object"""

    tempface = None
    tempobj = None
    tempshape = None
    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "getSubVolume"):
        tempshape = obj.Proxy.getSubVolume(obj)
    elif hasattr(obj, "Subvolume") and obj.Subvolume:
        tempshape = obj.Subvolume
    if tempshape:
        if len(tempshape.Faces) == 6:
            # We assume the standard output of ArchWindows
            faces = sorted(tempshape.Faces, key=lambda f: f.CenterOfMass.z)
            baseface = faces[0]
            ext = faces[-1].CenterOfMass.sub(faces[0].CenterOfMass)
            tempface = obj.Document.addObject("Part::Feature", "BaseFace")
            tempface.Shape = baseface
            tempobj = obj.Document.addObject("Part::Extrusion", "Opening")
            tempobj.Base = tempface
            tempobj.DirMode = "Custom"
            tempobj.Dir = FreeCAD.Vector(ext).normalize()
            tempobj.LengthFwd = ext.Length
        else:
            tempobj = obj.Document.addObject("Part::Feature", "Opening")
            tempobj.Shape = tempshape
    if tempobj:
        tempobj.recompute()
    return tempface, tempobj


def create_relationship(old_obj, obj, parent, element, ifcfile, mode=None):
    """Creates a relationship between an IFC object and a parent IFC object"""

    if isinstance(parent, (FreeCAD.DocumentObject, FreeCAD.Document)):
        parent_element = get_ifc_element(parent)
    else:
        parent_element = parent
    # case 1: element inside spatiual structure
    if parent_element.is_a("IfcSpatialStructureElement") and element.is_a("IfcElement"):
        # first remove the FreeCAD object from any parent
        if old_obj:
            for old_par in old_obj.InList:
                if hasattr(old_par, "Group") and old_obj in old_par.Group:
                    old_par.Group = [o for o in old_par.Group if o != old_obj]
            try:
                api_run("spatial.unassign_container", ifcfile, products=[element])
            except:
                # older version of IfcOpenShell
                api_run("spatial.unassign_container", ifcfile, product=element)
        if element.is_a("IfcOpeningElement"):
            uprel = api_run(
                "void.add_opening",
                ifcfile,
                opening=element,
                element=parent_element,
            )
        else:
            try:
                uprel = api_run(
                    "spatial.assign_container",
                    ifcfile,
                    products=[element],
                    relating_structure=parent_element,
                )
            except:
                # older version of ifcopenshell
                uprel = api_run(
                    "spatial.assign_container",
                    ifcfile,
                    product=element,
                    relating_structure=parent_element,
                )
    # case 2: door/window inside element
    # https://standards.buildingsmart.org/IFC/RELEASE/IFC4/ADD2_TC1/HTML/annex/annex-e/wall-with-opening-and-window.htm
    elif parent_element.is_a("IfcElement") and element.is_a() in [
        "IfcDoor",
        "IfcWindow",
    ]:
        if old_obj:
            tempface, tempobj = get_subvolume(old_obj)
            if tempobj:
                opening = create_product(tempobj, parent, ifcfile, "IfcOpeningElement")
                set_attribute(ifcfile, product, "Name", "Opening")
                old_obj.Document.removeObject(tempobj.Name)
                if tempface:
                    old_obj.Document.removeObject(tempface.Name)
                api_run(
                    "void.add_opening", ifcfile, opening=opening, element=parent_element
                )
                api_run("void.add_filling", ifcfile, opening=opening, element=element)
        # windows must also be part of a spatial container
        try:
            api_run("spatial.unassign_container", ifcfile, products=[element])
        except:
            # old version of IfcOpenShell
            api_run("spatial.unassign_container", ifcfile, product=element)
        if parent_element.ContainedInStructure:
            container = parent_element.ContainedInStructure[0].RelatingStructure
            try:
                uprel = api_run(
                    "spatial.assign_container",
                    ifcfile,
                    products=[element],
                    relating_structure=container,
                )
            except:
                # old version of IfcOpenShell
                uprel = api_run(
                    "spatial.assign_container",
                    ifcfile,
                    product=element,
                    relating_structure=container,
                )
        elif parent_element.Decomposes:
            container = parent_element.Decomposes[0].RelatingObject
            try:
                uprel = api_run(
                    "aggregate.assign_object",
                    ifcfile,
                    products=[element],
                    relating_object=container,
                )
            except:
                # older version of ifcopenshell
                uprel = api_run(
                    "aggregate.assign_object",
                    ifcfile,
                    product=element,
                    relating_object=container,
                )
    # case 4: void element
    elif (parent_element.is_a("IfcElement") and element.is_a("IfcOpeningElement"))\
    or (mode == "opening"):
        uprel = api_run(
            "void.add_opening", ifcfile, opening=element, element=parent_element
        )
    # case 3: element aggregated inside other element
    else:
        try:
            api_run("aggregate.unassign_object", ifcfile, products=[element])
        except:
            # older version of ifcopenshell
            api_run("aggregate.unassign_object", ifcfile, product=element)
        try:
            uprel = api_run(
                "aggregate.assign_object",
                ifcfile,
                products=[element],
                relating_object=parent_element,
            )
        except:
            # older version of ifcopenshell
            uprel = api_run(
                "aggregate.assign_object",
                ifcfile,
                product=element,
                relating_object=parent_element,
            )
    if hasattr(parent, "Proxy") and hasattr(parent.Proxy, "addObject"):
        parent.Proxy.addObject(parent, obj)
    return uprel


def get_elem_attribs(ifcentity):
    # This function can become pure IFC

    # usually info_ifcentity = ifcentity.get_info() would de the trick
    # the above could raise an unhandled exception on corrupted ifc files
    # in IfcOpenShell
    # see https://github.com/IfcOpenShell/IfcOpenShell/issues/2811
    # thus workaround

    info_ifcentity = {"id": ifcentity.id(), "class": ifcentity.is_a()}

    # get attrib keys
    attribs = []
    for anumber in range(20):
        try:
            attr = ifcentity.attribute_name(anumber)
        except Exception:
            break
        # print(attr)
        attribs.append(attr)

    # get attrib values
    for attr in attribs:
        try:
            value = getattr(ifcentity, attr)
        except Exception as e:
            # print(e)
            value = "Error: {}".format(e)
            print(
                "DEBUG: The entity #{} has a problem on attribute {}: {}".format(
                    ifcentity.id(), attr, e
                )
            )
        # print(value)
        info_ifcentity[attr] = value

    return info_ifcentity


def migrate_schema(ifcfile, schema):
    """migrates a file to a new schema"""

    # This function can become pure IFC

    newfile = ifcopenshell.file(schema=schema)
    migrator = ifcopenshell.util.schema.Migrator()
    table = {}
    for entity in ifcfile:
        new_entity = migrator.migrate(entity, newfile)
        table[entity.id()] = new_entity.id()
    return newfile, table


def remove_ifc_element(obj,delete_obj=False):
    """removes the IFC data associated with an object.
    If delete_obj is True, the FreeCAD object is also deleted"""

    # This function can become pure IFC

    ifcfile = get_ifcfile(obj)
    element = get_ifc_element(obj)
    if ifcfile and element:
        api_run("root.remove_product", ifcfile, product=element)
        if delete_obj:
            obj.Document.removeObject(obj.Name)
        return True
    return False


def get_orphan_elements(ifcfile):
    """returns a list of orphan products in an ifcfile"""

    products = ifcfile.by_type("IfcElement")
    products = [p for p in products if not p.Decomposes]
    products = [p for p in products if not p.ContainedInStructure]
    products = [
        p for p in products if not hasattr(p, "VoidsElements") or not p.VoidsElements
    ]
    return products


def get_group(project, name):
    """returns a group of the given type under the given IFC project. Creates it if needed"""

    if not project:
        return None
    if hasattr(project, "Group"):
        group = project.Group
    elif hasattr(project, "Objects"):
        group = project.Objects
    else:
        group = []
    for c in group:
        if c.isDerivedFrom("App::DocumentObjectGroupPython"):
            if c.Name == name:
                return c
    if hasattr(project, "Document"):
        doc = project.Document
    else:
        doc = project
    group = add_object(doc, otype="group", oname=name)
    group.Label = name.strip("Ifc").strip("Group")
    # if FreeCAD.GuiUp:
    #    group.ViewObject.ShowInTree = PARAMS.GetBool("ShowDataGroups", False)
    if hasattr(project.Proxy, "addObject"):
        project.Proxy.addObject(project, group)
    return group


def load_orphans(obj):
    """loads orphan objects from the given project object"""

    if isinstance(obj, FreeCAD.DocumentObject):
        doc = obj.Document
    else:
        doc = obj
    ifcfile = get_ifcfile(obj)
    shapemode = obj.ShapeMode
    elements = get_orphan_elements(ifcfile)
    for element in elements:
        create_object(element, doc, ifcfile, shapemode)


def remove_tree(objs):
    """Removes all given objects and their children, if not used by others"""

    if not objs:
        return
    doc = objs[0].Document
    nobjs = objs
    for obj in objs:
        for child in obj.OutListRecursive:
            if not child in nobjs:
                nobjs.append(child)
    deletelist = []
    for obj in nobjs:
        for par in obj.InList:
            if par not in nobjs:
                break
        else:
            deletelist.append(obj.Name)
    for n in deletelist:
        doc.removeObject(n)


