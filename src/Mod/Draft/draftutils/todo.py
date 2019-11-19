"""This module provides the ToDo class for the Draft Workbench.

This module provides the ToDo class to delay the commit of commands,
which depends on QtCore.QTimer.
"""
## @package todo
# \ingroup DRAFT
# \brief This module provides the ToDo class for the Draft Workbench.

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

import sys
import six
import traceback
import FreeCAD
import FreeCADGui
from PySide import QtCore


__title__ = "FreeCAD Draft Workbench, Todo class"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = ["http://www.freecadweb.org"]


class ToDo:
    """A static class that delays execution of functions.

    It calls `QtCore.QTimer.singleShot(0, doTasks)`
    where `doTasks` is a static method which executes
    the commands stored in the list attributes.

    Attributes
    ----------
    itinerary : list of tuples
        Each tuple is of the form `(name, arg)`.
        The `name` is a reference (pointer) to a function,
        and `arg` is the corresponding argument that is passed
        to that function.
        It then tries executing the function with the argument,
        if available, or without it, if not available.
        ::
            name(arg)
            name()

    commitlist : list of tuples
        Each tuple is of the form `(name, command_list)`.
        The `name` is a string identifier or description of the commands
        that will be run, and `command_list` is a list of strings
        that indicate the Python instructions that will be executed,
        or a reference to a single function that will be executed.

        If `command_list` is a list, the program opens a transaction,
        then runs all commands in the list in sequence,
        and finally commits the transaction.
        ::
            command_list = ["command1", "command2", "..."]
            FreeCAD.ActiveDocument.openTransaction(name)
            FreeCADGui.doCommand("command1")
            FreeCADGui.doCommand("command2")
            FreeCADGui.doCommand("...")
            FreeCAD.ActiveDocument.commitTransaction()

        If `command_list` is a reference to a function
        the function is executed directly.
        ::
            command_list = function
            FreeCAD.ActiveDocument.openTransaction(name)
            function()
            FreeCAD.ActiveDocument.commitTransaction()

    afteritinerary : list of tuples
        Each tuple is of the form `(name, arg)`.
        This list is used just like `itinerary`.

    Lists
    -----
    The lists contain tuples. Each tuple contains a `name` which is just
    a string to identify the operation, and a `command_list` which is
    a list of strings, each string an individual Python instruction.
    """
    itinerary = []
    commitlist = []
    afteritinerary = []

    @staticmethod
    def doTasks():
        """Execute the commands stored in the lists.

        The lists are `itinerary`, `commitlist` and `afteritinerary`.
        """
        print("Debug: doing delayed tasks.\n"
              "itinerary: {0}\n"
              "commitlist: {1}\n"
              "afteritinerary: {2}\n".format(todo.itinerary,
                                             todo.commitlist,
                                             todo.afteritinerary))
        try:
            for f, arg in todo.itinerary:
                try:
                    # print("debug: executing", f)
                    if arg or (arg is False):
                        f(arg)
                    else:
                        f()
                except Exception:
                    FreeCAD.Console.PrintLog(traceback.format_exc())
                    wrn = ("ToDo.doTasks, Unexpected error:\n"
                           "{0}\n"
                           "in {1}({2})".format(sys.exc_info()[0], f, arg))
                    FreeCAD.Console.PrintWarning(wrn)
        except ReferenceError:
            print("Debug: ToDo.doTasks: "
                  "queue contains a deleted object, skipping")
        todo.itinerary = []

        if todo.commitlist:
            for name, func in todo.commitlist:
                if six.PY2:
                    if isinstance(name, six.text_type):
                        name = name.encode("utf8")
                # print("debug: committing " + str(name))
                try:
                    name = str(name)
                    FreeCAD.ActiveDocument.openTransaction(name)
                    if isinstance(func, list):
                        for string in func:
                            FreeCADGui.doCommand(string)
                    else:
                        func()
                    FreeCAD.ActiveDocument.commitTransaction()
                except Exception:
                    FreeCAD.Console.PrintLog(traceback.format_exc())
                    wrn = ("ToDo.doTasks, Unexpected error:\n"
                           "{0}\n"
                           "in {1}".format(sys.exec_info()[0], func))
                    FreeCAD.Console.PrintWarning(wrn)
            # Restack Draft screen widgets after creation
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.restack()
        todo.commitlist = []

        for f, arg in todo.afteritinerary:
            try:
                # print("debug: executing", f)
                if arg:
                    f(arg)
                else:
                    f()
            except Exception:
                FreeCAD.Console.PrintLog(traceback.format_exc())
                wrn = ("ToDo.doTasks, Unexpected error:\n"
                       "{0}\n"
                       "in {1}({2})".format(sys.exc_info()[0], f, arg))
                FreeCAD.Console.PrintWarning(wrn)
        todo.afteritinerary = []

    @staticmethod
    def delay(f, arg):
        """Add the function and argument to the itinerary list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        If the `itinerary` list is empty, it will call
        `QtCore.QTimer.singleShot(0, ToDo.doTasks)`
        to execute the commands in the other lists.

        Finally, it will build the tuple `(f, arg)`
        and append it to the `itinerary` list.

        Parameters
        ----------
        f : function reference
            A reference (pointer) to a Python command
            which can be executed directly.
            ::
                f()

        arg : argument reference
            A reference (pointer) to the argument to the `f` function.
            ::
                f(arg)
        """
        # print("debug: delaying", f)
        if todo.itinerary == []:
            QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.itinerary.append((f, arg))

    @staticmethod
    def delayCommit(cl):
        """Execute the other lists, and add to the commit list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        First it calls
        `QtCore.QTimer.singleShot(0, ToDo.doTasks)`
        to execute the commands in all lists.

        Then the `cl` list is assigned as the new commit list.

        Parameters
        ----------
        cl : list of tuples
            Each tuple is of the form `(name, command_list)`.
            The `name` is a string identifier or description of the commands
            that will be run, and `command_list` is a list of strings
            that indicate the Python instructions that will be executed.

            See the attributes of the `ToDo` class for more information.
        """
        # print("debug: delaying commit", cl)
        QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.commitlist = cl

    @staticmethod
    def delayAfter(f, arg):
        """Add the function and argument to the afteritinerary list.

        Schedule geometry manipulation that would crash Coin if done
        in the event callback.

        Works the same as `delay`.

        If the `afteritinerary` list is empty, it will call
        `QtCore.QTimer.singleShot(0, ToDo.doTasks)`
        to execute the commands in the other lists.

        Finally, it will build the tuple `(f, arg)`
        and append it to the `afteritinerary` list.
        """
        # print("debug: delaying", f)
        if todo.afteritinerary == []:
            QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.afteritinerary.append((f, arg))


todo = ToDo
