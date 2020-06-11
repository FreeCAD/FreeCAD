# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provide the Base object for all Draft Gui commands."""
## @package gui_base
# \ingroup DRAFT
# \brief This module provides the Base object for all Draft Gui commands.

from pivy import coin
import FreeCAD as App
import FreeCADGui as Gui
import Part
import Draft
import Draft_rc  # include resources, icons, ui files
import draftutils.todo as todo
# noinspection PyProtectedMember,PyProtectedMember
from draftutils.messages import _msg, _log
from draftutils.translate import _tr

__metaclass__ = type  # to support Python 2 use of `super()`


class GuiCommandSimplest:
    """Simplest base class for GuiCommands.

    This class only sets up the command name and the document object
    to use for the command.
    When it is executed, it logs the command name to the log file,
    and prints the command name to the console.

    It implements the `IsActive` method, which must return `True`
    when the command should be available.
    It should return `True` when there is an active document,
    otherwise the command (button or menu) should be disabled.

    This class is meant to be inherited by other GuiCommand classes
    to quickly log the command name, and set the correct document object.

    Parameter
    ---------
    name: str, optional
        It defaults to `'None'`.
        The name of the action that is being run,
        for example, `'Heal'`, `'Flip dimensions'`,
        `'Line'`, `'Circle'`, etc.

    doc: App::Document, optional
        It defaults to the value of `App.activeDocument()`.
        The document object itself, which indicates where the actions
        of the command will be executed.

    Attributes
    ----------
    command_name: str
        This is the command name, which is assigned by `name`.

    doc: App::Document
        This is the document object itself, which is assigned by `doc`.

        This attribute should be used by functions to make sure
        that the operations are performed in the correct document
        and not in other documents.
        To set the active document we can use

        >>> App.setActiveDocument(self.doc.Name)
    """

    def __init__(self, name="None", doc=App.activeDocument()):
        self.command_name = name
        self.doc = doc

    def IsActive(self):
        """Return True when this command should be available.

        It is `True` when there is a document.
        """
        if App.activeDocument():
            return True
        else:
            return False

    def Activated(self):
        """Execute when the command is called.

        Log the command name to the log file and console.
        Also update the `doc` attribute.
        """
        self.doc = App.activeDocument()
        _log("Document: {}".format(self.doc.Label))
        _log("GuiCommand: {}".format(self.command_name))
        _msg("{}".format(16 * "-"))
        _msg("GuiCommand: {}".format(self.command_name))


class GuiCommandNeedsSelection(GuiCommandSimplest):
    """Base class for GuiCommands that need a selection to be available.

    It re-implements the `IsActive` method to return `True`
    when there is both an active document and an active selection.

    It inherits `GuiCommandSimplest` to set up the document
    and other behavior. See this class for more information.
    """

    def IsActive(self):
        """Return True when this command should be available.

        It is `True` when there is a selection.
        """
        if App.activeDocument() and Gui.Selection.getSelection():
            return True
        else:
            return False


class GuiCommandBase:
    """Generic class that is the basis of all Gui commands.

    This class should eventually replace `DraftTools.DraftTool`,
    once all functionality in that class is merged here.

    Attributes
    ----------
    commit_list : list of 2-element tuples
        Each tuple is made of a string, and a list of strings.
        ::
            commit_list = [(string1, list1), (string2, list2), ...]

        The string is a simple header, for example, a command name,
        that indicates what is being executed.

        Each string in the list of strings represents a Python instruction
        which will be executed in a delayed fashion
        by `todo.ToDo.delayCommit()`
        ::
            list1 = ["a = FreeCAD.Vector()",
                     "pl = FreeCAD.Placement()",
                     "Draft.autogroup(obj)"]

            commit_list = [("Something", list1)]

        This is used when the 3D view has event callbacks that crash
        Coin3D.
        If this is not needed, those commands could be called in the
        body of the command without problem.
        ::
            >>> a = FreeCAD.Vector()
            >>> pl = FreeCAD.Placement()
            >>> Draft.autogroup(obj)
    """

    def __init__(self):
        self.call = None
        self.commit_list = []
        self.doc = None
        App.activeDraftCommand = None
        self.view = None
        self.planetrack = None

    def IsActive(self):
        """Return True when this command should be available."""
        if App.ActiveDocument:
            return True
        else:
            return False

    def finish(self):
        """Terminate the active command by committing the list of commands.

        It also perform some other tasks like terminating
        the plane tracker and the snapper.
        """
        App.activeDraftCommand = None
        if self.planetrack:
            self.planetrack.finalize()
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.off()
        if self.call:
            try:
                self.view.removeEventCallback("SoEvent", self.call)
            except RuntimeError:
                # the view has been deleted already
                pass
            self.call = None
        if self.commit_list:
            todo.ToDo.delayCommit(self.commit_list)
        self.commit_list = []

    def commit(self, name, func):
        """Store actions to be committed to the document.

        Parameters
        ----------
        name : str
            A string that indicates what is being committed.

        func : list of strings
            Each element of the list should be a Python command
            that will be executed.
        """
        self.commit_list.append((name, func))


