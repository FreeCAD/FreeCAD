# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

This module contains function for extracting relevant parts of geometry and
a few unrelated function useful at various places in the Fem module.
"""

__title__ = "FEM Utilities"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

import os
import subprocess
from platform import system

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui


# ************************************************************************************************
# document objects
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
    """ Check if *obj* is derived from *t* honoring Fems typesystem.

    Essentially just call ``obj.isDerivedFrom(t)`` and return it's value. For
    objects using Fems typesystem (see :py:func:`type_of_obj`) return always
    True if the Fem type is equal to *t*.

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecad.org/viewtopic.php?f=10&t=32625
    """
    if (hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type") and obj.Proxy.Type == t):
        return True
    return obj.isDerivedFrom(t)


# ************************************************************************************************
# working dir
def get_pref_working_dir(solver_obj):
    """ Return working directory for solver honoring user settings.

    :throws femtools.errors.MustSaveError:
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
# see commit a9c19ca6d42c for more information
# the FEM preferences will be used by both
def get_temp_dir(obj=None):
    from tempfile import mkdtemp
    return mkdtemp(prefix="fcfem_")


def get_beside_dir(obj):
    base = get_beside_base(obj)
    specific_path = os.path.join(base, obj.Label)
    if not os.path.isdir(specific_path):
        make_dir(specific_path)
    return specific_path


def get_custom_dir(obj):
    base = get_custom_base(obj)
    specific_path = os.path.join(
        base, obj.Document.Name, obj.Label)
    if not os.path.isdir(specific_path):
        make_dir(specific_path)
    return specific_path


def get_beside_base(obj):
    fcstdPath = obj.Document.FileName
    if fcstdPath == "":
        new_path = get_temp_dir()
        error_message = (
            "Please save the file before executing a solver or creating a mesh. "
            "This must be done because the location of the working directory "
            "is set to \"Beside *.FCStd File\". For the moment the tmp dir {} is used."
            .format(new_path)
        )
        FreeCAD.Console.PrintError("{}\n".format(error_message))
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                "Can't start Solver or Mesh creation besides FC file.",
                error_message
            )

        # from .errors import MustSaveError
        # raise MustSaveError()
        return new_path
    else:
        return os.path.splitext(fcstdPath)[0]


def get_custom_base(solver):
    from femsolver.settings import get_custom_dir
    path = get_custom_dir()
    if not os.path.isdir(path):
        new_path = get_temp_dir()
        error_message = (
            "Selected working directory {} doesn't exist. "
            " For the moment the tmp dir {} is used."
            .format(path, new_path)
        )
        FreeCAD.Console.PrintError("{}\n".format(error_message))
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                "Can't start Solver or Mesh creation.",
                error_message
            )
        # from .errors import DirectoryDoesNotExistError
        # raise DirectoryDoesNotExistError("Invalid path")
        return new_path
    return path


def check_working_dir(wdir):
    # check if working_dir exist, if not use a tmp dir and inform the user
    # print(wdir)
    from os.path import isdir
    if isdir(wdir):
        return True
    else:
        return False


def make_dir(specific_path):
    try:
        os.makedirs(specific_path)
    except OSError:
        new_path = get_temp_dir()
        # it could fail for various reasons, full disk, etc
        # beside dir fails on installed FC examples from start wb
        error_message = (
            "Failed to create the directory {}. "
            " For the moment the tmp dir {} is used."
            .format(specific_path, new_path)
        )
        FreeCAD.Console.PrintError("{}\n".format(error_message))
        return new_path
    return specific_path


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
    overallboundbox = None
    # netgen mesh obj has an attribute Shape which is an Document obj, which has no BB
    # a FemMesh without a Shape could be clipped too
    # https://forum.freecad.org/viewtopic.php?f=18&t=52920
    for o in doc.Objects:

        FreeCAD.Console.PrintMessage(":\n")  # debug only
        bb = None

        try:
            FreeCAD.Console.PrintMessage(
                "trying: {}: getPropertyOfGeometry()\n".format(o.Label)
            )  # debug only
            bb = o.getPropertyOfGeometry().BoundBox
            FreeCAD.Console.PrintMessage("{}\n".format(bb))  # debug only
        except Exception:
            FreeCAD.Console.PrintMessage("exception \n")  # debug only

        if bb is None:
            try:
                FreeCAD.Console.PrintMessage("trying: {}: FemMesh\n".format(o.Label))  # debug only
                bb = o.FemMesh.BoundBox
                FreeCAD.Console.PrintMessage("{}\n".format(bb))  # debug only
            except Exception:
                FreeCAD.Console.PrintMessage("exception \n")  # debug only

        if bb:
            if bb.isValid():
                if not overallboundbox:
                    overallboundbox = bb
                else:
                    overallboundbox.add(bb)
        else:                                                                   # debug only
            FreeCAD.Console.PrintMessage("no bb\n")                             # debug only

    FreeCAD.Console.PrintMessage("overallBB:" + str(overallboundbox) + "\n")    # debug only
    return overallboundbox


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
        FreeCAD.Console.PrintMessage("No or more than one object selected.\n")
    else:
        sel = selectionex[0]
        if len(sel.SubObjects) != 1:
            FreeCAD.Console.PrintMessage("More than one element selected.\n")
        else:
            aFace = sel.SubObjects[0]
            if aFace.ShapeType != "Face":
                FreeCAD.Console.PrintMessage("Not a Face selected.\n")
            else:
                FreeCAD.Console.PrintMessage(":-)\n")
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
    from femtools.geomtools import get_element
    if hasattr(fem_doc_object, "References") and fem_doc_object.References:
        first_ref_obj = fem_doc_object.References[0]
        first_ref_shape = get_element(first_ref_obj[0], first_ref_obj[1][0])
        st = first_ref_shape.ShapeType
        FreeCAD.Console.PrintLog(
            "References: {} in {}, {}\n"
            . format(st, fem_doc_object.Name, fem_doc_object.Label)
        )
        return st
    else:
        FreeCAD.Console.PrintLog(
            "References: empty in {}, {}\n"
            . format(fem_doc_object.Name, fem_doc_object.Label)
        )
        return ""


def pydecode(bytestring):
    """ Return *bytestring* as a unicode string """
    return bytestring.decode("utf-8")


def startProgramInfo(code):
    """ starts a program under Windows minimized, hidden or normal """
    if system() == "Windows":
        info = subprocess.STARTUPINFO()
        if code == "hide":
            SW_HIDE = 0
            info.wShowWindow = SW_HIDE
        elif code == "minimize":
            SW_MINIMIZE = 6
            info.wShowWindow = SW_MINIMIZE
        elif code == "normal":
            SW_DEFAULT = 10
            info.wShowWindow = SW_DEFAULT
        info.dwFlags = subprocess.STARTF_USESHOWWINDOW
        return info


def expandParentObject():
    """ expands parent and selected obj in tree view """
    trees = FreeCADGui.getMainWindow().findChildren(QtGui.QTreeWidget)
    for tree in trees:
        items = tree.selectedItems()
        if items == []:
            continue
        for item in items:
            tree.expandItem(item)
