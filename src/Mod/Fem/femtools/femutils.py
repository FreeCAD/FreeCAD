# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
""" Collection of functions for the Fem module.

This module contains function for managing a analysis and all the different
types of objects it contains, helper for executing a simulation, function for
extracting relevant parts of geometry and a few unrelated function useful at
various places in the Fem module.
"""


__title__ = "FEM Utilities"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import os
import sys

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui


def createObject(doc, name, proxy, viewProxy=None):
    """ Add python object to document using python type string.

    Add a document object suitable for the *proxy* and the *viewProxy* to *doc*
    and attach it to the *proxy* and the *viewProxy*. This function can only be
    used with python proxies that specify their C++ type via the BaseType class
    member (e.g. Cube.BaseType). If there already exists a object with *name* a
    suitable unique name is generated. To auto generate a name pass ``""``.

    :param doc:         document object to which the object is added
    :param name:        string of the name of new object in *doc*, use
                        ``""`` to generate a name
    :param proxy:       python proxy for new object
    :param viewProxy:   view proxy for new object

    :returns:           reference to new object
    """
    obj = doc.addObject(proxy.BaseType, name)
    proxy(obj)
    if FreeCAD.GuiUp and viewProxy is not None:
        viewProxy(obj.ViewObject)
    return obj


def findAnalysisOfMember(member):
    """ Find Analysis the *member* belongs to.

    :param member: a document object

    :returns:
     If a analysis that contains *member* can be found a reference is returned.
     If no such object exists in the document of *member*, ``None`` is returned.
    """
    if member is None:
        raise ValueError("Member must not be None")
    for obj in member.Document.Objects:
        if obj.isDerivedFrom("Fem::FemAnalysis"):
            if member in obj.Group:
                return obj
            if _searchGroups(member, obj.Group):
                return obj
    return None


def _searchGroups(member, objs):
    for o in objs:
        if o == member:
            return True
        if hasattr(o, "Group"):
            return _searchGroups(member, o.Group)
    return False


def get_member(analysis, t):
    """ Return list of all members of *analysis* of type *t*.

    Search *analysis* for members of type *t*. This method checks the custom
    python typesytem (BaseType class property) used by the Fem module if
    possible. If the object does not use the python typesystem the usual
    isDerivedFrom from the C++ dynamic type system is used.

    :param analysis: only objects part of this analysis are considered
    :param t:        only objects of this type are returned

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    if analysis is None:
        raise ValueError("Analysis must not be None")
    matching = []
    for m in analysis.Group:
        # since is _derived_from is used the father could be used
        # to test too (ex. "Fem::FemMeshObject")
        if is_derived_from(m, t):
            matching.append(m)
    return matching


def get_single_member(analysis, t):
    """ Return one object of type *t* and part of *analysis*.

    Search *analysis* for members of type *t* and return the first one that's
    found. This method checks the custom python typesytem (BaseType class
    property) used by the Fem module if possible. If the object doesn't use the
    python typesystem the usual isDerivedFrom from the C++ dynamic type system
    is used.

    :param analysis: only objects part of this analysis are considered
    :param t:        only a object of this type is returned

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    objs = get_member(analysis, t)
    return objs[0] if objs else None


