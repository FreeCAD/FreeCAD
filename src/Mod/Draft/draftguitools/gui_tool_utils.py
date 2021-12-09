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
import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftutils.messages import _wrn

# Set modifier keys from the parameter database
MODS = ["shift", "ctrl", "alt"]
MODCONSTRAIN = MODS[utils.get_param("modconstrain", 0)]
MODSNAP = MODS[utils.get_param("modsnap", 1)]
MODALT = MODS[utils.get_param("modalt", 2)]


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


def get_point(target, args,
              mobile=False, sym=False, workingplane=True, noTracker=False):
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

    mobile: bool, optional
        It defaults to `False`.
        If it is `True` the constraining occurs from the location of
        the mouse cursor when `Shift` is pressed; otherwise from the last
        entered point.

    sym: bool, optional
        It defaults to `False`.
        If it is `True`, the x and y values always stay equal.

    workingplane: bool, optional
        It defaults to `True`.
        If it is `False`, the point won't be projected on the currently
        active working plane.

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

    if target.node:
        last = target.node[-1]
    else:
        last = None

    amod = hasMod(args, MODSNAP)
    cmod = hasMod(args, MODCONSTRAIN)
    point = None

    if hasattr(Gui, "Snapper"):
        point = Gui.Snapper.snap(args["Position"],
                                 lastpoint=last,
                                 active=amod,
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
    if target.node:
        if target.featureName == "Rectangle":
            ui.displayPoint(point, target.node[0],
                            plane=App.DraftWorkingPlane, mask=mask)
        else:
            ui.displayPoint(point, target.node[-1],
                            plane=App.DraftWorkingPlane, mask=mask)
    else:
        ui.displayPoint(point, plane=App.DraftWorkingPlane, mask=mask)
    return point, ctrlPoint, info


getPoint = get_point


def set_working_plane_to_object_under_cursor(mouseEvent):
    """Set the working plane to the object under the cursor.

    It tests for an object under the cursor.
    If it is found, it checks whether a `'face'` or `'curve'` component
    is selected in the object's `Shape`.
    Then it tries to align the working plane to that face or curve.

    The working plane is only aligned to the face if
    the working plane is not `'weak'`.

    Parameters
    ----------
    mouseEvent: Coin event
        Coin event with the mouse, that is, a click.

    Returns
    -------
    None
        If no object was found in the 3D view under the cursor.
        Or if the working plane is not `weak`.
        Or if there was an exception with aligning the working plane
        to the component under the cursor.

    Coin info
        The `getObjectInfo` of the object under the cursor.
    """
    objectUnderCursor = gui_utils.get_3d_view().getObjectInfo((
        mouseEvent["Position"][0],
        mouseEvent["Position"][1]))

    if not objectUnderCursor:
        return None

    try:
        # Get the component "face" or "curve" under the "Shape"
        # of the selected object
        componentUnderCursor = getattr(
            App.ActiveDocument.getObject(objectUnderCursor['Object']).Shape,
            objectUnderCursor["Component"])

        if not App.DraftWorkingPlane.weak:
            return None

        if "Face" in objectUnderCursor["Component"]:
            App.DraftWorkingPlane.alignToFace(componentUnderCursor)
        else:
            App.DraftWorkingPlane.alignToCurve(componentUnderCursor)
        App.DraftWorkingPlane.weak = True
        return objectUnderCursor
    except Exception:
        pass

    return None


setWorkingPlaneToObjectUnderCursor = set_working_plane_to_object_under_cursor


def set_working_plane_to_selected_object():
    """Set the working plane to the selected object's face.

    The working plane is only aligned to the face if
    the working plane is `'weak'`.

    Returns
    -------
    None
        If more than one object was selected.
        Or if the selected object has many subelements.
        Or if the single subelement is not a `'Face'`.

    App::DocumentObject
        The single object that contains a single selected face.
    """
    sel = Gui.Selection.getSelectionEx()
    if len(sel) != 1:
        return None
    sel = sel[0]
    if (sel.HasSubObjects
            and len(sel.SubElementNames) == 1
            and "Face" in sel.SubElementNames[0]):
        if App.DraftWorkingPlane.weak:
            App.DraftWorkingPlane.alignToFace(sel.SubObjects[0])
            App.DraftWorkingPlane.weak = True
        return sel.Object
    return None


setWorkingPlaneToSelectedObject = set_working_plane_to_selected_object


def get_support(mouseEvent=None):
    """Return the supporting object and set the working plane.

    It saves the current working plane, then sets it to the selected object.

    Parameters
    ----------
    mouseEvent: Coin event, optional
        It defaults to `None`.
        Coin event with the mouse, that is, a click.

        If there is a mouse event it calls
        `set_working_plane_to_object_under_cursor`.
        Otherwise, it calls `set_working_plane_to_selected_object`.

    Returns
    -------
    None
        If there was a mouse event, but there was nothing under the cursor.
        Or if the working plane is not `weak`.
        Or if there was an exception with aligning the working plane
        to the component under the cursor.
        Or if more than one object was selected.
        Or if the selected object has many subelements.
        Or if the single subelement is not a `'Face'`.

    Coin info
        If there was a mouse event, the `getObjectInfo`
        of the object under the cursor.

    App::DocumentObject
        If there was no mouse event, the single selected object
        that contains the single selected face that was used
        to align the working plane.
    """
    App.DraftWorkingPlane.save()
    if mouseEvent:
        return setWorkingPlaneToObjectUnderCursor(mouseEvent)
    return setWorkingPlaneToSelectedObject()


getSupport = get_support


def redraw_3d_view():
    """Force a redraw of 3D view or do nothing if it fails."""
    try:
        Gui.ActiveDocument.ActiveView.redraw()
    except AttributeError as err:
        _wrn(err)


redraw3DView = redraw_3d_view

## @}
