# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
# *   Copyright (c) 2018 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


__title__ = "FEM Utilities"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import sys
import FreeCAD
import FreeCAD as App


def createObject(doc, name, proxy, viewProxy):
    obj = doc.addObject(proxy.BaseType, name)
    proxy(obj)
    if App.GuiUp:
        viewProxy(obj.ViewObject)
    return obj


def findAnalysisOfMember(member):
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
    if analysis is None:
        raise ValueError("Analysis must not be None")
    matching = []
    for m in analysis.Group:
        if is_derived_from(m, t):  # since is _derived_from is used the father could be used to test too (ex. 'Fem::FemMeshObject')
            matching.append(m)
    return matching


def get_single_member(analysis, t):
    objs = get_member(analysis, t)
    return objs[0] if objs else None


# collect analysis members used in CalculiX and Z88
def get_several_member(analysis, t):
    # if no member is found, an empty list is returned
    objs = get_member(analysis, t)
    members = []
    for m in objs:
        obj_dict = {}
        obj_dict['Object'] = m
        obj_dict['RefShapeType'] = get_refshape_type(m)
        members.append(obj_dict)
    return members


# typeID and object type defs
def type_of_obj(obj):
    '''returns objects TypeId (C++ objects) or Proxy.Type (Python objects)'''
    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type
    return obj.TypeId


def is_of_type(obj, ty):
    '''returns True if an object is of a given TypeId (C++ objects) or Proxy.Type (Python Features)'''
    # only returns true if the exact TypeId is given.
    # For FeaturPythons the Proxy.Type has to be given. Keep in mind the TypeId for them is the TypeId from the C++ father class
    return type_of_obj(obj) == ty


def is_derived_from(obj, t):
    '''returns True if an object or its inheritance chain is of a given TypeId (C++ objects) or Proxy.Type (Python objects)'''
    # returns true for all FEM objects if given t == 'App::DocumentObject' since this is a father of the given object
    # see https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    if (hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type") and obj.Proxy.Type == t):
        return True
    return obj.isDerivedFrom(t)


def getBoundBoxOfAllDocumentShapes(doc):
    overalboundbox = None
    for o in doc.Objects:
        # netgen mesh obj has an attribute Shape which is an Document obj, which has no BB
        if hasattr(o, 'Shape') and hasattr(o.Shape, 'BoundBox'):
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
    aFace = None
    # print(selectionex)
    if len(selectionex) != 1:
        FreeCAD.Console.PrintMessage('none OR more than one object selected')
    else:
        sel = selectionex[0]
        if len(sel.SubObjects) != 1:
            FreeCAD.Console.PrintMessage('more than one element selected')
        else:
            aFace = sel.SubObjects[0]
            if aFace.ShapeType != 'Face':
                FreeCAD.Console.PrintMessage('not a Face selected')
            else:
                FreeCAD.Console.PrintMessage(':-)')
                return aFace
    return aFace


def get_refshape_type(fem_doc_object):
    # returns the reference shape type
    # for force object:
    # in GUI defined frc_obj all frc_obj have at least one ref_shape and ref_shape have all the same shape type
    # for material object:
    # in GUI defined material_obj could have no RefShape and RefShapes could be different type
    # we're going to need the RefShapes to be the same type inside one fem_doc_object
    # TODO: check if all RefShapes inside the object really have the same type
    import femmesh.meshtools as FemMeshTools
    if hasattr(fem_doc_object, 'References') and fem_doc_object.References:
        first_ref_obj = fem_doc_object.References[0]
        first_ref_shape = FemMeshTools.get_element(first_ref_obj[0], first_ref_obj[1][0])
        st = first_ref_shape.ShapeType
        FreeCAD.Console.PrintMessage(fem_doc_object.Name + ' has ' + st + ' reference shapes.\n')
        return st
    else:
        FreeCAD.Console.PrintMessage(fem_doc_object.Name + ' has empty References.\n')
        return ''


def pydecode(bytestring):
    if sys.version_info.major < 3:
        return bytestring
    else:
        return bytestring.decode("utf-8")
