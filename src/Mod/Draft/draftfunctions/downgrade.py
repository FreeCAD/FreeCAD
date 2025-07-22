# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 The FreeCAD Project Association                    *
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
"""Provides functions to downgrade objects by different methods.

See also the `upgrade` function.
"""
## @package downgrade
# \ingroup draftfunctions
# \brief Provides functions to downgrade objects by different methods.

## \addtogroup draftfunctions
# @{
import FreeCAD as App
from draftmake import make_copy
from draftutils import utils
from draftutils import params
from draftutils import gui_utils
from draftutils.groups import is_group
from draftutils.messages import _msg
from draftutils.translate import translate


def downgrade(objects, delete=False, force=None):
    """Downgrade the given objects.

    This is a counterpart to `upgrade`.

    Parameters
    ----------
    objects: Part::Feature or list
        A single object to downgrade or a list
        containing various such objects.

    delete: bool, optional
        It defaults to `False`.
        If it is `True`, the old objects are deleted, and only the resulting
        object is kept.

    force: str, optional
        It defaults to `None`.
        Its value can be used to force a certain method of downgrading.
        It can be any of: `'explode'`, `'shapify'`, `'subtr'`, `'splitFaces'`,
        `'cut2'`, `'getWire'`, `'splitWires'`, or `'splitCompounds'`.

    Returns
    -------
    tuple
        A tuple containing two lists, a list of new objects
        and a list of objects to be deleted.

    See Also
    --------
    upgrade
    """

    # definitions of actions to perform

    def explode(obj):
        """Explode a Draft block or array."""
        obj_pl = obj.Placement
        # block:
        if getattr(obj, "Components", []):
            delete_list.append(obj)
            for comp in obj.Components:
                # Objects in Components are in the same parent group as the block.
                comp.Placement = obj_pl.multiply(comp.Placement)
                comp.Visibility = True
            return True
        # array:
        if getattr(obj, "Base", None) is None:
            return False
        if not hasattr(obj, "PlacementList"):
            return False
        # Array must be added to delete_list before base. See can_be_deleted.
        delete_list.append(obj)
        base = obj.Base
        if not base.Visibility:
            # Delete base if it is not visible. The can_be_deleted
            # function will check if it is not used elsewhere.
            delete_list.append(base)
        new_list = []
        if getattr(obj, "ExpandArray", False):
            delete_list.extend(obj.ElementList)
            for lnk in obj.ElementList:
                newobj = make_copy.make_copy(base)
                newobj.Placement = obj_pl.multiply(lnk.Placement)
                newobj.Visibility = True
                new_list.append(newobj)
        else:
            for arr_pl in obj.PlacementList:
                newobj = make_copy.make_copy(base)
                newobj.Placement = obj_pl.multiply(arr_pl)
                newobj.Visibility = True
                new_list.append(newobj)
        add_to_parent(obj, new_list)
        add_list.extend(new_list)
        return True

    def cut2(objects):
        """Cut 2nd object from 1st."""
        newobj = doc.addObject("Part::Cut", "Cut")
        newobj.Base = objects[0]
        newobj.Tool = objects[1]
        format(objects[0], [newobj])
        add_to_parent(objects[0], [newobj])
        add_list.append(newobj)
        return True

    def splitCompounds(objects):
        """Split solids contained in compound objects into new objects."""
        result = False
        for obj in objects:
            if not obj.Shape.Solids:
                continue
            new_list = []
            for solid in obj.Shape.Solids:
                newobj = doc.addObject("Part::Feature", "Solid")
                newobj.Shape = solid
                new_list.append(newobj)
            format(obj, new_list)
            add_to_parent(obj, new_list)
            add_list.extend(new_list)
            delete_list.append(obj)
            result = True
        return result

    def splitFaces(objects):
        """Split faces contained in objects into new objects."""
        result = False
        preserveFaceColor = params.get_param("preserveFaceColor")
        preserveFaceNames = params.get_param("preserveFaceNames")
        for obj in objects:
            if not obj.Shape.Faces:
                continue
            new_list = []
            if App.GuiUp and preserveFaceColor and obj.ViewObject:
                colors = obj.ViewObject.DiffuseColor
            else:
                colors = None
            label = getattr(obj, "Label", "")
            for ind, face in enumerate(obj.Shape.Faces):
                newobj = doc.addObject("Part::Feature", "Face")
                newobj.Shape = face
                format(obj, [newobj])
                if App.GuiUp and preserveFaceColor and colors:
                    # At this point, some single-color objects might have just
                    # a single value in colors for all faces, so we handle that:
                    if ind < len(colors):
                        color = colors[ind]
                    else:
                        color = colors[0]
                    newobj.ViewObject.ShapeColor = color
                if preserveFaceNames:
                    newobj.Label = "{} {}".format(label, newobj.Label)
                new_list.append(newobj)
            add_to_parent(obj, new_list)
            add_list.extend(new_list)
            delete_list.append(obj)
            result = True
        return result

    def subtr(objects):
        """Subtract faces from the first one."""
        faces = []
        done_list = []
        for obj in objects:
            if obj.Shape.Faces:
                faces.extend(obj.Shape.Faces)
                done_list.append(obj)
        if not faces:
            return False
        main_face = faces.pop(0)
        for face in faces:
            main_face = main_face.cut(face)
        if main_face.isNull():
            return False
        newobj = doc.addObject("Part::Feature", "Subtraction")
        newobj.Shape = main_face
        format(done_list[0], [newobj])
        add_to_parent(done_list[0], [newobj])
        add_list.append(newobj)
        delete_list.extend(done_list)
        return True

    def getWire(obj):
        """Get the wire from a face object."""
        if not obj.Shape.Faces:
            return False
        new_list = []
        for wire in obj.Shape.Faces[0].Wires:
            newobj = doc.addObject("Part::Feature", "Wire")
            newobj.Shape = wire
            new_list.append(newobj)
        format(obj, new_list)
        add_to_parent(obj, new_list)
        add_list.extend(new_list)
        delete_list.append(obj)
        return True

    def splitWires(objects):
        """Split the wires contained in objects into edges."""
        result = False
        for obj in objects:
            if not obj.Shape.Edges:
                continue
            new_list = []
            for edge in obj.Shape.Edges:
                newobj = doc.addObject("Part::Feature", "Edge")
                newobj.Shape = edge
                new_list.append(newobj)
            format(obj, new_list)
            add_to_parent(obj, new_list)
            add_list.extend(new_list)
            delete_list.append(obj)
            result = True
        return result

    def _shapify(obj):
        """Wrapper for utils.shapify."""
        newobj = utils.shapify(obj, delete=False)
        if newobj:
            format(obj, [newobj])
            add_to_parent(obj, [newobj])
            add_list.append(newobj)
            delete_list.append(obj)
            return True
        return False


    # helper functions (same as in upgrade.py)

    def get_parent(obj):
        # Problem with obj.getParent():
        # https://github.com/FreeCAD/FreeCAD/issues/19600
        parent = obj.getParentGroup()
        if parent is not None:
            return parent
        return obj.getParentGeoFeatureGroup()

    def can_be_deleted(obj):
        if not obj.InList:
            return True
        for other in obj.InList:
            if is_group(other):
                continue
            if other.TypeId == "App::Part":
                continue
            return False
        return True

    def delete_object(obj):
        if utils.is_deleted(obj):
            return
        parent = get_parent(obj)
        if parent is not None and parent.TypeId == "PartDesign::Body":
            obj = parent
        if not can_be_deleted(obj):
            # Make obj invisible instead:
            obj.Visibility = False
            return
        if obj.TypeId == "PartDesign::Body":
            obj.removeObjectsFromDocument()
        doc.removeObject(obj.Name)

    def add_to_parent(obj, new_list):
        parent = get_parent(obj)
        if parent is None:
            if doc.getObject("Draft_Construction"):
                # This cludge is required because the make_* commands may
                # put new objects in the construction group.
                constr_group = doc.getObject("Draft_Construction")
                for newobj in new_list:
                    constr_group.removeObject(newobj)
            return
        if parent.TypeId == "PartDesign::Body":
            # We don't add to a PD Body. We process its placement and
            # add to its parent instead.
            for newobj in new_list:
                newobj.Placement = parent.Placement.multiply(newobj.Placement)
            add_to_parent(parent, new_list)
            return
        for newobj in new_list:
            # Using addObject is different from just changing the Group property.
            # With addObject the object will be added to the parent group, but if
            # that is a normal group, also to that group's parent GeoFeatureGroup,
            # if available.
            parent.addObject(newobj)

    def format(obj, new_list):
        for newobj in new_list:
            gui_utils.format_object(newobj, obj, ignore_construction=True)


    doc = App.ActiveDocument
    add_list = []
    delete_list = []
    result = False

    if not isinstance(objects, list):
        objects = [objects]
    if not objects:
        return add_list, delete_list

    # analyzing objects
    solids = []
    faces = []
    edges = []
    onlyedges = True
    parts = []

    for obj in objects:
        if hasattr(obj, "Shape"):
            for solid in obj.Shape.Solids:
                solids.append(False)
            for face in obj.Shape.Faces:
                faces.append(face)
            for edge in obj.Shape.Edges:
                edges.append(edge)
            if obj.Shape.ShapeType != "Edge":
                onlyedges = False
            parts.append(obj)
    objects = parts

    if not objects:
        result = False

    elif force:
        # functions that work on a single object:
        single_funcs = {"explode": explode,
                        "getWire": getWire,
                        "shapify": _shapify}
        # functions that work on multiple objects:
        multi_funcs = {"cut2": cut2,
                       "splitCompounds": splitCompounds,
                       "splitFaces": splitFaces,
                       "splitWires": splitWires,
                       "subtr": subtr}
        if force in single_funcs:
            result = any([single_funcs[force](obj) for obj in objects])
        elif force in multi_funcs:
            result = multi_funcs[force](objects)
        else:
            _msg(translate("draft", "Downgrade: Unknown force method:") + " " + force)
            result = False

    else:
        parent = get_parent(objects[0])
        same_parent = True
        same_parent_type = getattr(parent, "TypeId", "")  # "" for global space.
        if len(objects) > 1:
            for obj in objects[1:]:
                if get_parent(obj) != parent:
                    same_parent = False
                    same_parent_type = None
                    break

        # we have a block, we explode it
        if len(objects) == 1 and utils.get_type(objects[0]) == "Block":
            result = explode(objects[0])
            if result:
                _msg(translate("draft", "Found 1 block: exploding it"))

        # we have an array, we explode it
        elif len(objects) == 1 and "Array" in utils.get_type(objects[0]):
            result = explode(objects[0])
            if result:
                _msg(translate("draft", "Found 1 array: exploding it"))

        # special case, we have one parametric object: we "de-parametrize" it
        elif len(objects) == 1 \
                and hasattr(objects[0], "Shape") \
                and (
                    hasattr(objects[0], "Base")
                    or hasattr(objects[0], "Profile")
                    or hasattr(objects[0], "Sections")
                ):
            result = _shapify(objects[0])
            if result:
                _msg(translate("draft", "Found 1 parametric object: breaking its dependencies"))

        # we have one multi-solids compound object: extract its solids
        elif len(objects) == 1 \
                and hasattr(objects[0], "Shape") \
                and len(solids) > 1:
            result = splitCompounds(objects)
            if result:
                _msg(translate("draft", "Found 1 multi-solids compound: exploding it"))

        # we have only 2 objects: cut 2nd from 1st
        elif len(objects) == 2 and same_parent and same_parent_type != "PartDesign::Body":
            result = cut2(objects)
            if result:
                _msg(translate("draft", "Found 2 objects: subtracting them"))

        elif len(faces) > 1:
            # one object with several faces: split it
            if len(objects) == 1:
                result = splitFaces(objects)
                if result:
                    _msg(translate("draft", "Found several faces: splitting them"))
            # several objects: remove all the faces from the first one
            elif same_parent and same_parent_type != "PartDesign::Body":
                result = subtr(objects)
                if result:
                    _msg(translate("draft", "Found several faces: subtracting them from the first one"))

        # only one face: we extract its wires
        elif len(faces) > 0:
            result = getWire(objects[0])
            if result:
                _msg(translate("draft", "Found 1 face: extracting its wires"))

        # no faces: split wire into single edges
        elif not onlyedges:
            result = splitWires(objects)
            if result:
                _msg(translate("draft", "Found only wires: extracting their edges"))

    # no result has been obtained
    if not result:
        _msg(translate("draft", "Unable to downgrade these objects"))

    if delete:
        for obj in delete_list:
            delete_object(obj)

    gui_utils.select(add_list)
    return add_list, delete_list

## @}
