# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import FreeCAD as App
import FreeCADGui as Gui

from FreeCAD import DocumentObject

from Draft import make_layer

from ..app.compute_diff import compute_diff, DiffResultPropertyContainer, is_equal_shape
from ..app.report_diff import print_diff
from ..app.util import filenames_to_docs


def copy_object_to_layer(obj: DocumentObject, layer: DocumentObject) -> None:
    doc = App.ActiveDocument
    copy = doc.copyObject(obj)
    copy.Label = obj.Label
    copy.adjustRelativeLinks(layer)
    layer.ViewObject.dropObject(copy)


def add_object_as_link_to_layer(obj: DocumentObject, layer: DocumentObject) -> None:
    doc = App.ActiveDocument
    link = doc.addObject("App::Link", "Link")
    link.ViewObject.OverrideMaterial = True
    link.LinkedObject = obj
    link.Label = obj.Label
    link.adjustRelativeLinks(layer)
    layer.ViewObject.dropObject(link)


def add_objects_as_links_to_layer(objs: set[DocumentObject], layer: DocumentObject) -> None:
    for obj in objs:
        add_object_as_link_to_layer(obj, layer)


def copy_objects_to_layer(objs: set[DocumentObject], layer: DocumentObject) -> None:
    for obj in objs:
        copy_object_to_layer(obj, layer)


def get_objs_same_shape(objs_different: dict[str, DiffResultPropertyContainer],
                        left: App.Document, right: App.Document) -> set[DocumentObject]:
    objs_same_shape = set()
    objs_different_shape = dict[str, DiffResultPropertyContainer]()
    for obj_name, diff_props in objs_different.items():
        obj_left = left.getObject(obj_name)
        obj_right = right.getObject(obj_name)

        if (
                obj_left.isDerivedFrom("Part::Feature")
                and obj_right.isDerivedFrom("Part::Feature")
                and is_equal_shape(obj_left.Shape, obj_right.Shape)
        ):
            objs_same_shape.add(obj_left)
        else:
            objs_different_shape[obj_name] = diff_props
    return objs_same_shape, objs_different_shape


def show_diff(filename_left: str, filename_right: str) -> None:
    left, right = filenames_to_docs(filename_left, filename_right)

    diff = compute_diff(left, right)
    print_diff(left.Label, right.Label, diff)

    diff_doc = App.newDocument("Difference", label="Difference")
    diff_doc.saveAs(f"{App.getTempPath()}Difference.FCStd")
    App.ActiveDocument = diff_doc
    Gui.ActiveDocument = Gui.getDocument(diff_doc.Name)

    def show_objs(objs: set[DocumentObject], name: str, label: str, color: tuple[int, int, int], transparency=50) -> None:
        if len(objs) > 0:
            layer = make_layer(name=name,
                               shape_color=color,
                               transparency=transparency)
            layer.Label = label
            copy_objects_to_layer(objs, layer)

    show_objs(diff.objs_only_in_left, "OnlyInLeft", f"Only in {left.Label}", (175, 99, 99))
    show_objs(diff.objs_only_in_right, "OnlyInRight", f"Only in {right.Label}", (99, 175, 99))

    if len(diff.objs_same) > 0:
        objs_same = set(left.getObject(obj_name) for obj_name in diff.objs_same)
        show_objs(objs_same, "Same", "Same", (255, 255, 99))

    objs_same_shape, objs_different_shapes = get_objs_same_shape(diff.objs_different, left, right)
    show_objs(objs_same_shape, "SameShape", "Same Shape (different properties)", (255, 255, 99))
    # show objects with different shapes in a separate layer
