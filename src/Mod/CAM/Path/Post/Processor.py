# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
"""
The base classes for post processors in CAM workbench.
"""
from PySide import QtCore, QtGui
from importlib import reload
import FreeCAD
import Path
import Path.Base.Util as PathUtil
import importlib.util
import os
import sys
import re

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class _TempObject:
    Path = None
    Name = "Fixture"
    InList = []
    Label = "Fixture"


class PostProcessorFactory:
    """Factory class for creating post processors."""

    @staticmethod
    def get_post_processor(job, postname):
        # Log initial debug message
        Path.Log.debug("PostProcessorFactory.get_post_processor()")

        # Posts have to be in a place we can find them
        syspath = sys.path
        paths = Path.Preferences.searchPathsPost()
        paths.extend(sys.path)

        module_name = f"{postname}_post"
        class_name = postname.title()

        # Iterate all the paths to find the module
        for path in paths:
            module_path = os.path.join(path, f"{module_name}.py")
            spec = importlib.util.spec_from_file_location(module_name, module_path)

            if spec and spec.loader:
                module = importlib.util.module_from_spec(spec)
                try:
                    spec.loader.exec_module(module)
                    Path.Log.debug(f"found module {module_name} at {module_path}")

                except (FileNotFoundError, ImportError, ModuleNotFoundError):
                    continue

                try:
                    PostClass = getattr(module, class_name)
                    return PostClass(job)
                except AttributeError:
                    # Return an instance of WrapperPost if no valid module is found
                    Path.Log.debug(f"Post processor {postname} is a script")
                    return WrapperPost(job, module_path)


class PostProcessor:
    """Base Class.  All postprocessors should inherit from this class."""

    def __init__(self, job, tooltip, tooltipargs, units, *args, **kwargs):
        self._tooltip = tooltip
        self._tooltipargs = tooltipargs
        self._units = units
        self._job = job
        self._args = args
        self._kwargs = kwargs

    @classmethod
    def exists(cls, processor):
        return processor in Path.Preferences.allAvailablePostProcessors()

    def export(self):
        raise NotImplementedError("Subclass must implement abstract method")

    @property
    def tooltip(self):
        """Get the tooltip text for the post processor."""
        raise NotImplementedError("Subclass must implement abstract method")
        # return self._tooltip

    @property
    def tooltipArgs(self):
        """Get the tooltip arguments for the post processor."""
        raise NotImplementedError("Subclass must implement abstract method")
        # return self._tooltipargs

    @property
    def units(self):
        """Get the units used by the post processor."""
        return self._units

    def _buildPostList(self):
        """
        determines the specific objects and order to
        postprocess  Returns a list of objects which can be passed to
        exportObjectsWith() for final posting."""

        def __fixtureSetup(order, fixture, job):
            """Convert a Fixture setting to _TempObject instance with a G0 move to a
            safe height every time the fixture coordinate system change.  Skip
            the move for first fixture, to avoid moving before tool and tool
            height compensation is enabled."""

            fobj = _TempObject()
            c1 = Path.Command(fixture)
            fobj.Path = Path.Path([c1])
            # Avoid any tool move after G49 in preamble and before tool change
            # and G43 in case tool height compensation is in use, to avoid
            # dangerous move without toolgg compesation.
            if order != 0:
                c2 = Path.Command(
                    "G0 Z"
                    + str(
                        job.Stock.Shape.BoundBox.ZMax + job.SetupSheet.ClearanceHeightOffset.Value
                    )
                )
                fobj.Path.addCommands(c2)
            fobj.InList.append(job)
            return fobj

        wcslist = self._job.Fixtures
        orderby = self._job.OrderOutputBy
        Path.Log.debug(f"Ordering by {orderby}")

        postlist = []

        if orderby == "Fixture":
            Path.Log.debug("Ordering by Fixture")
            # Order by fixture means all operations and tool changes will be
            # completed in one fixture before moving to the next.

            currTool = None
            for index, f in enumerate(wcslist):
                # create an object to serve as the fixture path
                sublist = [__fixtureSetup(index, f, self._job)]

                # Now generate the gcode
                for obj in self._job.Operations.Group:
                    tc = PathUtil.toolControllerForOp(obj)
                    if tc is not None and PathUtil.opProperty(obj, "Active"):
                        if tc.ToolNumber != currTool:
                            sublist.append(tc)
                            Path.Log.debug(f"Appending TC: {tc.Name}")
                            currTool = tc.ToolNumber
                    sublist.append(obj)
                postlist.append((f, sublist))

        elif orderby == "Tool":
            Path.Log.debug("Ordering by Tool")
            # Order by tool means tool changes are minimized.
            # all operations with the current tool are processed in the current
            # fixture before moving to the next fixture.

            toolstring = "None"
            currTool = None

            # Build the fixture list
            fixturelist = []
            for index, f in enumerate(wcslist):
                # create an object to serve as the fixture path
                fixturelist.append(__fixtureSetup(index, f, self._job))

            # Now generate the gcode
            curlist = []  # list of ops for tool, will repeat for each fixture
            sublist = []  # list of ops for output splitting

            Path.Log.track(self._job.PostProcessorOutputFile)
            for idx, obj in enumerate(self._job.Operations.Group):
                Path.Log.track(obj.Label)

                # check if the operation is active
                if not getattr(obj, "Active", True):
                    Path.Log.track()
                    continue

                # Determine the proper string for the Op's TC
                tc = PathUtil.toolControllerForOp(obj)
                if tc is None:
                    tcstring = "None"
                elif "%T" in self._job.PostProcessorOutputFile:
                    tcstring = f"{tc.ToolNumber}"
                else:
                    tcstring = re.sub(r"[^\w\d-]", "_", tc.Label)
                Path.Log.track(toolstring)

                if tc is None or tc.ToolNumber == currTool:
                    curlist.append(obj)
                elif tc.ToolNumber != currTool and currTool is None:  # first TC
                    sublist.append(tc)
                    curlist.append(obj)
                    currTool = tc.ToolNumber
                    toolstring = tcstring

                elif tc.ToolNumber != currTool and currTool is not None:  # TC
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)
                    postlist.append((toolstring, sublist))
                    sublist = [tc]
                    curlist = [obj]
                    currTool = tc.ToolNumber
                    toolstring = tcstring

                if idx == len(self._job.Operations.Group) - 1:  # Last operation.
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)

                    postlist.append((toolstring, sublist))

        elif orderby == "Operation":
            Path.Log.debug("Ordering by Operation")
            # Order by operation means ops are done in each fixture in
            # sequence.
            currTool = None

            # Now generate the gcode
            for obj in self._job.Operations.Group:

                # check if the operation is active
                if not getattr(obj, "Active", True):
                    continue

                sublist = []
                Path.Log.debug(f"obj: {obj.Name}")

                for index, f in enumerate(wcslist):
                    sublist.append(__fixtureSetup(index, f, self._job))
                    tc = PathUtil.toolControllerForOp(obj)
                    if tc is not None:
                        if self._job.SplitOutput or (tc.ToolNumber != currTool):
                            sublist.append(tc)
                            currTool = tc.ToolNumber
                    sublist.append(obj)
                postlist.append((obj.Label, sublist))

        Path.Log.debug(f"Postlist: {postlist}")

        if self._job.SplitOutput:
            Path.Log.track()
            return postlist

        Path.Log.track()
        finalpostlist = [("allitems", [item for slist in postlist for item in slist[1]])]
        Path.Log.debug(f"Postlist: {postlist}")
        return finalpostlist


