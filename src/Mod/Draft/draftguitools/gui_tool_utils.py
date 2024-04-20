# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides utility functions that are used by many Draft Gui Commands.

These functions are used by different command classes in the `DraftTools`
module. We assume that the graphical interface was already loaded
as they operate on selections and graphical properties.
"""
## @package gui_tool_utils
# \ingroup draftguitools
# \brief Provides utility functions that are used by many Draft Gui Commands.

## \addtogroup draftguitools
# @{
import FreeCAD as App
import FreeCADGui as Gui
import WorkingPlane
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.messages import _wrn

# Set modifier keys from the parameter database
MODS = ["shift", "ctrl", "alt"]


def get_mod_constrain_key():
    return MODS[params.get_param("modconstrain")]


def get_mod_snap_key():
    return MODS[params.get_param("modsnap")]


def get_mod_alt_key():
    return MODS[params.get_param("modalt")]


def format_unit(exp, unit="mm"):
    """Return a formatting string to set a number to the correct unit."""
    return App.Units.Quantity(exp, App.Units.Length).UserString


formatUnit = format_unit


def select_object(arg):
    """Handle the selection of objects depending on buttons pressed.

    This is a scene event handler, to be called from the Draft tools
    when they need to select an object.
    ::
        self.call = self.view.addEventCallback("SoEvent", select_object)

    Parameters
    ----------
    arg: Coin event
        The Coin event received from the 3D view.

        If it is of type Keyboard and the `ESCAPE` key, it runs the `finish`
        method of the active command.

        If Ctrl key is pressed, multiple selection is enabled until the
        button is released.
        Then it runs the `proceed` method of the active command
        to continue with the command's logic.
    """
    if arg["Type"] == "SoKeyboardEvent":
        if arg["Key"] == "ESCAPE":
            App.activeDraftCommand.finish()
            # TODO: this part raises a coin3D warning about scene traversal.
            # It needs to be fixed.
    elif not arg["CtrlDown"] and Gui.Selection.hasSelection():
        App.activeDraftCommand.proceed()


selectObject = select_object


def has_mod(args, mod):
    """Check if args has a specific modifier.

    Parameters
    ----------
    args: Coin event
        The Coin event received from the 3D view.

    mod: str
        A string indicating the modifier, either `'shift'`, `'ctrl'`,
        or `'alt'`.

    Returns
    -------
    bool
        It returns `args["ShiftDown"]`, `args["CtrlDown"]`,
        or `args["AltDown"]`, depending on the passed value of `mod`.
    """
    if mod == "shift":
        return args["ShiftDown"]
    elif mod == "ctrl":
        return args["CtrlDown"]
    elif mod == "alt":
        return args["AltDown"]


hasMod = has_mod


def set_mod(args, mod, state):
    """Set a specific modifier state in args.

    Parameters
    ----------
    args: Coin event
        The Coin event received from the 3D view.

    mod: str
        A string indicating the modifier, either `'shift'`, `'ctrl'`,
        or `'alt'`.

    state: bool
        The boolean value of `state` is assigned to `args["ShiftDown"]`,
        `args["CtrlDown"]`, or `args["AltDown"]`
        depending on `mod`.
    """
    if mod == "shift":
        args["ShiftDown"] = state
    elif mod == "ctrl":
        args["CtrlDown"] = state
    elif mod == "alt":
        args["AltDown"] = state


setMod = set_mod


def get_point(target, args, noTracker=False):
    """Return a constrained 3D point and its original point.

    It is used by the Draft tools.

    Parameters
    ----------
    target: object (class)
        The target object with a `node` attribute. If this is present,
        return the last node, otherwise return `None`.

        In the Draft tools, `target` is essentially the same class
        of the Gui command, that is, `self`. Therefore, this method
        probably makes more sense as a class method.

    args: Coin event
        The Coin event received from the 3D view.

    noTracker: bool, optional
        It defaults to `False`.
        If it is `True`, the tracking line will not be displayed.

    Returns
    -------
    CoinPoint, Base::Vector3, str
        It returns a tuple with some information.

        The first is the Coin point returned by `Snapper.snap`
        or by the `ActiveView`; the second is that same point
        turned into an `App.Vector`,
        and the third is some information of the point
        returned by the `Snapper` or by the `ActiveView`.
    """
    ui = Gui.draftToolBar
    if not ui.mouse:
        return None, None, None

    if target.node:
        last = target.node[-1]
    else:
        last = None

    smod = has_mod(args, get_mod_snap_key())
    cmod = has_mod(args, get_mod_constrain_key())
    point = None

    if hasattr(Gui, "Snapper"):
        point = Gui.Snapper.snap(args["Position"],
                                 lastpoint=last,
                                 active=smod,
                                 constrain=cmod,
                                 noTracker=noTracker)
        info = Gui.Snapper.snapInfo
        mask = Gui.Snapper.affinity
    if not point:
        p = Gui.ActiveDocument.ActiveView.getCursorPos()
        point = Gui.ActiveDocument.ActiveView.getPoint(p)
        info = Gui.ActiveDocument.ActiveView.getObjectInfo(p)
        mask = None

    ctrlPoint = App.Vector(point)
    wp = WorkingPlane.get_working_plane(update=False)
    if target.node:
        if target.featureName == "Rectangle":
            ui.displayPoint(point, target.node[0], plane=wp, mask=mask)
        else:
            ui.displayPoint(point, target.node[-1], plane=wp, mask=mask)
    else:
        ui.displayPoint(point, plane=wp, mask=mask)
    return point, ctrlPoint, info


getPoint = get_point


def set_working_plane_to_object_under_cursor(mouseEvent):
    """Align the working plane to the face under the cursor.

    The working plane is only aligned if it is `'auto'`.

    Parameters
    ----------
    mouseEvent: Coin event
        Coin mouse event.

    Returns
    -------
    App::DocumentObject or None
        The parent object the face belongs to, if alignment occurred, or None.
    """
    objectUnderCursor = gui_utils.get_3d_view().getObjectInfo((
        mouseEvent["Position"][0],
        mouseEvent["Position"][1]))

    if not objectUnderCursor:
        return None
    if "Face" not in objectUnderCursor["Component"]:
        return None
    wp = WorkingPlane.get_working_plane(update=False)
    if not wp.auto:
        return None

    import Part
    if "ParentObject" in objectUnderCursor:
        obj = objectUnderCursor["ParentObject"]
        sub = objectUnderCursor["SubName"]
    else:
        obj = App.ActiveDocument.getObject(objectUnderCursor["Object"])
        sub = objectUnderCursor["Component"]
    shape = Part.getShape(obj, sub, needSubElement=True, retType=0)

    if wp.align_to_face(shape, _hist_add=False):
        wp.auto = True
        return obj

    return None


setWorkingPlaneToObjectUnderCursor = set_working_plane_to_object_under_cursor


def set_working_plane_to_selected_object():
    """Align the working plane to a preselected face.

    The working plane is only aligned if it is `'auto'`.

    Returns
    -------
    App::DocumentObject or None
        The parent object the face belongs to, if alignment occurred, or None.
    """
    wp = WorkingPlane.get_working_plane(update=False)
    if not wp.auto:
        return None

    sels = Gui.Selection.getSelectionEx("", 0)

    if len(sels) == 1 \
            and len(sels[0].SubObjects) == 1 \
            and sels[0].SubObjects[0].ShapeType == "Face":
        import Part
        shape = Part.getShape(sels[0].Object,
                              sels[0].SubElementNames[0],
                              needSubElement=True,
                              retType=0)

        if wp.align_to_face(shape, _hist_add=False):
            wp.auto = True
            return sels[0].Object

    return None


setWorkingPlaneToSelectedObject = set_working_plane_to_selected_object


def get_support(mouseEvent=None):
    """"Align the working plane to a preselected face or the face under the cursor.

    The working plane is only aligned if it is `'auto'`.

    Parameters
    ----------
    mouseEvent: Coin event, optional
        Defaults to `None`.
        Coin mouse event.

    Returns
    -------
    App::DocumentObject or None
        The parent object the face belongs to, if alignment occurred, or None.
    """
    if mouseEvent is None:
        return set_working_plane_to_selected_object()
    return set_working_plane_to_object_under_cursor(mouseEvent)


getSupport = get_support


def redraw_3d_view():
    """Force a redraw of 3D view or do nothing if it fails."""
    try:
        Gui.ActiveDocument.ActiveView.redraw()
    except AttributeError as err:
        _wrn(err)


redraw3DView = redraw_3d_view

## @}
