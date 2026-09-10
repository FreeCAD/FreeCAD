# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to create Clone objects."""

from __future__ import annotations

## @package make_clone
# \ingroup draftmake
# \brief Provides functions to create Clone objects.

## \addtogroup draftmake
# @{
from typing import Protocol, cast

import FreeCAD as App
from draftobjects.clone import Clone, CloneObject
from draftutils import params
from draftutils import utils
from draftutils import gui_utils

if App.GuiUp:
    from PySide import QtCore
    from draftviewproviders.view_clone import ViewProviderClone


class _CloneSourceProxy(Protocol):
    Type: str


class _CloneSourceObject(Protocol):
    Label: str
    LongName: str
    Placement: App.Placement
    Proxy: _CloneSourceProxy

    def isDerivedFrom(self, identifier: str, /) -> bool: ...


class _ArchCloneObject(Protocol):
    Label: str
    CloneOf: object
    Placement: App.Placement
    RailingLeft: object
    RailingRight: object


def _new_clone_object(doc: App.Document, type_id: str, name: str) -> CloneObject:
    return cast(CloneObject, doc.addObject(type_id, name))


def _make_bim_clone(selected_base: _CloneSourceObject) -> _ArchCloneObject | None:

    try:
        import Arch
    except:
        # BIM not present
        return None

    if utils.get_type(selected_base) == "BuildingPart":
        cl = cast(_ArchCloneObject, Arch.makeComponent())
    else:
        try:  # new-style make function
            cl = cast(
                _ArchCloneObject,
                getattr(Arch, "make_" + selected_base.Proxy.Type.lower())(),
            )
        except Exception:
            try:  # old-style make function
                cl = cast(
                    _ArchCloneObject,
                    getattr(Arch, "make" + selected_base.Proxy.Type)(),
                )
            except Exception:
                return None

    base = cast(
        _CloneSourceObject,
        utils.get_clone_base(cast(App.DocumentObject, selected_base)),
    )
    prefix = cast(str, params.get_param("ClonePrefix"))
    cl.Label = prefix + base.Label
    cl.CloneOf = base
    if utils.get_type(selected_base) != "BuildingPart":
        cl.Placement = selected_base.Placement
    if utils.get_type(selected_base) == "Stairs":
        railing_left = getattr(selected_base, "RailingLeft", None)
        if railing_left:
            cl.RailingLeft = _make_bim_clone(cast(_CloneSourceObject, railing_left))
        railing_right = getattr(selected_base, "RailingRight", None)
        if railing_right:
            cl.RailingRight = _make_bim_clone(cast(_CloneSourceObject, railing_right))

    for prop in ("Description", "IfcType", "Material", "Subvolume", "Tag"):
        try:
            setattr(cl, prop, getattr(base, prop))
        except Exception:
            pass
    if App.GuiUp:
        # Shape of clone may not yet be available (v1.1 regression). See below.
        QtCore.QTimer.singleShot(0, lambda: gui_utils.format_object(cl, base))
    return cl


def make_clone(obj, delta=None, forcedraft=False):
    """clone(obj,[delta,forcedraft])

    Makes a clone of the given object(s).
    The clone is an exact, linked copy of the given object. If the original
    object changes, the final object changes too.

    Parameters
    ----------
    obj :

    delta : Base.Vector
        Delta Vector to move the clone from the original position.

    forcedraft : bool
        If forcedraft is True, the resulting object is a Draft clone
        even if the input object is an Arch object.

    """

    doc = App.ActiveDocument
    if not doc:
        App.Console.PrintError("No active document. Aborting\n")
        return

    prefix_param = params.get_param("ClonePrefix")
    prefix = ""

    if prefix_param:
        prefix = str(prefix_param).strip() + " "

    if not isinstance(obj, list):
        obj = [obj]

    source = cast(_CloneSourceObject, obj[0])
    draft_clone = None

    if (
        len(obj) == 1
        and source.isDerivedFrom("Part::Part2DObject")
        and utils.get_type(source) not in ["BezCurve", "BSpline", "Wire"]
    ):
        # "BezCurve", "BSpline" and "Wire" objects created with < v1.1
        # are "Part::Part2DObject" objects but they need not be 2D.
        draft_clone = _new_clone_object(doc, "Part::Part2DObjectPython", "Clone2D")
        draft_clone.Label = prefix + source.Label + " (2D)"
    elif (
        len(obj) == 1
        and (hasattr(source, "CloneOf") or utils.get_type(source) == "BuildingPart")
        and not forcedraft
    ):
        # arch objects can be clones
        cl = _make_bim_clone(source)
        if cl is not None:
            if App.GuiUp:
                # Delay required in case a stairs with railings is cloned:
                QtCore.QTimer.singleShot(0, lambda: gui_utils.select(cl))
            return cl

    # fall back to Draft clone mode
    if draft_clone is None:
        draft_clone = _new_clone_object(doc, "Part::FeaturePython", "Clone")
        draft_clone.addExtension("Part::AttachExtensionPython")
        draft_clone.Label = prefix + source.Label
    Clone(draft_clone)
    draft_clone.Objects = obj
    if delta:
        draft_clone.Placement.move(delta)
    elif (len(obj) == 1) and hasattr(source, "Placement"):
        draft_clone.Placement = source.Placement
    if hasattr(draft_clone, "LongName") and hasattr(source, "LongName"):
        draft_clone.LongName = source.LongName
    if App.GuiUp:
        ViewProviderClone(draft_clone.ViewObject)
        # Shape of clone may not yet be available (v1.1 regression). We need to delay
        # `format_object()` as that function requires the correct number of faces.
        # https://github.com/FreeCAD/FreeCAD/issues/27958
        QtCore.QTimer.singleShot(0, lambda: gui_utils.format_object(draft_clone, source))
        gui_utils.select(draft_clone)
    return draft_clone


clone = make_clone

## @}