def get_several_member(analysis, t):
    """ Get members and pack them for Calculix/Z88.

    Collect members by calling :py:func:`get_member` and pack them into a
    data structure that can be consumed by calculix and Z88 solver modules.

    :param analysis: see :py:func:`get_member`
    :param t: see :py:func:`get_member`

    :returns:
     A list containing one dict per member. Each dict has two entries:
     ``"Object"`` and ``"RefShapeType"``. ``dict["Object"]`` contains the
     member document object. ``dict["RefShapeType"]`` contains the shape type
     of the *References* property of the member (used by constraints) as a
     string ("Vertex", "Edge", "Face" or "Solid"). If the member doesn't have a
     *References* property ``dict["RefShapeType"]`` is the empty string ``""``.

    :note:
     Undefined behaviour if one of the members has a *References* property
     which is empty.

    :note:
     Undefined behaviour if the type of the references of one object are not
     all the same.

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    # if no member is found, an empty list is returned
    objs = get_member(analysis, t)
    members = []
    for m in objs:
        obj_dict = {}
        obj_dict["Object"] = m
        obj_dict["RefShapeType"] = get_refshape_type(m)
        members.append(obj_dict)
    return members


def get_mesh_to_solve(analysis):
    """ Find one and only mesh object of *analysis*.

    :returns:
     A tuple ``(object, message)``. If and only if the analysis contains
     exactly one mesh object the first value of the tuple is the mesh document
     object. Otherwise the first value is ``None`` and the second value is a
     error message indicating what went wrong.
    """
    mesh_to_solve = None
    for m in analysis.Group:
        if m.isDerivedFrom("Fem::FemMeshObject") and not is_of_type(m, "Fem::FemMeshResult"):
            if not mesh_to_solve:
                mesh_to_solve = m
            else:
                return (None, "FEM: multiple mesh in analysis not yet supported!")
    if mesh_to_solve is not None:
        return (mesh_to_solve, "")
    else:
        return (None, "FEM: no mesh object found in analysis.")


# typeID and object type defs
def type_of_obj(obj):
    """ Return type of *obj* honoring the special typesystem of Fem.

    Python objects of the Fem workbench define their type via a class member
    ``<Class>.Type``. Return this type if the property exists. If not return
    the conventional ``TypeId`` value.

    :para obj: a document object
    """
    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type
    return obj.TypeId


def is_of_type(obj, ty):
    """ Compare type of *obj* with *ty* honoring Fems typesystem.

    See :py:func:`type_of_obj` for more info about the special typesystem of
    the Fem module.

    :returns:
     ``True`` if *obj* is of type *ty*, ``False`` otherwise. Type must match
     exactly: Derived objects are not considered to be of type of one of their
     super classes.
    """
    return type_of_obj(obj) == ty


def is_derived_from(obj, t):
    """ Check if *obj* is derived from *t* honoring Fems typesytem.

    Essentially just call ``obj.isDerivedFrom(t)`` and return it's value. For
    objects using Fems typesystem (see :py:func:`type_of_obj`) return always
    True if the Fem type is equal to *t*.

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    if (hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type") and obj.Proxy.Type == t):
        return True
    return obj.isDerivedFrom(t)


# ************************************************************************************************
# working dir
def get_pref_working_dir(solver_obj):
    """ Return working directory for solver honoring user settings.

    :throws femsolver.run.MustSaveError:
     If user setting is set to BESIDE and the document isn't saved.

    :note:
     Not working correctly for most cases because this circumvents directory
     caching of the solver framework. For solver use getMachine from run.py
     instead.
    """
    from femsolver import settings
    dir_setting = settings.get_dir_setting()
    if dir_setting == settings.DirSetting.TEMPORARY:
        setting_working_dir = get_temp_dir(solver_obj)
    elif dir_setting == settings.DirSetting.BESIDE:
        setting_working_dir = get_beside_dir(solver_obj)
    elif dir_setting == settings.DirSetting.CUSTOM:
        setting_working_dir = get_custom_dir(solver_obj)
    else:
        setting_working_dir = ""
    return setting_working_dir


# these are a duplicate of the methods in src/Mod/Fem/femsolver/run.py
# see commit xxx (will be added when in master) for more information
# the FEM preferences will be used by both
def get_temp_dir(obj=None):
    from tempfile import mkdtemp
    return mkdtemp(prefix="fcfem_")


def get_beside_dir(obj):
    base = get_beside_base(obj)
    specific_path = os.path.join(base, obj.Label)
    if not os.path.isdir(specific_path):
        os.makedirs(specific_path)
    return specific_path


def get_custom_dir(obj):
    base = get_custom_base(obj)
    specific_path = os.path.join(
        base, obj.Document.Name, obj.Label)
    if not os.path.isdir(specific_path):
        os.makedirs(specific_path)
    return specific_path


def get_beside_base(obj):
    fcstdPath = obj.Document.FileName
    if fcstdPath == "":
        error_message = (
            "Please save the file before executing a solver or creating a mesh. "
            "This must be done because the location of the working directory "
            "is set to \"Beside *.FCStd File\". For the moment a tmp dir is used."
        )
        FreeCAD.Console.PrintError(error_message + "\n")
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                "Can't start Solver or Mesh creation besides FC file.",
                error_message
            )
        # raise MustSaveError()
        return get_temp_dir()
    else:
        return os.path.splitext(fcstdPath)[0]


def get_custom_base(solver):
    from femsolver.settings import get_custom_dir
    path = get_custom_dir()
    if not os.path.isdir(path):
        error_message = "Selected working directory doesn't exist."
        FreeCAD.Console.PrintError(error_message + "\n")
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                "Can't start Solver or Mesh creation.",
                error_message
            )
        raise DirectoryDoesNotExistError("Invalid path")
    return path


