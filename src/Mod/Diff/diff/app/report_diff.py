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

from FreeCAD import DocumentObject

from .compute_diff import DiffResultPropertyContainer, compute_diff
from .util import filenames_to_docs as filenames_to_docs


def print_obj_names(objs: set[DocumentObject], msg: str) -> None:
    print(f"  Objects {msg}:")
    for obj in objs:
        print(f"    {obj.Name}")


def print_prop_names(props: set[str], msg: str, indent=2) -> None:
    indent_str = " " * indent
    print(f"{indent_str}Properties {msg}:")
    for prop in props:
        print(f"{indent_str}  {prop}")


def print_diff_property_container(type: str, left: str, right: str,
                                  diff: DiffResultPropertyContainer,
                                  indent=2) -> None:
    indent_str = " " * indent
    if len(diff.props_only_in_left) > 0:
        print_prop_names(diff.props_only_in_left, f"only in {type} {left}", indent)
    if len(diff.props_only_in_right) > 0:
        print_prop_names(diff.props_only_in_right, f"only in {type} {right}", indent)

    if len(diff.props_different) > 0:
        print(f"{indent_str}Properties that are different:")
        for prop_name, (value_left, value_right) in diff.props_different.items():
            print(f"{indent_str}  {prop_name}:")
            print(f"{indent_str}    {type} {left}: {value_left}")
            print(f"{indent_str}    {type} {right}: {value_right}")


def print_diff(left: str, right: str, diff: DiffResultPropertyContainer) -> None:
    print(f"Difference between {left} and {right}:")
    if not diff.props.is_same():
        print_diff_property_container("document", left, right, diff.props)

    if len(diff.objs_only_in_left) > 0:
        print_obj_names(diff.objs_only_in_left, f"only in {left}")
    if len(diff.objs_only_in_right) > 0:
        print_obj_names(diff.objs_only_in_right, f"only in {right}")

    if len(diff.objs_different) > 0:
        print("  Objects that are different:")
        for obj_name, obj_diff in diff.objs_different.items():
            print(f"    Object {obj_name}:")
            print_diff_property_container("object in", left, right, obj_diff, indent=6)


def report_diff(filename_left: str, filename_right: str) -> None:
    left, right = filenames_to_docs(filename_left, filename_right)
    diff = compute_diff(left, right)

    print_diff(left.Label, right.Label, diff)