class WrapperPost(PostProcessor):
    """Wrapper class for old post processors that are scripts."""

    def __init__(self, job, script_path, *args, **kwargs):
        super().__init__(job, tooltip=None, tooltipargs=None, units=None, *args, **kwargs)
        self.script_path = script_path
        Path.Log.debug(f"WrapperPost.__init__({script_path})")
        self.load_script()

    def load_script(self):
        """Dynamically load the script as a module."""
        try:
            spec = importlib.util.spec_from_file_location("script_module", self.script_path)
            self.script_module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(self.script_module)
        except Exception as e:
            raise ImportError(f"Failed to load script: {e}")

        if not hasattr(self.script_module, "export"):
            raise AttributeError("The script does not have an 'export' function.")

        # Set properties based on attributes of the module
        self._units = "Metric" if getattr(self.script_module, "UNITS", "G21") == "G21" else "Inch"
        self._tooltip = getattr(self.script_module, "TOOLTIP", "No tooltip provided")
        self._tooltipargs = getattr(self.script_module, "TOOLTIP_ARGS", [])

    def export(self):
        """Dynamically reload the module for the export to ensure up-to-date usage."""

        postables = self._buildPostList()
        Path.Log.debug(f"postables count: {len(postables)}")

        g_code_sections = []
        for idx, section in enumerate(postables):
            partname, sublist = section

            gcode = self.script_module.export(sublist, "-", self._job.PostProcessorArgs)
            Path.Log.debug(f"Exported {partname}")
            g_code_sections.append((partname, gcode))
        return g_code_sections

    @property
    def tooltip(self):
        return self._tooltip

    @property
    def tooltipArgs(self):
        return self._tooltipargs

    @property
    def units(self):
        return self._units
