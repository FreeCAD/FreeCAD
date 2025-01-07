# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Library General Public License (LGPL)   *
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

"""Diffing tool for NativeIFC project objects"""


import FreeCAD
from nativeifc import ifc_tools

translate = FreeCAD.Qt.translate


def show_type(obj):
    """Adds the types of that object as FreeCAD objects"""

    element = ifc_tools.get_ifc_element(obj)
    typerel = getattr(element, "IsTypedBy", None)
    project = ifc_tools.get_project(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not all([element, typerel, project, ifcfile]):
        return
    for rel in typerel:
        typelt = rel.RelatingType
        if typelt:
            typeobj = ifc_tools.create_object(typelt, obj.Document, ifcfile)
            ifc_tools.get_group(project, "IfcTypesGroup").addObject(typeobj)
            obj.Type = typeobj


def is_typable(obj):
    """Checks if an object can become a type"""

    element = ifc_tools.get_ifc_element(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not element or not ifcfile:
        return False
    type_class = element.is_a() + "Type"
    schema = ifcfile.wrapped_data.schema_name()
    schema = ifc_tools.ifcopenshell.ifcopenshell_wrapper.schema_by_name(schema)
    try:
        declaration = schema.declaration_by_name(type_class)
    except RuntimeError:
        return False
    return True


def convert_to_type(obj, keep_object=False):
    """Converts an object to a type. If keep_object is
    True, the original object is kept (and adopts the new type)."""

    if not is_typable(obj):
        return
    if not getattr(obj, "Shape", None):
        return
    if FreeCAD.GuiUp:
        import FreeCADGui
        dlg = FreeCADGui.PySideUic.loadUi(":/ui/dialogConvertType.ui")
        result = dlg.exec_()
        if not result:
            return
        keep_object = dlg.checkKeepObject.isChecked()
    element = ifc_tools.get_ifc_element(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    project = ifc_tools.get_project(obj)
    if not element or not ifcfile or not project:
        return
    type_element = ifc_tools.api_run("root.copy_class", ifcfile, product=element)
    type_element = ifc_tools.api_run("root.reassign_class", ifcfile, product=type_element, ifc_class=obj.Class+"Type")
    type_obj = ifc_tools.create_object(type_element, obj.Document, ifcfile)
    if keep_object:
        obj.Type = type_obj
    else:
        ifc_tools.remove_ifc_element(obj, delete_obj=True)
    ifc_tools.get_group(project, "IfcTypesGroup").addObject(type_obj)


def edit_type(obj):
    """Edits the type of this object"""

    element = ifc_tools.get_ifc_element(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not element or not ifcfile:
        return

    typerel = getattr(element, "IsTypedBy", None)
    if obj.Type:
        # verify the type is compatible -ex IFcWall in IfcWallType
        if obj.Type.Class != element.is_a() + "Type":
            t = translate("BIM","Error: Incompatible type")
            FreeCAD.Console.PrintError(obj.Label+": "+t+": "+obj.Type.Class+"\n")
            obj.Type = None
            return
        # change type
        new_type = ifc_tools.get_ifc_element(obj.Type)
        if not new_type:
            return
        for rel in typerel:
            if rel.RelatingType == new_type:
                return
        # assign the new type
        ifc_tools.api_run("type.assign_type",
                          ifcfile,
                          related_objects=[element],
                          relating_type=new_type
        )
    elif typerel:
        # TODO remove type?
        # Not doing anything right now because an unset Type property could screw the ifc file
        pass
