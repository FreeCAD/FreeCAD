# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftfunctions.cut as cut

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

    None
        If there is a problem it will return `None`.

    See Also
    --------
    upgrade
    """
    _name = "downgrade"
    utils.print_header(_name, "Downgrade objects")

    if not isinstance(objects, list):
        objects = [objects]

    delete_list = []
    add_list = []
    doc = App.ActiveDocument

    # actions definitions
    def explode(obj):
        """Explode a Draft block."""
        pl = obj.Placement
        for o in obj.Components:
            o.Placement = pl.multiply(o.Placement)
            if App.GuiUp:
                o.ViewObject.Visibility = True
        delete_list.append(obj)
        return True

    def cut2(objects):
        """Cut first object from the last one."""
        newobj = cut.cut(objects[0], objects[1])
        if newobj:
            add_list.append(newobj)
            return newobj
        return None

    def splitCompounds(objects):
        """Split solids contained in compound objects into new objects."""
        result = False
        for o in objects:
            if o.Shape.Solids:
                for s in o.Shape.Solids:
                    newobj = doc.addObject("Part::Feature", "Solid")
                    newobj.Shape = s
                    add_list.append(newobj)
                result = True
                delete_list.append(o)
        return result

    def splitFaces(objects):
        """Split faces contained in objects into new objects."""
        result = False
        params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor")  # True
        preserveFaceNames = params.GetBool("preserveFaceNames")  # True
        for o in objects:
            if App.GuiUp and preserveFaceColor and o.ViewObject:
                voDColors = o.ViewObject.DiffuseColor
            else:
                voDColors = None
            oLabel = o.Label if hasattr(o, 'Label') else ""
            if o.Shape.Faces:
                for ind, f in enumerate(o.Shape.Faces):
                    newobj = doc.addObject("Part::Feature", "Face")
                    newobj.Shape = f
                    if preserveFaceNames:
                        newobj.Label = "{} {}".format(oLabel, newobj.Label)
                    if App.GuiUp and preserveFaceColor and voDColors:
                        # At this point, some single-color objects might have
                        # just a single value in voDColors for all faces,
                        # so we handle that
                        if ind < len(voDColors):
                            tcolor = voDColors[ind]
                        else:
                            tcolor = voDColors[0]
                        # does is not applied visually on its own
                        # just in case
                        newobj.ViewObject.DiffuseColor[0] = tcolor
                        # this gets applied, works by itself too
                        newobj.ViewObject.ShapeColor = tcolor
                    add_list.append(newobj)
                result = True
                delete_list.append(o)
        return result

    def subtr(objects):
        """Subtract objects from the first one."""
        faces = []
        for o in objects:
            if o.Shape.Faces:
                faces.extend(o.Shape.Faces)
                delete_list.append(o)
        u = faces.pop(0)
        for f in faces:
            u = u.cut(f)
        if not u.isNull():
            newobj = doc.addObject("Part::Feature", "Subtraction")
            newobj.Shape = u
            add_list.append(newobj)
            return newobj
        return None

    def getWire(obj):
        """Get the wire from a face object."""
        result = False
        for w in obj.Shape.Faces[0].Wires:
            newobj = doc.addObject("Part::Feature", "Wire")
            newobj.Shape = w
            add_list.append(newobj)
            result = True
        delete_list.append(obj)
        return result

    def splitWires(objects):
        """Split the wires contained in objects into edges."""
        result = False
        for o in objects:
            if o.Shape.Edges:
                for e in o.Shape.Edges:
                    newobj = doc.addObject("Part::Feature", "Edge")
                    newobj.Shape = e
                    add_list.append(newobj)
                delete_list.append(o)
                result = True
        return result

    def delete_object(obj):
        if obj.FullName == "?": # Already deleted.
            return
        # special case: obj is a body or belongs to a body:
        if obj.TypeId == "PartDesign::Body":
            obj.removeObjectsFromDocument()
        if hasattr(obj, "_Body") and obj._Body is not None:
            obj = obj._Body
            obj.removeObjectsFromDocument()
        else:
            for parent in obj.InList:
                if parent.TypeId == "PartDesign::Body" \
                        and obj in parent.Group:
                    obj = parent
                    obj.removeObjectsFromDocument()
                    break
        doc.removeObject(obj.Name)

    # analyzing objects
    faces = []
    edges = []
    onlyedges = True
    parts = []
    solids = []
    result = None

    for o in objects:
        if hasattr(o, 'Shape'):
            for s in o.Shape.Solids:
                solids.append(s)
            for f in o.Shape.Faces:
                faces.append(f)
            for e in o.Shape.Edges:
                edges.append(e)
            if o.Shape.ShapeType != "Edge":
                onlyedges = False
            parts.append(o)
    objects = parts

    if force:
        if force in ("explode", "shapify", "subtr", "splitFaces",
                     "cut2", "getWire", "splitWires"):
            # TODO: Using eval to evaluate a string is not ideal
            # and potentially a security risk.
            # How do we execute the function without calling eval?
            # Best case, a series of if-then statements.
            shapify = utils.shapify
            result = eval(force)(objects)
        else:
            _msg(translate("draft","Upgrade: Unknown force method:") + " " + force)
            result = None
    else:
        # applying transformation automatically
        # we have a block, we explode it
        if len(objects) == 1 and utils.get_type(objects[0]) == "Block":
            result = explode(objects[0])
            if result:
                _msg(translate("draft","Found 1 block: exploding it"))

        # we have one multi-solids compound object: extract its solids
        elif len(objects) == 1 \
                and hasattr(objects[0], "Shape") \
                and len(solids) > 1:
            result = splitCompounds(objects)
            # print(result)
            if result:
                _msg(translate("draft","Found 1 multi-solids compound: exploding it"))

        # special case, we have one parametric object: we "de-parametrize" it
        elif len(objects) == 1 \
                and hasattr(objects[0], "Shape") \
                and hasattr(objects[0], "Base") \
                and not objects[0].isDerivedFrom("PartDesign::Feature"):
            result = utils.shapify(objects[0])
            if result:
                _msg(translate("draft","Found 1 parametric object: breaking its dependencies"))
                add_list.append(result)
                # delete_list.append(objects[0])

        # we have only 2 objects: cut 2nd from 1st
        elif len(objects) == 2:
            result = cut2(objects)
            if result:
                _msg(translate("draft","Found 2 objects: subtracting them"))

        elif len(faces) > 1:
            # one object with several faces: split it
            if len(objects) == 1:
                result = splitFaces(objects)
                if result:
                    _msg(translate("draft","Found several faces: splitting them"))
            # several objects: remove all the faces from the first one
            else:
                result = subtr(objects)
                if result:
                    _msg(translate("draft","Found several objects: subtracting them from the first one"))

        # only one face: we extract its wires
        elif len(faces) > 0:
            result = getWire(objects[0])
            if result:
                _msg(translate("draft","Found 1 face: extracting its wires"))

        # no faces: split wire into single edges
        elif not onlyedges:
            result = splitWires(objects)
            if result:
                _msg(translate("draft","Found only wires: extracting their edges"))

        # no result has been obtained
        if not result:
            _msg(translate("draft","No more downgrade possible"))

    if delete:
        for o in delete_list:
            delete_object(o)
        delete_list = []

    gui_utils.select(add_list)
    return add_list, delete_list

## @}