class PolarCircularBase(GuiCommandBase):
    """This class is the base of the PolarArray and CircularArray Command to be
    subclassed by them.

    The functionality of this class entails adding and removing the callbacks
    for selecting the AxisReference and the objects to be duplicated.
   """
    def __init__(self):
        super(PolarCircularBase, self).__init__()
        self.location = None
        self.mouse_event = None
        self.view = None
        self.callback_move = None
        self.callback_click = None
        self.ui = None
        self.point = App.Vector()

    def Activated(self):
        """Execute when the command is called.

        We add callbacks that connect the 3D view with
        the widgets of the task panel.
        """
        _log("GuiCommand: {}".format(_tr(self.command_name)))
        _msg("{}".format(16 * "-"))
        _msg("GuiCommand: {}".format(_tr(self.command_name)))

        self.location = coin.SoLocation2Event.getClassTypeId()
        self.mouse_event = coin.SoMouseButtonEvent.getClassTypeId()
        self.view = Draft.get3DView()
        self.add_center_callbacks()

        self.ui.source_command = self
        # Gui.Control.showDialog(self.ui)
        todo.ToDo.delay(Gui.Control.showDialog, self.ui)

    def move(self, event_cb):
        """Execute as a callback when the pointer moves in the 3D view.

        It should automatically update the coordinates in the widgets
        of the task panel.
        """
        event = event_cb.getEvent()
        mousepos = event.getPosition().getValue()
        ctrl = event.wasCtrlDown()
        self.point = Gui.Snapper.snap(mousepos, active=ctrl)
        if self.ui:
            self.ui.display_point(self.point)

    def click(self, event_cb=None):
        """Execute as a callback when the pointer clicks on the 3D view.

        It should act as if the Enter key was pressed, or the OK button
        was pressed in the task panel.
        """
        if event_cb:
            event = event_cb.getEvent()
            if (event.getState() != coin.SoMouseButtonEvent.DOWN
                    or event.getButton() != coin.SoMouseButtonEvent.BUTTON1):
                return
        if self.ui and self.point:
            # The accept function of the interface
            # should call the completed function
            # of the calling class (this one).
            self.ui.accept()

    def add_center_callbacks(self):
        """Execute when the center selection should be enabled"""
        self.callback_move = \
            self.view.addEventCallbackPivy(self.location, self.move)
        self.callback_click = \
            self.view.addEventCallbackPivy(self.mouse_event, self.click)

    def remove_center_callbacks(self):
        """Execute when the center selection should be disabled"""
        if self.callback_move:
            self.view.removeEventCallbackPivy(self.location,
                                              self.callback_move)
            self.callback_move = None

        if self.callback_click:
            self.view.removeEventCallbackPivy(self.mouse_event,
                                              self.callback_click)
            self.callback_click = None

    def add_axis_selection_observer(self):
        """Execute when axis reference selection should be enabled"""
        Gui.Selection.clearSelection(App.ActiveDocument.Name)
        self.axis_observer = AxisSelectionObserver(self)
        Gui.Selection.addObserver(self.axis_observer)

    def remove_axis_selection_observer(self):
        """Execute when axis reference selection should be disabled"""
        if self.axis_observer:
            Gui.Selection.removeObserver(self.axis_observer)
            self.axis_observer = None

    def completed(self):
        """Execute when the command is terminated.

        We should remove the callbacks that were added to the 3D view
        and then close the task panel.
        """
        self.remove_center_callbacks()
        self.remove_axis_selection_observer()
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
            super(PolarCircularBase, self).finish()


# noinspection PyPep8Naming
class AxisSelectionObserver:
    """This classes functions will be called when an selection
    event occurs after axis selection is enabled. This class is used by
    PolarCircularBase.
    """

    def __init__(self, source_command):
        self.source_command = source_command

    def addSelection(self, doc, obj_name, sub_name, pnt):
        """Executed when a new selection is added during AxisReference
        selection process.

        The selection will be cleared and checked if the selected
        object is edge, if so the axis will be displayed in the UI.
        """
        Gui.Selection.clearSelection(App.ActiveDocument.Name)
        selection = Gui.ActiveDocument.getObject(obj_name)
        selection_object = selection.Object
        edge = selection_object.getSubObject(sub_name)
        if isinstance(edge, Part.Edge) and isinstance(edge.Curve, Part.Line):
            self.source_command.ui.display_axis(obj_name, sub_name)
            self.source_command.remove_axis_selection_observer()
        else:
            self.source_command.ui.disable_axis_selection()
            raise TypeError("Selected object is not an edge.")
