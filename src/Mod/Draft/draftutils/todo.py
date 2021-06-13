# ***************************************************************************
# *   (c) 2009, Yorik van Havre <yorik@uncreated.net>                       *
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
"""Provides the ToDo static class to run commands with a time delay.

The `ToDo` class is used to delay the commit of commands for later execution.
This is necessary when a GUI command needs to manipulate the 3D view
in such a way that a callback would crash `Coin`.

The `ToDo` class essentially calls `QtCore.QTimer.singleShot`
to execute the instructions stored in internal lists.
"""
## @package todo
# \ingroup draftutils
# \brief Provides the ToDo static class to run commands with a time delay.

import sys
import traceback
import PySide.QtCore as QtCore

import FreeCAD as App
import FreeCADGui as Gui

from draftutils.messages import _msg, _wrn, _err, _log

__title__ = "FreeCAD Draft Workbench, Todo class"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = ["http://www.freecadweb.org"]

_DEBUG = 0
_DEBUG_inner = 0

## \addtogroup draftutils
# @{


class ToDo:
    """A static class that delays execution of functions.

    It calls `QtCore.QTimer.singleShot(0, doTasks)`
    where `doTasks` is a static method which executes
    the commands stored in the list attributes.

    Attributes
    ----------
    itinerary: list of tuples
        Each tuple is of the form `(func, args)`. `func` is a function
        to be called, `args` is an iterable of function argumenst (empty
        for no arguments). Each tuple will be executed in turn as::
            func(*args)

    commitlist: list of tuples
        Each tuple is of the form `(name, func_or_list)`.
        The `name` is a string identifier, `func_or_list` is a python
        function or a list of strings with python code to be executed.

    afteritinerary: list of tuples
        Each tuple is of the form `(func, args)`.
        This list is used just like `itinerary`.
    """

    itinerary = []
    commitlist = []
    afteritinerary = []
    timerpending = False

    @staticmethod
    def _process_list(queue, debug):
        for f, args in queue:
            try:
                if _DEBUG_inner:
                    _msg("Debug: executing {}.\n"
                         "function: {}\n".format(debug, f))
                f(*args)
            except Exception:
                try:
                    _log(traceback.format_exc())
                    _err(traceback.format_exc())
                    wrn = ("ToDo.doTasks, Unexpected error:\n"
                           "{0}\n"
                           "in {1} with args {2}".format(sys.exc_info()[0], f, args))
                    _wrn(wrn)
                except ReferenceError:
                    _wrn("Debug: ToDo.doTasks: "
                         "queue contains a deleted object, skipping")

    @staticmethod
    def doTasks():
        """Execute the commands stored in the lists.

        The lists are `itinerary`, `commitlist` and `afteritinerary`.
        """
        # Work on local versions on the list, so any new items added
        # during processing will be added to new lists to be processed
        # later.
        itinerary, commitlist, afteritinerary = ToDo.itinerary, ToDo.commitlist, ToDo.afteritinerary
        todo.itinerary, todo.commitlist, todo.afteritinerary = [], [], []

        if _DEBUG:
            _msg("Debug: doing delayed tasks.\n"
                 "itinerary: {0}\n"
                 "commitlist: {1}\n"
                 "afteritinerary: {2}\n".format(itinerary,
                                                commitlist,
                                                afteritinerary))

        ToDo._process_list(itinerary, "delayed")

        if commitlist:
            ToDo._process_list(commitlist, "commit")

            # Restack Draft screen widgets after creation
            if hasattr(Gui, "Snapper"):
                Gui.Snapper.restack()

        ToDo._process_list(afteritinerary, "after")

        if ToDo.itinerary or ToDo.commitlist or ToDo.afteritinerary:
            # New items were queued while processing, run again later
            QtCore.QTimer.singleShot(0, ToDo.doTasks)
        else:
            ToDo.timerpending = False

    @staticmethod
    def _add_to_list(queue, debug, f, *args):
        if _DEBUG:
            _msg("Debug: delaying {}.\n"
                 "function: {}\n".format(debug, f))
        if not ToDo.timerpending:
            QtCore.QTimer.singleShot(0, ToDo.doTasks)
            ToDo.timerpending = True
        queue.append((f, args))

    @staticmethod
    def delay(f, *args):
        """Add the function and any number of arguments to the itinerary list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        Parameters
        ----------
        f: function to be executed later
        args: zero or more arguments to be passed to f
        """
        ToDo._add_to_list(ToDo.itinerary, "delayed", f, *args)

    @staticmethod
    def _doCommit(name, func_or_list):
        name = str(name)
        App.activeDocument().openTransaction(name)
        if isinstance(func_or_list, list):
            for cmd in func_or_list:
                Gui.doCommand(cmd)
        else:
            func_or_list()
        App.activeDocument().commitTransaction()

    @staticmethod
    def delayCommit(cl):
        """Execute the other lists, and add to the commit list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        Parameters
        ----------
        cl: list of tuples
            Each tuple is of the form `(name, func_or_list)`.

            The `name` is a string identifier or description of the commands
            that will be run (used as the name of the transaction), and
            `func_or_list` indicates what needs to be executed.

            If `func_or_list` is a list, all items must be strings
            containing python code. The program then opens a transaction,
            runs each string using Gui.doCommand and finally commits the
            transaction.

            If `func_or_list` is a reference to a function
            the function is executed directly (also within a transaction).
        """
        for (name, func_or_list) in cl:
            ToDo._add_to_list(ToDo.commitlist, "commit", ToDo._doCommit, name, func_or_list)

    @staticmethod
    def delayAfter(f, *args):
        """Add the function and argument to the afteritinerary list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        Works the same as `delay`.
        """
        ToDo._add_to_list(ToDo.afteritinerary, "after", f, *args)


# Alias for compatibility with v0.18 and earlier
todo = ToDo

## @}
