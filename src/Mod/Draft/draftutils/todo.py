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
        Each tuple is of the form `(name, arg)`.
        The `name` is a reference (pointer) to a function,
        and `arg` is the corresponding argument that is passed
        to that function.
        It then tries executing the function with the argument,
        if available, or without it, if not available.
        ::
            name(arg)
            name()

    commitlist: list of tuples
        Each tuple is of the form `(name, func_or_list)`.
        The `name` is a string identifier, `func_or_list` is a python
        function or a list of strings with python code to be executed.

    afteritinerary: list of tuples
        Each tuple is of the form `(name, arg)`.
        This list is used just like `itinerary`.
    """

    itinerary = []
    commitlist = []
    afteritinerary = []
    timerpending = False

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
        for f, arg in itinerary:
            try:
                if _DEBUG_inner:
                    _msg("Debug: executing.\n"
                         "function: {}\n".format(f))
                if arg or (arg is False):
                    f(arg)
                else:
                    f()
            except Exception:
                try:
                    _log(traceback.format_exc())
                    _err(traceback.format_exc())
                    wrn = ("ToDo.doTasks, Unexpected error:\n"
                           "{0}\n"
                           "in {1}({2})".format(sys.exc_info()[0], f, arg))
                    _wrn(wrn)
                except ReferenceError:
                    _wrn("Debug: ToDo.doTasks: "
                         "queue contains a deleted object, skipping")

        if commitlist:
            for name, func in commitlist:
                if _DEBUG_inner:
                    _msg("Debug: committing.\n"
                         "name: {}\n".format(name))
                try:
                    name = str(name)
                    App.activeDocument().openTransaction(name)
                    if isinstance(func, list):
                        for string in func:
                            Gui.doCommand(string)
                    else:
                        func()
                    App.activeDocument().commitTransaction()
                except Exception:
                    _log(traceback.format_exc())
                    _err(traceback.format_exc())
                    wrn = ("ToDo.doTasks, Unexpected error:\n"
                           "{0}\n"
                           "in {1}".format(sys.exc_info()[0], func))
                    _wrn(wrn)
            # Restack Draft screen widgets after creation
            if hasattr(Gui, "Snapper"):
                Gui.Snapper.restack()

        for f, arg in afteritinerary:
            try:
                if _DEBUG_inner:
                    _msg("Debug: executing after.\n"
                         "function: {}\n".format(f))
                if arg:
                    f(arg)
                else:
                    f()
            except Exception:
                _log(traceback.format_exc())
                _err(traceback.format_exc())
                wrn = ("ToDo.doTasks, Unexpected error:\n"
                       "{0}\n"
                       "in {1}({2})".format(sys.exc_info()[0], f, arg))
                _wrn(wrn)

        if ToDo.itinerary or ToDo.commitlist or ToDo.afteritinerary:
            # New items were queued while processing, run again later
            QtCore.QTimer.singleShot(0, ToDo.doTasks)
        else:
            ToDo.timerpending = False

    @staticmethod
    def delay(f, arg):
        """Add the function and argument to the itinerary list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        Parameters
        ----------
        f: function reference
            A reference (pointer) to a Python command
            which can be executed directly.
            ::
                f()

        arg: argument reference
            A reference (pointer) to the argument to the `f` function.
            ::
                f(arg)
        """
        if _DEBUG:
            _msg("Debug: delaying.\n"
                 "function: {}\n".format(f))
        if not ToDo.timerpending:
            QtCore.QTimer.singleShot(0, ToDo.doTasks)
            ToDo.timerpending = True
        ToDo.itinerary.append((f, arg))

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
        if _DEBUG:
            _msg("Debug: delaying commit.\n"
                 "commitlist: {}\n".format(cl))
        if not ToDo.timerpending:
            QtCore.QTimer.singleShot(0, ToDo.doTasks)
            ToDo.timerpending = True
        ToDo.commitlist = cl

    @staticmethod
    def delayAfter(f, arg):
        """Add the function and argument to the afteritinerary list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        Works the same as `delay`.
        """
        if _DEBUG:
            _msg("Debug: delaying after.\n"
                 "function: {}\n".format(f))
        if not ToDo.timerpending:
            QtCore.QTimer.singleShot(0, ToDo.doTasks)
            ToDo.timerpending = True
        ToDo.afteritinerary.append((f, arg))


# Alias for compatibility with v0.18 and earlier
todo = ToDo

## @}