def check_working_dir(wdir):
    # check if working_dir exist, if not use a tmp dir and inform the user
    # print(wdir)
    from os.path import isdir
    if isdir(wdir):
        return True
    else:
        return False


# TODO: move in own error module
class MustSaveError(Exception):
    pass


class DirectoryDoesNotExistError(Exception):
    pass


# ************************************************************************************************
# other
def get_part_to_mesh(mesh_obj):
    """
    gmsh mesh object: the Attribute is Part
    netgen mesh object: the Attribute is Shape
    other mesh objects: do not have a Attribute which holds the part to mesh
    """
    if is_derived_from(mesh_obj, "Fem::FemMeshGmsh"):
        return mesh_obj.Part
    elif is_derived_from(mesh_obj, "Fem::FemMeshShapeNetgenObject"):
        return mesh_obj.Shape
    else:
        return None
    # TODO: the Attributes should be named with the same name
    # should it be Shape or Part?
    # IMHO Part since the Attributes references the document object and not a Shape


def getBoundBoxOfAllDocumentShapes(doc):
    """ Calculate bounding box containing all objects inside *doc*.

    :returns:
     A bounding box containing all objects that have a *Shape* attribute (all
     Part and PartDesign objects). If the document contains no such objects or
     no objects at all return ``None``.
    """
    overalboundbox = None
    for o in doc.Objects:
        # netgen mesh obj has an attribute Shape which is an Document obj, which has no BB
        if hasattr(o, "Shape") and hasattr(o.Shape, "BoundBox"):
            try:
                bb = o.Shape.BoundBox
            except:
                bb = None
            if bb.isValid():
                if not overalboundbox:
                    overalboundbox = bb
                overalboundbox.add(bb)
    return overalboundbox


def getSelectedFace(selectionex):
    """ Return selected face if exactly one face is selected.

    :returns:
     The selected face as a ``Part::TopoShape`` if exactly one face is selected.
     Otherwise return ``None``.

    :param selectionex:
     A list of selection object like the one Gui.Selection.getSelectionEx()
     returns.
    """
    aFace = None
    # print(selectionex)
    if len(selectionex) != 1:
        FreeCAD.Console.PrintMessage("none OR more than one object selected")
    else:
        sel = selectionex[0]
        if len(sel.SubObjects) != 1:
            FreeCAD.Console.PrintMessage("more than one element selected")
        else:
            aFace = sel.SubObjects[0]
            if aFace.ShapeType != "Face":
                FreeCAD.Console.PrintMessage("not a Face selected")
            else:
                FreeCAD.Console.PrintMessage(":-)")
                return aFace
    return aFace


def get_refshape_type(fem_doc_object):
    """ Return shape type the constraints references.

    Determine single shape type of references of *fem_doc_object* which must be
    a constraint (=have a *References* property). All references must be of the
    same type which is than returned as a string. A type can be "Vertex",
    "Edge", "Face" or "Solid".

    :param fem_doc_object:
     A constraint object with a *References* property.

    :returns:
     A string representing the shape type ("Vertex", "Edge", "Face" or
     "Solid"). If *fem_doc_object* isn't a constraint ``""`` is returned.

    :note:
     Undefined behaviour if the type of the references of one object are
     not all the same.

    :note:
     Undefined behaviour if constraint contains no references (empty list).
    """
    import femmesh.meshtools as FemMeshTools
    if hasattr(fem_doc_object, "References") and fem_doc_object.References:
        first_ref_obj = fem_doc_object.References[0]
        first_ref_shape = FemMeshTools.get_element(first_ref_obj[0], first_ref_obj[1][0])
        st = first_ref_shape.ShapeType
        FreeCAD.Console.PrintMessage(
            fem_doc_object.Name + " has " + st + " reference shapes.\n"
        )
        return st
    else:
        FreeCAD.Console.PrintMessage(
            fem_doc_object.Name + " has empty References.\n"
        )
        return ""


def pydecode(bytestring):
    """ Return *bytestring* as a unicode string for python 2 and 3.

    For python 2 *bytestring* is converted to a string of type ``unicode``. For
    python 3 it is returned as is because it uses unicode for it's ``str`` type
    already.
    """
    if sys.version_info.major < 3:
        return bytestring
    else:
        return bytestring.decode("utf-8")
