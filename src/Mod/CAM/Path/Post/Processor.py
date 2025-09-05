# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
The base classes for post processors in the CAM workbench.
"""
import argparse
import importlib.util
import os
from PySide import QtCore, QtGui
import re
import sys
from typing import Any, Dict, List, Optional, Tuple, Union

import Path.Base.Util as PathUtil
import Path.Post.UtilsArguments as PostUtilsArguments
import Path.Post.UtilsExport as PostUtilsExport

import FreeCAD
import Path

translate = FreeCAD.Qt.translate

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


#
# Define some types that are used throughout this file.
#
Defaults = Dict[str, bool]
FormatHelp = str
GCodeOrNone = Optional[str]
GCodeSections = List[Tuple[str, GCodeOrNone]]
Parser = argparse.ArgumentParser
ParserArgs = Union[None, str, argparse.Namespace]
Postables = Union[List, List[Tuple[str, List]]]
Section = Tuple[str, List]
Sublist = List
Units = str
Values = Dict[str, Any]
Visible = Dict[str, bool]


class PostProcessorFactory:
    """Factory class for creating post processors."""

    @staticmethod
    def get_post_processor(job, postname):
        # Log initial debug message
        Path.Log.debug("PostProcessorFactory.get_post_processor()")

        # Posts have to be in a place we can find them
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
                    return WrapperPost(job, module_path, module_name)


class PostProcessor:
    """Base Class.  All non-legacy postprocessors should inherit from this class."""

    def __init__(self, job, tooltip, tooltipargs, units, *args, **kwargs):
        self._tooltip = tooltip
        self._tooltipargs = tooltipargs
        self._units = units
        self._job = job
        self._args = args
        self._kwargs = kwargs
        self.reinitialize()

    @classmethod
    def exists(cls, processor):
        return processor in Path.Preferences.allAvailablePostProcessors()

    @property
    def tooltip(self):
        """Get the tooltip text for the post processor."""
        raise NotImplementedError("Subclass must implement abstract method")
        # return self._tooltip

    @property
    def tooltipArgs(self) -> FormatHelp:
        return self.parser.format_help()

    @property
    def units(self):
        """Get the units used by the post processor."""
        return self._units

    def _buildPostList(self):
        """
        determines the specific objects and order to postprocess
        Returns a list of objects which can be passed to
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
            # dangerous move without tool compensation.
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
                    if tc is not None and PathUtil.activeForOp(obj):
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

            def commitToPostlist():
                if len(curlist) > 0:
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)
                    postlist.append((toolstring, sublist))

            Path.Log.track(self._job.PostProcessorOutputFile)
            for idx, obj in enumerate(self._job.Operations.Group):
                Path.Log.track(obj.Label)

                # check if the operation is active
                if not PathUtil.activeForOp(obj):
                    Path.Log.track()
                    continue

                tc = PathUtil.toolControllerForOp(obj)

                # The operation has no ToolController or uses the same
                # ToolController as the previous operations

                if tc is None or tc.ToolNumber == currTool:
                    # Queue current operation
                    curlist.append(obj)

                # The operation is the first operation or uses a different
                # ToolController as the previous operations

                else:
                    # Commit previous operations
                    commitToPostlist()

                    # Queue current ToolController and operation
                    sublist = [tc]
                    curlist = [obj]
                    currTool = tc.ToolNumber

                    # Determine the proper string for the operation's
                    # ToolController
                    if "%T" in self._job.PostProcessorOutputFile:
                        toolstring = f"{tc.ToolNumber}"
                    else:
                        toolstring = re.sub(r"[^\w\d-]", "_", tc.Label)

            # Commit remaining operations
            commitToPostlist()

        elif orderby == "Operation":
            Path.Log.debug("Ordering by Operation")
            # Order by operation means ops are done in each fixture in
            # sequence.
            currTool = None

            # Now generate the gcode
            for obj in self._job.Operations.Group:

                # check if the operation is active
                if not PathUtil.activeForOp(obj):
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

    def export(self) -> Union[None, GCodeSections]:
        """Process the parser arguments, then postprocess the 'postables'."""
        args: ParserArgs
        flag: bool

        Path.Log.debug("Exporting the job")

        (flag, args) = self.process_arguments()
        #
        # If the flag is True, then continue postprocessing the 'postables'.
        #
        if flag:
            return self.process_postables()
        #
        # The flag is False meaning something unusual happened.
        #
        # If args is None then there was an error during argument processing.
        #
        if args is None:
            return None
        #
        # Otherwise args will contain the argument list formatted for output
        # instead of the "usual" gcode.
        #
        return [("allitems", args)]  # type: ignore

    def init_arguments(
        self,
        values: Values,
        argument_defaults: Defaults,
        arguments_visible: Visible,
    ) -> Parser:
        """Initialize the shared argument definitions."""
        _parser: Parser = PostUtilsArguments.init_shared_arguments(
            values, argument_defaults, arguments_visible
        )
        #
        # Add any argument definitions that are not shared with other postprocessors here.
        #
        return _parser

    def init_argument_defaults(self, argument_defaults: Defaults) -> None:
        """Initialize which arguments (in a pair) are shown as the default argument."""
        PostUtilsArguments.init_argument_defaults(argument_defaults)
        #
        # Modify which argument to show as the default in flag-type arguments here.
        # If the value is True, the first argument will be shown as the default.
        # If the value is False, the second argument will be shown as the default.
        #
        # For example, if you want to show Metric mode as the default, use:
        #   argument_defaults["metric_inch"] = True
        #
        # If you want to show that "Don't pop up editor for writing output" is
        # the default, use:
        #   argument_defaults["show-editor"] = False.
        #
        # Note:  You also need to modify the corresponding entries in the "values" hash
        #        to actually make the default value(s) change to match.
        #

    def init_arguments_visible(self, arguments_visible: Visible) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        PostUtilsArguments.init_arguments_visible(arguments_visible)
        #
        # Modify the visibility of any arguments from the defaults here.
        #

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        PostUtilsArguments.init_shared_values(values)
        #
        # Set any values here that need to override the default values set
        # in the init_shared_values routine.
        #
        values["UNITS"] = self._units

    def process_arguments(self) -> Tuple[bool, ParserArgs]:
        """Process any arguments to the postprocessor."""
        #
        # This function is separated out to make it easier to inherit from this class.
        #
        args: ParserArgs
        flag: bool

        (flag, args) = PostUtilsArguments.process_shared_arguments(
            self.values, self.parser, self._job.PostProcessorArgs, self.all_visible, "-"
        )
        #
        # If the flag is True, then all of the arguments should be processed normally.
        #
        if flag:
            #
            # Process any additional arguments here.
            #
            #
            # Update any variables that might have been modified while processing the arguments.
            #
            self._units = self.values["UNITS"]
        #
        # If the flag is False, then args is either None (indicating an error while
        # processing the arguments) or a string containing the argument list formatted
        # for output.  Either way the calling routine will need to handle the args value.
        #
        return (flag, args)

    def process_postables(self) -> GCodeSections:
        """Postprocess the 'postables' in the job to g code sections."""
        #
        # This function is separated out to make it easier to inherit from this class.
        #
        gcode: GCodeOrNone
        g_code_sections: GCodeSections
        partname: str
        postables: Postables
        section: Section
        sublist: Sublist

        postables = self._buildPostList()

        Path.Log.debug(f"postables count: {len(postables)}")

        g_code_sections = []
        for _, section in enumerate(postables):
            partname, sublist = section
            gcode = PostUtilsExport.export_common(self.values, sublist, "-")
            g_code_sections.append((partname, gcode))

        return g_code_sections

    def reinitialize(self) -> None:
        """Initialize or reinitialize the 'core' data structures for the postprocessor."""
        #
        # This is also used to reinitialize the data structures between tests.
        #
        self.values: Values = {}
        self.init_values(self.values)
        self.argument_defaults: Defaults = {}
        self.init_argument_defaults(self.argument_defaults)
        self.arguments_visible: Visible = {}
        self.init_arguments_visible(self.arguments_visible)
        self.parser: Parser = self.init_arguments(
            self.values, self.argument_defaults, self.arguments_visible
        )
        #
        # Create another parser just to get a list of all possible arguments
        # that may be output using --output_all_arguments.
        #
        self.all_arguments_visible: Visible = {}
        for k in iter(self.arguments_visible):
            self.all_arguments_visible[k] = True
        self.all_visible: Parser = self.init_arguments(
            self.values, self.argument_defaults, self.all_arguments_visible
        )


class WrapperPost(PostProcessor):
    """Wrapper class for old post processors that are scripts."""

    def __init__(self, job, script_path, module_name, *args, **kwargs):
        super().__init__(job, tooltip=None, tooltipargs=None, units=None, *args, **kwargs)
        self.script_path = script_path
        self.module_name = module_name
        Path.Log.debug(f"WrapperPost.__init__({script_path})")
        self.load_script()

    def load_script(self):
        """Dynamically load the script as a module."""
        try:
            spec = importlib.util.spec_from_file_location(self.module_name, self.script_path)
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
