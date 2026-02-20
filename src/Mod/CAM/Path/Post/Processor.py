# SPDX-License-Identifier: LGPL-2.1-or-later

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
import CONSTANTS

import Path.Base.Util as PathUtil
import Path.Post.UtilsArguments as PostUtilsArguments
import Path.Post.UtilsExport as PostUtilsExport
import Path.Post.PostList as PostList

import FreeCAD
import Path

import Path.Post.Utils as PostUtils
from Machine.models.machine import (
    MachineFactory,
)

translate = FreeCAD.Qt.translate

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class _HeaderBuilder:
    """Builder class for constructing G-code header with structured data storage."""

    def __init__(self):
        """Initialize the header builder with empty data structures."""
        self._exporter = None
        self._machine = None
        self._post_processor = None
        self._cam_file = None
        self._project_file = None
        self._output_units = None
        self._document_name = None
        self._description = None
        self._author = None
        self._output_time = None
        self._tools = []  # List of (tool_number, tool_name) tuples
        self._fixtures = []  # List of fixture names
        self._notes = []  # list of notes

    def add_exporter_info(self, exporter: str = "FreeCAD"):
        """Add exporter information to the header."""
        self._exporter = exporter

    def add_machine_info(self, machine: str):
        """Add machine information to the header."""
        self._machine = machine

    def add_post_processor(self, name: str):
        """Add post processor name to the header."""
        self._post_processor = name

    def add_cam_file(self, filename: str):
        """Add CAM file information to the header."""
        self._cam_file = filename

    def add_project_file(self, filename: str):
        """Add project file information to the header."""
        self._project_file = filename

    def add_output_units(self, units: str):
        """Add output units information to the header."""
        self._output_units = units

    def add_document_name(self, name: str):
        """Add document name to the header."""
        self._document_name = name

    def add_description(self, description: str):
        """Add description to the header."""
        self._description = description

    def add_author(self, author: str):
        """Add author information to the header."""
        self._author = author

    def add_output_time(self, timestamp: str):
        """Add output timestamp to the header."""
        self._output_time = timestamp

    def add_tool(self, tool_number: int, tool_name: str):
        """Add a tool to the header."""
        self._tools.append((tool_number, tool_name))

    def add_fixture(self, fixture_name: str):
        """Add a fixture to the header."""
        self._fixtures.append(fixture_name)

    def add_note(self, note: str):
        """Add a note to the header."""
        self._notes.append(note)

    @property
    def Path(self) -> Path.Path:
        """Return a Path.Path containing Path.Commands as G-code comments for the header."""
        commands = []

        # Add exporter info
        if self._exporter:
            commands.append(Path.Command(f"(Exported by {self._exporter})"))

        # Add machine info
        if self._machine:
            commands.append(Path.Command(f"(Machine: {self._machine})"))

        # Add post processor info
        if self._post_processor:
            commands.append(Path.Command(f"(Post Processor: {self._post_processor})"))

        # Add CAM file info
        if self._cam_file:
            commands.append(Path.Command(f"(Cam File: {self._cam_file})"))

        # Add project file info
        if self._project_file:
            commands.append(Path.Command(f"(Project File: {self._project_file})"))

        # Add output units info
        if self._output_units:
            commands.append(Path.Command(f"(Output Units: {self._output_units})"))

        # Add document name
        if self._document_name:
            sanitized = self._document_name.replace("(", "[").replace(")", "]")
            commands.append(Path.Command(f"(Document: {sanitized})"))

        # Add description
        if self._description:
            sanitized = self._description.replace("(", "[").replace(")", "]")
            commands.append(Path.Command(f"(Description: {sanitized})"))

        # Add author info
        if self._author:
            sanitized = self._author.replace("(", "[").replace(")", "]")
            commands.append(Path.Command(f"(Author: {sanitized})"))

        # Add output time
        if self._output_time:
            commands.append(Path.Command(f"(Output Time: {self._output_time})"))

        # Add tools
        for tool_number, tool_name in self._tools:
            # Sanitize tool name to prevent nested parentheses from breaking G-code comments
            sanitized_name = tool_name.replace("(", "[").replace(")", "]")
            commands.append(Path.Command(f"(T{tool_number}={sanitized_name})"))

        # Add fixtures (if needed in header)
        for fixture in self._fixtures:
            sanitized = fixture.replace("(", "[").replace(")", "]")
            commands.append(Path.Command(f"(Fixture: {sanitized})"))

        # Add notes
        for note in self._notes:
            sanitized = note.replace("(", "[").replace(")", "]")
            commands.append(Path.Command(f"(Note: {sanitized})"))

        return Path.Path(commands)


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
        Path.Log.debug(f"PostProcessorFactory.get_post_processor() - postname: {postname}")
        Path.Log.debug(f"PostProcessorFactory.get_post_processor() - module_name: {module_name}")
        Path.Log.debug(f"PostProcessorFactory.get_post_processor() - class_name: {class_name}")

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
                    Path.Log.debug(f"Found class {class_name} in module {module_name}")
                    return PostClass(job)
                except AttributeError:
                    # Return an instance of WrapperPost if no valid module is found
                    Path.Log.debug(f"Post processor {postname} is a script")
                    return WrapperPost(job, module_path, module_name)
                except Exception as e:
                    # Log any other exception during instantiation
                    Path.Log.debug(f"Error instantiating {class_name}: {e}")
                    # If job is None (filtering context), try to return the class itself
                    # so the machine editor can check its schema methods
                    if job is None:
                        try:
                            PostClass = getattr(module, class_name)
                            Path.Log.debug(
                                f"Returning uninstantiated class {class_name} for schema inspection"
                            )
                            # Return a mock instance that can be used for schema inspection
                            return PostClass.__new__(PostClass)
                        except:
                            pass
                    raise

        return None


def needsTcOp(oldTc, newTc):
    return PostList.needsTcOp(oldTc, newTc)


class PostProcessor:
    """Base Class.  All non-legacy postprocessors should inherit from this class."""

    @classmethod
    def get_common_property_schema(cls) -> List[Dict[str, Any]]:
        """
        Return schema for common properties that all postprocessors should expose.

        These are properties that apply to all postprocessors and define fundamental
        capabilities or behaviors. Subclasses can override this to change defaults
        but should generally include all common properties.

        Returns:
            List of common property schema dictionaries.
        """
        # Use centralized command lists from CONSTANTS
        all_supported_commands = (
            CONSTANTS.GCODE_SUPPORTED + CONSTANTS.MCODE_SUPPORTED + CONSTANTS.GCODE_NON_CONFORMING
        )

        return [
            {
                "name": "file_extension",
                "type": "string",
                "label": translate("CAM", "File Extension"),
                "default": "nc",
                "help": translate(
                    "CAM",
                    "Default file extension for output files (without the dot). "
                    "Common extensions: nc, gcode, tap, ngc, sbp, etc.",
                ),
            },
            {
                "name": "supports_tool_radius_compensation",
                "type": "bool",
                "label": translate("CAM", "Tool Radius Compensation (G41/G42)"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Enable if this postprocessor supports G41/G42 tool radius compensation commands. "
                    "When enabled, the postprocessor can output cutter compensation codes.",
                ),
            },
            {
                "name": "supported_commands",
                "type": "text",
                "label": translate("CAM", "Supported G-code Commands"),
                "default": "\n".join(all_supported_commands),
                "help": translate(
                    "CAM",
                    "List of G-code commands supported by this postprocessor (one per line). "
                    "Commands not in this list will be filtered out or cause warnings.",
                ),
            },
            {
                "name": "drill_cycles_to_translate",
                "type": "text",
                "label": translate("CAM", "Drill Cycles to Translate"),
                "default": "\n".join(CONSTANTS.GCODE_MOVE_DRILL),
                "help": translate(
                    "CAM",
                    "List of drill cycle commands to translate to G0/G1 moves (one per line). "
                    f"Standard drill cycles: {', '.join(CONSTANTS.GCODE_MOVE_DRILL)}. "
                    "Leave empty if postprocessor supports drill cycles natively.",
                ),
            },
            {
                "name": "preamble",
                "type": "text",
                "label": translate("CAM", "Preamble"),
                "default": "",
                "help": translate(
                    "CAM", "G-code commands inserted at the start of the program after the header."
                ),
            },
            {
                "name": "postamble",
                "type": "text",
                "label": translate("CAM", "Postamble"),
                "default": "",
                "help": translate("CAM", "G-code commands inserted at the end of the program."),
            },
            {
                "name": "safetyblock",
                "type": "text",
                "label": translate("CAM", "Safety Block"),
                "default": "",
                "help": translate(
                    "CAM",
                    "Safety commands to reset machine to known safe condition (e.g., G40, G49, G80).",
                ),
            },
            {
                "name": "pre_operation",
                "type": "text",
                "label": translate("CAM", "Pre-Operation"),
                "default": "",
                "help": translate("CAM", "G-code commands inserted before each operation."),
            },
            {
                "name": "post_operation",
                "type": "text",
                "label": translate("CAM", "Post-Operation"),
                "default": "",
                "help": translate("CAM", "G-code commands inserted after each operation."),
            },
            {
                "name": "pre_tool_change",
                "type": "text",
                "label": translate("CAM", "Pre-Tool Change"),
                "default": "",
                "help": translate("CAM", "G-code commands inserted before tool changes."),
            },
            {
                "name": "post_tool_change",
                "type": "text",
                "label": translate("CAM", "Post-Tool Change"),
                "default": "",
                "help": translate("CAM", "G-code commands inserted after tool changes."),
            },
        ]

    @classmethod
    def get_property_schema(cls) -> List[Dict[str, Any]]:
        """
        Return schema describing configurable properties for this postprocessor.

        This method should be overridden by subclasses to add postprocessor-specific
        properties. The common properties from get_common_property_schema() are
        automatically included.

        Each property is a dictionary with the following keys:
        - name: str - Internal property name (used as key in machine.postprocessor_properties)
        - type: str - Property type: 'bool', 'int', 'float', 'str', 'text', 'choice', 'file'
        - label: str - Human-readable label for the UI
        - default: Any - Default value for the property
        - help: str - Help text describing the property
        - Additional type-specific keys:
          - For 'int'/'float': min, max, decimals (float only)
          - For 'choice': choices (list of valid values)
          - For 'file': filters (file type filters)

        Returns:
            List of property schema dictionaries. Empty list means no configurable properties.
        """
        return []

    @classmethod
    def get_full_property_schema(cls) -> List[Dict[str, Any]]:
        """
        Return the complete property schema including both common and postprocessor-specific properties.

        This method combines common properties with postprocessor-specific properties.
        The machine editor should call this method to get all properties.

        Returns:
            List of all property schema dictionaries (common + specific).
        """
        common_props = cls.get_common_property_schema()
        specific_props = cls.get_property_schema()
        return common_props + specific_props

    def __init__(self, job, tooltip, tooltipargs, units, *args, **kwargs):
        self._tooltip = tooltip
        self._tooltipargs = tooltipargs
        self._units = units
        self._args = args
        self._kwargs = kwargs

        # Handle job: can be single job or list of jobs
        if isinstance(job, list):
            self._jobs = job
            if len(self._jobs) == 0:
                raise ValueError("At least one job must be provided")
            # Validate all jobs have the same machine (if Machine attribute exists)
            if hasattr(self._jobs[0], "Machine"):
                machine_name = self._jobs[0].Machine
                for job in self._jobs[1:]:
                    if hasattr(job, "Machine") and job.Machine != machine_name:
                        raise ValueError("All jobs must have the same machine")
            # For now, only single job supported
            if len(self._jobs) > 1:
                raise NotImplementedError(
                    "Multiple jobs are not yet supported. Please process one job at a time."
                )
            self._job = self._jobs[0]  # For backward compatibility
        else:
            self._jobs = [job]
            self._job = job

        # Get machine
        if self._job is None:
            self._machine = None
        elif hasattr(self._job, "Machine"):
            try:
                machine = MachineFactory.get_machine(self._job.Machine)
                if machine is None:
                    # Machine not found in factory - allow manual assignment later
                    Path.Log.warning(
                        f"Machine '{self._job.Machine}' not found in factory. Machine can be set manually."
                    )
                    self._machine = None
                else:
                    self._machine = machine
            except FileNotFoundError as e:
                # Machine not found in factory - allow manual assignment later (e.g., in tests)
                Path.Log.warning(
                    f"Machine '{self._job.Machine}' not found: {e}. Machine can be set manually."
                )
                self._machine = None
        else:
            # Job doesn't have Machine attribute yet (e.g., MockJob or legacy job)
            self._machine = None
        self._modal_state = {
            "X": None,
            "Y": None,
            "Z": None,
            "A": None,
            "B": None,
            "C": None,
            "U": None,
            "V": None,
            "W": None,
            "F": None,
            "S": None,
        }
        self.reinitialize()

        if isinstance(job, dict):
            # process only selected operations
            self._job = job["job"]
            self._operations = job["operations"]
        else:
            # get all operations from 'Operations' group
            self._operations = (
                getattr(self._job.Operations, "Group", []) if self._job is not None else []
            )

    @classmethod
    def exists(cls, processor):
        return processor in Path.Preferences.allAvailablePostProcessors()

    @property
    def tooltip(self):
        """Get the tooltip text for the post processor."""
        raise NotImplementedError("Subclass must implement abstract method")

    @property
    def tooltipArgs(self) -> FormatHelp:
        return self.parser.format_help()

    @property
    def units(self):
        """Get the units used by the post processor."""
        return self._units

    def get_file_extension(self):
        """Return the effective file extension for output files.

        Resolution order:
        1. Machine postprocessor_properties['file_extension'] (user override)
        2. PostProcessor schema default for 'file_extension'
        3. 'nc' fallback

        Returns:
            Extension string without leading dot (e.g. 'sbp', 'gcode', 'nc').
        """
        # Check machine properties first
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            ext = self._machine.postprocessor_properties.get("file_extension", "")
            if ext:
                return ext

        # Fall back to schema default
        for prop in self.get_full_property_schema():
            if prop["name"] == "file_extension":
                return prop.get("default", "nc")

        return "nc"

    def _buildPostList(self, early_tool_prep=False):
        """Determine the specific objects and order to postprocess.

        Args:
            early_tool_prep: If True, split tool changes into separate prep (Tn)
                           and change (M6) commands for better machine efficiency

        Returns:
            List of (name, operations) tuples
        """
        return PostList.buildPostList(self, early_tool_prep)

    def _merge_machine_config(self):
        """Merge machine configuration into the values dict.

        Maps machine config output options to the legacy values dict keys
        used throughout the postprocessor. Subclasses can override to add
        custom config merging.
        """
        if not (self._machine and hasattr(self._machine, "output")):
            return

        # Map machine config to values dict keys using new nested structure
        output_options = self._machine.output

        # Main output options
        if hasattr(output_options, "output_tool_length_offset"):
            self.values["OUTPUT_TOOL_LENGTH_OFFSET"] = output_options.output_tool_length_offset
        if hasattr(output_options, "remote_post"):
            self.values["REMOTE_POST"] = output_options.remote_post

        # Header options
        if hasattr(output_options, "header"):
            header = output_options.header
            if hasattr(header, "include_date"):
                self.values["OUTPUT_HEADER"] = (
                    header.include_date
                )  # Using include_date as master switch
            if hasattr(header, "include_tool_list"):
                self.values["LIST_TOOLS_IN_HEADER"] = header.include_tool_list
            if hasattr(header, "include_fixture_list"):
                self.values["LIST_FIXTURES_IN_HEADER"] = header.include_fixture_list
            if hasattr(header, "include_machine_name"):
                self.values["MACHINE_NAME_IN_HEADER"] = header.include_machine_name
            if hasattr(header, "include_description"):
                self.values["DESCRIPTION_IN_HEADER"] = header.include_description
            if hasattr(header, "include_project_file"):
                self.values["PROJECT_FILE_IN_HEADER"] = header.include_project_file
            if hasattr(header, "include_units"):
                self.values["OUTPUT_UNITS_IN_HEADER"] = header.include_units
            if hasattr(header, "include_document_name"):
                self.values["DOCUMENT_NAME_IN_HEADER"] = header.include_document_name

        # Comment options
        if hasattr(output_options, "comments"):
            comments = output_options.comments
            if hasattr(comments, "enabled"):
                self.values["OUTPUT_COMMENTS"] = comments.enabled
            if hasattr(comments, "symbol"):
                self.values["COMMENT_SYMBOL"] = comments.symbol
                Path.Log.debug(f"Set COMMENT_SYMBOL to: {comments.symbol}")
            if hasattr(comments, "include_operation_labels"):
                self.values["OUTPUT_OPERATION_LABELS"] = comments.include_operation_labels
            if hasattr(comments, "include_blank_lines"):
                self.values["OUTPUT_BLANK_LINES"] = comments.include_blank_lines
            if hasattr(comments, "output_bcnc_comments"):
                Path.Log.debug(f"Found output_bcnc_comments: {comments.output_bcnc_comments}")
                self.values["OUTPUT_BCNC"] = comments.output_bcnc_comments
                Path.Log.debug(f"Set OUTPUT_BCNC to: {self.values['OUTPUT_BCNC']}")

        # Formatting options
        if hasattr(output_options, "formatting"):
            formatting = output_options.formatting
            if hasattr(formatting, "line_numbers"):
                self.values["OUTPUT_LINE_NUMBERS"] = formatting.line_numbers
            if hasattr(formatting, "line_number_start"):
                self.values["line_number"] = formatting.line_number_start
            if hasattr(formatting, "line_increment"):
                self.values["LINE_INCREMENT"] = formatting.line_increment
            if hasattr(formatting, "line_number_prefix"):
                self.values["LINE_NUMBER_PREFIX"] = formatting.line_number_prefix
            if hasattr(formatting, "command_space"):
                self.values["COMMAND_SPACE"] = formatting.command_space
            if hasattr(formatting, "end_of_line_chars"):
                self.values["END_OF_LINE_CHARS"] = formatting.end_of_line_chars

        # Precision options
        if hasattr(output_options, "precision"):
            precision = output_options.precision
            Path.Log.debug(
                f"Loading precision from machine config - axis: {getattr(precision, 'axis', 'N/A')}, feed: {getattr(precision, 'feed', 'N/A')}, spindle: {getattr(precision, 'spindle', 'N/A')}"
            )
            if hasattr(precision, "axis") and precision.axis is not None:
                if isinstance(precision.axis, (int, float)) and precision.axis >= 0:
                    self.values["AXIS_PRECISION"] = int(precision.axis)
                    Path.Log.debug(f"Set AXIS_PRECISION to: {precision.axis}")
                else:
                    Path.Log.warning(
                        f"Invalid axis precision value: {precision.axis}. Must be non-negative. Using default."
                    )
            if hasattr(precision, "feed") and precision.feed is not None:
                if isinstance(precision.feed, (int, float)) and precision.feed >= 0:
                    self.values["FEED_PRECISION"] = int(precision.feed)
                    Path.Log.debug(f"Set FEED_PRECISION to: {precision.feed}")
                else:
                    Path.Log.warning(
                        f"Invalid feed precision value: {precision.feed}. Must be non-negative. Using default."
                    )
            if hasattr(precision, "spindle") and precision.spindle is not None:
                if isinstance(precision.spindle, (int, float)) and precision.spindle >= 0:
                    self.values["SPINDLE_DECIMALS"] = int(precision.spindle)
                    Path.Log.debug(f"Set SPINDLE_DECIMALS to: {precision.spindle}")
                else:
                    Path.Log.warning(
                        f"Invalid spindle precision value: {precision.spindle}. Must be non-negative. Using default."
                    )

        Path.Log.debug(
            f"Final precision values - AXIS_PRECISION: {self.values.get('AXIS_PRECISION')}, FEED_PRECISION: {self.values.get('FEED_PRECISION')}, SPINDLE_DECIMALS: {self.values.get('SPINDLE_DECIMALS')}"
        )

        # Duplicate options
        if hasattr(output_options, "duplicates"):
            duplicates = output_options.duplicates
            if hasattr(duplicates, "commands"):
                self.values["OUTPUT_DUPLICATE_COMMANDS"] = duplicates.commands
            if hasattr(duplicates, "parameters"):
                self.values["OUTPUT_DOUBLES"] = duplicates.parameters

    def _build_header(self, postables):
        """Build the G-code header from job/machine metadata.

        Creates a _HeaderBuilder populated with machine name, project file,
        document name, description, author, and timestamp based on machine
        config settings. Also collects tool and fixture info from postables.

        Subclasses can override to customize header content.

        Returns:
            _HeaderBuilder instance with header data populated.
        """
        gcodeheader = _HeaderBuilder()

        # Only add header information if output_header is enabled
        header_enabled = True
        if self._machine and hasattr(self._machine, "output"):
            header_enabled = self._machine.output.output_header

        if header_enabled:
            # Add machine name if enabled
            if (
                self._machine
                and hasattr(self._machine, "output")
                and hasattr(self._machine.output, "header")
            ):
                if self._machine.output.header.include_machine_name:
                    gcodeheader.add_machine_info(self._machine.name)

                # Add project file if enabled
                if self._machine.output.header.include_project_file and self._job:
                    if hasattr(self._job, "Document") and self._job.Document:
                        project_file = self._job.Document.FileName
                        if project_file:
                            gcodeheader.add_project_file(project_file)

                # Add document name if enabled
                if self._machine.output.header.include_document_name and self._job:
                    if hasattr(self._job, "Document") and self._job.Document:
                        doc_name = self._job.Document.Label
                        if doc_name:
                            gcodeheader.add_document_name(doc_name)

                # Add description if enabled
                if self._machine.output.header.include_description and self._job:
                    if hasattr(self._job, "Description") and self._job.Description:
                        gcodeheader.add_description(self._job.Description)

                # Add author if enabled
                if self._job and hasattr(self._job, "Document") and self._job.Document:
                    if hasattr(self._job.Document, "CreatedBy") and self._job.Document.CreatedBy:
                        gcodeheader.add_author(self._job.Document.CreatedBy)

                # Add date/time if enabled
                if self._machine.output.header.include_date:
                    import datetime

                    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    gcodeheader.add_output_time(timestamp)

        # Collect tool and fixture info from postables
        if self.values.get("OUTPUT_HEADER", True):
            for section_name, sublist in postables:
                for item in sublist:
                    if hasattr(item, "ToolNumber"):  # Tool controller
                        # Check if tools should be listed in header
                        list_tools = True
                        if (
                            self._machine
                            and hasattr(self._machine, "output")
                            and hasattr(self._machine.output, "list_tools_in_header")
                        ):
                            list_tools = self._machine.output.list_tools_in_header

                        if list_tools:
                            gcodeheader.add_tool(item.ToolNumber, item.Label)
                    elif hasattr(item, "Label") and item.Label == "Fixture":  # Fixture
                        # Check if fixtures should be listed in header
                        list_fixtures = True
                        if (
                            self._machine
                            and hasattr(self._machine, "output")
                            and hasattr(self._machine.output, "list_fixtures_in_header")
                        ):
                            list_fixtures = self._machine.output.list_fixtures_in_header

                        if list_fixtures:
                            if hasattr(item, "Path") and item.Path and item.Path.Commands:
                                fixture_name = item.Path.Commands[0].Name
                                gcodeheader.add_fixture(fixture_name)

        return gcodeheader

    def _expand_canned_cycles(self, postables):
        """Terminate canned drill cycles in postable paths.

        Adds cycle termination commands (G80) after canned cycle sequences.
        Drill cycle translation is handled separately by the postprocessor
        via the drill_cycles_to_translate property.

        Subclasses can override to customize canned cycle handling.
        """
        for section_name, sublist in postables:
            for item in sublist:
                has_drill_cycles = False
                if hasattr(item, "Path") and item.Path:
                    drill_commands = [
                        "G73",
                        "G74",
                        "G81",
                        "G82",
                        "G83",
                        "G84",
                        "G85",
                        "G86",
                        "G87",
                        "G88",
                        "G89",
                    ]
                    has_drill_cycles = any(cmd.Name in drill_commands for cmd in item.Path.Commands)

                if has_drill_cycles:
                    item.Path = PostUtils.cannedCycleTerminator(item.Path)

    def _expand_split_arcs(self, postables):
        """Split arc commands into linear segments if configured.

        When machine processing.split_arcs is True, replaces G2/G3 arc
        commands with sequences of G1 linear moves.

        Subclasses can override to customize arc handling.
        """
        if not (
            self._machine
            and hasattr(self._machine, "processing")
            and self._machine.processing.split_arcs
        ):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    item.Path = PostUtils.splitArcs(item.Path)

    def _expand_spindle_wait(self, postables):
        """Inject G4 dwell after spindle start commands (M3/M4).

        When a spindle has spindle_wait > 0, inserts a G4 pause command
        after each spindle start command to allow the spindle to reach speed.

        Subclasses can override to customize spindle wait behavior.
        """
        if not self._machine:
            return

        spindle = self._machine.get_spindle_by_index(0)
        if not (spindle and spindle.spindle_wait > 0):
            return

        wait_time = spindle.spindle_wait
        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    for cmd in item.Path.Commands:
                        new_commands.append(cmd)
                        # After spindle start commands, inject G4 pause
                        if cmd.Name in CONSTANTS.MCODE_SPINDLE_ON:
                            # Create G4 dwell command with P parameter
                            pause_cmd = Path.Command("G4", {"P": wait_time})
                            new_commands.append(pause_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _expand_coolant_delay(self, postables):
        """Inject G4 dwell after coolant on commands.

        When a spindle has coolant_delay > 0, inserts a G4 pause command
        after each coolant on command.

        Subclasses can override to customize coolant delay behavior.
        """
        if not self._machine:
            return

        spindle = self._machine.get_spindle_by_index(0)
        if not (spindle and spindle.coolant_delay > 0):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    for cmd in item.Path.Commands:
                        new_commands.append(cmd)
                        # After coolant on commands, inject G4 pause
                        if cmd.Name in CONSTANTS.MCODE_COOLANT_ON:
                            # Create G4 dwell command with P parameter
                            pause_cmd = Path.Command("G4", {"P": spindle.coolant_delay})
                            new_commands.append(pause_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _expand_translate_rapids(self, postables):
        """Replace G0 rapid moves with G1 linear moves.

        When machine processing.translate_rapid_moves is True, replaces
        G0/G00 commands with G1 using the tool controller rapid rate.

        Subclasses can override to customize rapid move translation.
        """
        if not (
            self._machine
            and hasattr(self._machine, "processing")
            and self._machine.processing.translate_rapid_moves
        ):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    Path.Log.debug(f"Translating rapid moves for {item.Label}")
                    for cmd in item.Path.Commands:
                        if cmd.Name in CONSTANTS.GCODE_MOVE_RAPID:
                            cmd.Name = "G1"
                        new_commands.append(cmd)
                    item.Path = Path.Path(new_commands)

    def _expand_xy_before_z(self, postables):
        """Decompose first move after tool change into XY then Z.

        When machine processing.xy_before_z_after_tool_change is True,
        splits the first move command after a tool change into two moves:
        first XY positioning, then Z plunge. This is a safety feature to
        prevent plunging into the workpiece before positioning.

        Subclasses can override to customize post-tool-change move ordering.
        """
        if not (
            self._machine
            and hasattr(self._machine, "processing")
            and self._machine.processing.xy_before_z_after_tool_change
        ):
            return

        Path.Log.debug("Processing XY before Z after tool change")
        for section_name, sublist in postables:
            # Track whether we just saw a tool change
            tool_change_seen = False

            for item in sublist:
                # Check if this is a tool change item
                if hasattr(item, "ToolNumber"):
                    tool_change_seen = True
                    Path.Log.debug(f"Tool change detected: T{item.ToolNumber}")
                    continue

                # Process operations - check for moves after tool change
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    first_move_processed = False

                    for cmd in item.Path.Commands:
                        # Check if this is a tool change command (M6)
                        if cmd.Name in CONSTANTS.MCODE_TOOL_CHANGE:
                            new_commands.append(cmd)
                            tool_change_seen = True
                            first_move_processed = False
                            Path.Log.debug(f"M6 tool change detected in operation")
                            continue

                        # Check if this is the first move after tool change
                        if (
                            tool_change_seen
                            and not first_move_processed
                            and cmd.Name in CONSTANTS.GCODE_MOVE_ALL
                        ):
                            # Check if this move has both XY and Z components
                            has_xy = "X" in cmd.Parameters or "Y" in cmd.Parameters
                            has_z = "Z" in cmd.Parameters

                            if has_xy and has_z:
                                Path.Log.debug(
                                    f"Decomposing first move after tool change: {cmd.Name}"
                                )

                                # Create XY-only move (first)
                                xy_params = {}
                                for param in ["X", "Y", "A", "B", "C"]:
                                    if param in cmd.Parameters:
                                        xy_params[param] = cmd.Parameters[param]

                                if xy_params:
                                    xy_cmd = Path.Command(cmd.Name, xy_params)
                                    new_commands.append(xy_cmd)
                                    Path.Log.debug(f"  XY move: {cmd.Name} {xy_params}")

                                # Create Z-only move (second)
                                z_params = {"Z": cmd.Parameters["Z"]}
                                # Preserve other non-XY parameters (like F, S, etc.)
                                for param in cmd.Parameters:
                                    if param not in ["X", "Y", "Z", "A", "B", "C"]:
                                        z_params[param] = cmd.Parameters[param]

                                z_cmd = Path.Command(cmd.Name, z_params)
                                new_commands.append(z_cmd)
                                Path.Log.debug(f"  Z move: {cmd.Name} {z_params}")

                                first_move_processed = True
                                tool_change_seen = False  # Reset after decomposing the move
                            else:
                                # Move doesn't have both XY and Z, just add it as-is
                                new_commands.append(cmd)
                                if has_xy or has_z:
                                    first_move_processed = True
                                    tool_change_seen = False  # Reset after processing any move
                        else:
                            # Not the first move or not a move command
                            new_commands.append(cmd)

                    # Update the item's path if we made changes
                    if len(new_commands) != len(item.Path.Commands):
                        item.Path = Path.Path(new_commands)
                        Path.Log.debug(f"Updated path for {item.Label}")

    def _expand_bcnc_commands(self, postables):
        """Inject or remove bCNC block annotation commands.

        When OUTPUT_BCNC is True, injects bCNC block-name, block-expand,
        and block-enable commands at the start of each operation's path,
        and stores postamble commands for later insertion.

        When OUTPUT_BCNC is False, removes any existing bCNC commands.

        Subclasses can override to customize bCNC command handling.
        """
        output_bcnc = self.values.get("OUTPUT_BCNC", False)
        Path.Log.debug(f"OUTPUT_BCNC value: {output_bcnc}")
        # Clear any existing bCNC postamble commands to avoid state leakage
        self._bcnc_postamble_commands = None

        if output_bcnc:
            Path.Log.debug("Creating bCNC commands")
            # Create bCNC postamble commands
            bcnc_postamble_start_cmd = Path.Command("(Block-name: post_amble)")
            bcnc_postamble_start_cmd.Annotations = {"bcnc": "postamble_start"}

            bcnc_postamble_expand_cmd = Path.Command("(Block-expand: 0)")
            bcnc_postamble_expand_cmd.Annotations = {"bcnc": "postamble_meta"}

            bcnc_postamble_enable_cmd = Path.Command("(Block-enable: 1)")
            bcnc_postamble_enable_cmd.Annotations = {"bcnc": "postamble_meta"}

            # Store bCNC postamble commands for later insertion
            self._bcnc_postamble_commands = [
                bcnc_postamble_start_cmd,
                bcnc_postamble_expand_cmd,
                bcnc_postamble_enable_cmd,
            ]

            for section_name, sublist in postables:
                for item in sublist:
                    if hasattr(item, "Proxy"):  # OPERATION
                        # Create bCNC block start command
                        bcnc_start_cmd = Path.Command("(Block-name: " + item.Label + ")")
                        bcnc_start_cmd.Annotations = {"bcnc": "block_start"}

                        # Create bCNC block metadata commands
                        bcnc_expand_cmd = Path.Command("(Block-expand: 0)")
                        bcnc_expand_cmd.Annotations = {"bcnc": "block_meta"}

                        bcnc_enable_cmd = Path.Command("(Block-enable: 1)")
                        bcnc_enable_cmd.Annotations = {"bcnc": "block_meta"}

                        # Insert bCNC commands at the beginning of the operation's Path
                        if hasattr(item, "Path") and item.Path:
                            # Create a copy of the original commands to avoid modifying the original Path
                            original_commands = list(item.Path.Commands)

                            # Create new Path with bCNC commands
                            new_commands = [bcnc_start_cmd, bcnc_expand_cmd, bcnc_enable_cmd]
                            new_commands.extend(original_commands)
                            # Create a new Path object
                            item.Path = Path.Path(new_commands)
        else:
            # OUTPUT_BCNC is False - remove any existing bCNC commands from operations
            Path.Log.debug("Removing existing bCNC commands")
            for section_name, sublist in postables:
                for item in sublist:
                    if hasattr(item, "Proxy") and hasattr(item, "Path") and item.Path:
                        # Filter out any existing bCNC commands
                        filtered_commands = []
                        for cmd in item.Path.Commands:
                            if not (
                                cmd.Name.startswith("(Block-name:")
                                or cmd.Name.startswith("(Block-expand:")
                                or cmd.Name.startswith("(Block-enable:")
                            ):
                                filtered_commands.append(cmd)

                        # Create new Path without bCNC commands
                        if len(filtered_commands) != len(item.Path.Commands):
                            item.Path = Path.Path(filtered_commands)

    def _expand_tool_length_offset(self, postables):
        """Inject or remove G43 tool length offset commands.

        When OUTPUT_TOOL_LENGTH_OFFSET is True, adds G43 commands after M6
        tool change commands in operations, and tracks which tool change
        items need G43 commands vs which should have M6 suppressed.

        When OUTPUT_TOOL_LENGTH_OFFSET is False, removes any existing G43
        commands from operation paths.

        Subclasses can override to customize tool length offset handling.
        """
        output_tool_length_offset = self.values.get("OUTPUT_TOOL_LENGTH_OFFSET", True)
        Path.Log.debug(f"OUTPUT_TOOL_LENGTH_OFFSET value: {output_tool_length_offset}")

        # Dictionary to store G43 commands for tool change items
        self._tool_change_g43_commands = {}
        # Track which tool changes should be suppressed (because operation has M6)
        self._suppress_tool_change_m6 = set()

        if output_tool_length_offset:
            Path.Log.debug("Creating G43 tool length offset commands")
            for section_name, sublist in postables:
                # First pass: check if operations have M6 commands and add G43 to them
                for item in sublist:
                    if hasattr(item, "Proxy"):  # OPERATION
                        if hasattr(item, "Path") and item.Path:
                            commands_with_g43 = []
                            for cmd in item.Path.Commands:
                                commands_with_g43.append(cmd)
                                # If this is an M6 command, add G43 after it
                                if cmd.Name in ("M6", "M06") and "T" in cmd.Parameters:
                                    tool_num = cmd.Parameters["T"]
                                    g43_cmd = Path.Command("G43", {"H": tool_num})
                                    g43_cmd.Annotations = {"tool_length_offset": True}
                                    commands_with_g43.append(g43_cmd)
                                    Path.Log.debug(
                                        f"Added G43 H{tool_num} after M6 in operation {item.Label}"
                                    )

                            # Update the operation's Path with G43 commands inserted
                            if len(commands_with_g43) != len(item.Path.Commands):
                                item.Path = Path.Path(commands_with_g43)

                # Second pass: handle tool change items and mark suppression
                operations_with_m6 = set()
                for item in sublist:
                    if hasattr(item, "Proxy"):  # OPERATION
                        if hasattr(item, "Path") and item.Path:
                            for cmd in item.Path.Commands:
                                if cmd.Name in ("M6", "M06") and "T" in cmd.Parameters:
                                    tool_num = cmd.Parameters["T"]
                                    operations_with_m6.add(tool_num)

                for item in sublist:
                    if hasattr(item, "ToolNumber"):  # TOOLCHANGE
                        tool_num = item.ToolNumber
                        if tool_num in operations_with_m6:
                            # Suppress M6 generation for this tool change since operation has it
                            self._suppress_tool_change_m6.add(id(item))
                            Path.Log.debug(f"Suppressing M6 generation for tool change T{tool_num}")
                        else:
                            # Add G43 command to tool change item
                            g43_cmd = Path.Command("G43", {"H": tool_num})
                            g43_cmd.Annotations = {"tool_length_offset": True}
                            self._tool_change_g43_commands[id(item)] = [g43_cmd]
        else:
            # OUTPUT_TOOL_LENGTH_OFFSET is False - remove G43 commands from operation Paths
            Path.Log.debug("G43 tool length offset commands disabled")
            self._tool_change_g43_commands = {}
            self._suppress_tool_change_m6 = set()

            # Remove any existing G43 commands from operation Paths
            for section_name, sublist in postables:
                for item in sublist:
                    if hasattr(item, "Proxy") and hasattr(item, "Path") and item.Path:
                        # Filter out any existing G43 commands
                        filtered_commands = []
                        for cmd in item.Path.Commands:
                            if not (
                                cmd.Name == "G43" and cmd.Annotations.get("tool_length_offset")
                            ):
                                filtered_commands.append(cmd)

                        # Create new Path without G43 commands
                        if len(filtered_commands) != len(item.Path.Commands):
                            item.Path = Path.Path(filtered_commands)

    def export2(self) -> Union[None, GCodeSections]:
        """
        Process jobs through all postprocessing stages to produce final G-code.

        Assumes Stage 0 (Configuration) is complete.

        Stages:
        0. Pre-processing Dialog - Collect user input before processing
        1. Ordering - Build ordered list of postables
        2. Command Expansion - Canned cycles, arc splitting
        3. Command Conversion - Convert Path.Commands to G-code strings
        4. G-code Optimization - Deduplication, line numbering
        5. Output Production - Assemble final structure
        6. Remote Posting - Post-processing network operations
        """
        from Path.Post.GcodeProcessingUtils import (
            deduplicate_repeated_commands,
            suppress_redundant_axes_words,
            filter_inefficient_moves,
            insert_line_numbers,
        )

        Path.Log.debug("Starting export2()")

        # ===== STAGE 0: PRE-PROCESSING DIALOG =====
        # Allow post processors to collect user input before processing begins
        if not self.pre_processing_dialog():
            Path.Log.info("Pre-processing dialog cancelled - aborting export")
            return None

        # Merge machine configuration into values dict
        self._merge_machine_config()

        # ===== STAGE 1: ORDERING =====
        # Process all jobs (currently only first job supported)
        all_job_sections = []

        # Get early_tool_prep setting from machine config
        early_tool_prep = False
        if self._machine and hasattr(self._machine, "processing"):
            early_tool_prep = getattr(self._machine.processing, "early_tool_prep", False)

        # Build ordered postables for this job
        postables = self._buildPostList(early_tool_prep)

        # ===== STAGE 2: COMMAND EXPANSION =====
        gcodeheader = self._build_header(postables)
        self._expand_canned_cycles(postables)
        self._expand_split_arcs(postables)
        self._expand_spindle_wait(postables)
        self._expand_coolant_delay(postables)
        self._expand_translate_rapids(postables)
        self._expand_xy_before_z(postables)
        self._expand_bcnc_commands(postables)
        self._expand_tool_length_offset(postables)

        Path.Log.debug(postables)

        # ===== STAGE 3: COMMAND CONVERSION =====
        job_sections = []

        # Collect HEADER lines (comment-only) - controlled by OUTPUT_HEADER
        header_lines = []
        if self.values.get("OUTPUT_HEADER", True):
            header_commands = gcodeheader.Path.Commands if hasattr(gcodeheader, "Path") else []
            comment_symbol = self.values.get("COMMENT_SYMBOL", "(")
            for cmd in header_commands:
                if cmd.Name.startswith("("):
                    comment_text = (
                        cmd.Name[1:-1]
                        if cmd.Name.startswith("(") and cmd.Name.endswith(")")
                        else cmd.Name[1:]
                    )
                    if comment_symbol == "(":
                        header_lines.append(f"({comment_text})")
                    else:
                        header_lines.append(f"{comment_symbol} {comment_text}")

        # Collect PREAMBLE lines
        preamble_lines = []
        if self._machine and self._machine.postprocessor_properties.get("preamble"):
            preamble_lines = [
                line
                for line in self._machine.postprocessor_properties["preamble"].split("\n")
                if line.strip()
            ]

        # Insert unit command (G20/G21) based on output_units setting
        unit_command_line = []
        if self._machine and hasattr(self._machine, "output"):
            from Machine.models.machine import OutputUnits

            if self._machine.output.units == OutputUnits.METRIC:
                unit_command_line = ["G21"]
            elif self._machine.output.units == OutputUnits.IMPERIAL:
                unit_command_line = ["G20"]

        # Collect PRE-JOB lines
        pre_job_lines = []
        if self._machine and self._machine.postprocessor_properties.get("pre_job"):
            pre_job_lines = [
                line
                for line in self._machine.postprocessor_properties["pre_job"].split("\n")
                if line.strip()
            ]

        # Process each section (BODY)
        for section_name, sublist in postables:
            gcode_lines = []

            # Add header, preamble, unit command, and pre-job lines only to first section
            if section_name == postables[0][0]:
                # Header comments first (no line numbers)
                gcode_lines.extend(header_lines)
                # Then preamble
                gcode_lines.extend(preamble_lines)
                # Then unit command (G20/G21)
                gcode_lines.extend(unit_command_line)
                # Then pre-job
                gcode_lines.extend(pre_job_lines)

            for item in sublist:
                # Determine item type and add appropriate pre-blocks
                if hasattr(item, "ToolNumber"):  # TOOLCHANGE
                    if self._machine and self._machine.postprocessor_properties.get(
                        "pre_tool_change"
                    ):
                        pre_lines = [
                            line
                            for line in self._machine.postprocessor_properties[
                                "pre_tool_change"
                            ].split("\n")
                            if line.strip()
                        ]
                        gcode_lines.extend(pre_lines)

                    # Generate M6 tool change command
                    if self._machine and hasattr(self._machine, "processing"):
                        if self._machine.processing.tool_change:
                            # Check if M6 generation should be suppressed (operation already has M6)
                            if id(item) not in self._suppress_tool_change_m6:
                                # Generate M6 T{ToolNumber} command
                                tool_num = item.ToolNumber
                                m6_cmd = f"M6 T{tool_num}"
                                gcode_lines.append(m6_cmd)

                                # Output G43 commands if they were added in Stage 2.6
                                g43_commands = self._tool_change_g43_commands.get(id(item), [])
                                for g43_cmd in g43_commands:
                                    gcode_g43 = self.convert_command_to_gcode(g43_cmd)
                                    if gcode_g43 is not None and gcode_g43.strip():
                                        gcode_lines.append(gcode_g43)
                            else:
                                # M6 suppressed - operation already handles it
                                Path.Log.debug(
                                    f"M6 T{item.ToolNumber} suppressed - handled by operation"
                                )
                        else:
                            # Tool change disabled - output as comment
                            comment_symbol = self.values.get("COMMENT_SYMBOL", "(")
                            tool_num = item.ToolNumber
                            if comment_symbol == "(":
                                gcode_lines.append(f"(Tool change suppressed: M6 T{tool_num})")
                            else:
                                gcode_lines.append(
                                    f"{comment_symbol} Tool change suppressed: M6 T{tool_num}"
                                )
                    else:
                        # No machine config - check suppression before outputting M6
                        if id(item) not in self._suppress_tool_change_m6:
                            tool_num = item.ToolNumber
                            m6_cmd = f"M6 T{tool_num}"
                            gcode_lines.append(m6_cmd)

                            # Output G43 commands if they were added in Stage 2.6
                            g43_commands = self._tool_change_g43_commands.get(id(item), [])
                            for g43_cmd in g43_commands:
                                gcode_g43 = self.convert_command_to_gcode(g43_cmd)
                                if gcode_g43 is not None and gcode_g43.strip():
                                    gcode_lines.append(gcode_g43)
                elif hasattr(item, "Label") and item.Label == "Fixture":  # FIXTURE
                    if self._machine and self._machine.postprocessor_properties.get(
                        "pre_fixture_change"
                    ):
                        pre_lines = [
                            line
                            for line in self._machine.postprocessor_properties[
                                "pre_fixture_change"
                            ].split("\n")
                            if line.strip()
                        ]
                        gcode_lines.extend(pre_lines)
                elif hasattr(item, "Proxy"):  # OPERATION
                    if self._machine and self._machine.postprocessor_properties.get(
                        "pre_operation"
                    ):
                        pre_lines = [
                            line
                            for line in self._machine.postprocessor_properties[
                                "pre_operation"
                            ].split("\n")
                            if line.strip()
                        ]
                        gcode_lines.extend(pre_lines)

                # Convert Path commands to G-code
                if hasattr(item, "Path") and item.Path:
                    # Group consecutive rotary moves together
                    in_rotary_group = False

                    for cmd in item.Path.Commands:
                        try:
                            # Check if this command involves a rotary axis move
                            has_rotary = any(param in cmd.Parameters for param in ["A", "B", "C"])

                            # Start a new rotary group if needed
                            if has_rotary and not in_rotary_group:
                                if self._machine and self._machine.postprocessor_properties.get(
                                    "pre_rotary_move"
                                ):
                                    pre_rotary_lines = [
                                        line
                                        for line in self._machine.postprocessor_properties[
                                            "pre_rotary_move"
                                        ].split("\n")
                                        if line.strip()
                                    ]
                                    gcode_lines.extend(pre_rotary_lines)
                                in_rotary_group = True

                            # End rotary group if we're leaving rotary moves
                            elif not has_rotary and in_rotary_group:
                                if self._machine and self._machine.postprocessor_properties.get(
                                    "post_rotary_move"
                                ):
                                    post_rotary_lines = [
                                        line
                                        for line in self._machine.postprocessor_properties[
                                            "post_rotary_move"
                                        ].split("\n")
                                        if line.strip()
                                    ]
                                    gcode_lines.extend(post_rotary_lines)
                                in_rotary_group = False

                            # Convert command to G-code
                            gcode = self.convert_command_to_gcode(cmd)

                            # Handle tool_change setting - suppress M6 if disabled
                            if cmd.Name in ("M6", "M06"):
                                if (
                                    self._machine
                                    and hasattr(self._machine, "processing")
                                    and not self._machine.processing.tool_change
                                ):
                                    # Convert M6 to comment instead of outputting it
                                    comment_symbol = self.values.get("COMMENT_SYMBOL", "(")
                                    if comment_symbol == "(":
                                        gcode = f"(Tool change suppressed: {gcode})"
                                    else:
                                        gcode = f"{comment_symbol} Tool change suppressed: {gcode}"

                                # Handle tool_before_change setting - swap T and M6 order
                                # This is handled in convert_command_to_gcode, but we need to track it
                                # The actual swapping happens when formatting the command line

                            # Add the G-code line
                            if gcode is not None and gcode.strip():
                                gcode_lines.append(gcode)

                        except (ValueError, AttributeError) as e:
                            # Skip unsupported commands or log error
                            Path.Log.debug(f"Skipping command {cmd.Name}: {e}")

                    # Close rotary group if we ended while still in one
                    if in_rotary_group:
                        if self._machine and self._machine.postprocessor_properties.get(
                            "post_rotary_move"
                        ):
                            post_rotary_lines = [
                                line
                                for line in self._machine.postprocessor_properties[
                                    "post_rotary_move"
                                ].split("\n")
                                if line.strip()
                            ]
                            gcode_lines.extend(post_rotary_lines)

                # Add appropriate post-blocks
                if hasattr(item, "ToolNumber"):  # TOOLCHANGE
                    if self._machine and self._machine.postprocessor_properties.get(
                        "post_tool_change"
                    ):
                        post_lines = [
                            line
                            for line in self._machine.postprocessor_properties[
                                "post_tool_change"
                            ].split("\n")
                            if line.strip()
                        ]
                        gcode_lines.extend(post_lines)
                    # Add tool_return after tool change
                    if self._machine and self._machine.postprocessor_properties.get("tool_return"):
                        return_lines = [
                            line
                            for line in self._machine.postprocessor_properties["tool_return"].split(
                                "\n"
                            )
                            if line.strip()
                        ]
                        gcode_lines.extend(return_lines)
                elif hasattr(item, "Label") and item.Label == "Fixture":  # FIXTURE
                    if self._machine and self._machine.postprocessor_properties.get(
                        "post_fixture_change"
                    ):
                        post_lines = [
                            line
                            for line in self._machine.postprocessor_properties[
                                "post_fixture_change"
                            ].split("\n")
                            if line.strip()
                        ]
                        gcode_lines.extend(post_lines)
                elif hasattr(item, "Proxy"):  # OPERATION
                    if self._machine and self._machine.postprocessor_properties.get(
                        "post_operation"
                    ):
                        post_lines = [
                            line
                            for line in self._machine.postprocessor_properties[
                                "post_operation"
                            ].split("\n")
                            if line.strip()
                        ]
                        gcode_lines.extend(post_lines)

            # ===== STAGE 4: G-CODE OPTIMIZATION =====
            if gcode_lines:
                # Separate header comments from numbered lines
                num_header_lines = len(header_lines) if section_name == postables[0][0] else 0
                header_part = gcode_lines[:num_header_lines]
                body_part = gcode_lines[num_header_lines:]

                # Apply optimizations to body only (not header comments)
                if body_part:
                    # Modal command deduplication
                    # OUTPUT_DUPLICATE_COMMANDS: True = output all, False = suppress duplicates
                    if not self.values.get("OUTPUT_DUPLICATE_COMMANDS", True):
                        body_part = deduplicate_repeated_commands(body_part)

                    # Suppress redundant axis words (only if OUTPUT_DOUBLES is False)
                    # OUTPUT_DOUBLES: True = output all parameters, False = suppress duplicates
                    if not self.values.get("OUTPUT_DOUBLES", True):
                        body_part = suppress_redundant_axes_words(body_part)

                # Filter inefficient moves (optional optimization)
                # Collapses redundant G0 rapid move chains - may be too aggressive for some machines
                if body_part and self._machine and hasattr(self._machine, "processing"):
                    if hasattr(self._machine.processing, "filter_inefficient_moves"):
                        if self._machine.processing.filter_inefficient_moves:
                            body_part = filter_inefficient_moves(body_part)

                # Line numbering (only on body, not header comments)
                if body_part and self.values.get("OUTPUT_LINE_NUMBERS", False):
                    start = 10
                    increment = 10
                    if (
                        self._machine
                        and hasattr(self._machine, "output")
                        and hasattr(self._machine.output, "formatting")
                    ):
                        start = self._machine.output.formatting.line_number_start
                        increment = self._machine.output.formatting.line_increment
                    body_part = insert_line_numbers(body_part, start=start, increment=increment)

                # Recombine header and body
                final_lines = header_part + body_part

                # Build gcode with \n separators (standard format)
                gcode_with_newlines = "\n".join(final_lines)

                # Get configured line ending and apply transformation
                line_ending = self.values.get("END_OF_LINE_CHARS", "\n")

                if line_ending == "\n":
                    # Default: let _write_file convert to system line endings
                    gcode_string = gcode_with_newlines
                else:
                    # Custom or standard line endings: replace \n with configured chars
                    gcode_string = gcode_with_newlines.replace("\n", line_ending)

                # Add section to output
                job_sections.append((section_name, gcode_string))

        # Append POST-JOB and POSTAMBLE blocks to the last section
        if job_sections:
            last_section_name, last_section_gcode = job_sections[-1]
            additional_lines = []

            # Add bCNC postamble commands if they were created
            if (
                hasattr(self, "_bcnc_postamble_commands")
                and self._bcnc_postamble_commands is not None
            ):
                Path.Log.debug(
                    f"Processing {len(self._bcnc_postamble_commands)} bCNC postamble commands"
                )
                for cmd in self._bcnc_postamble_commands:
                    gcode = self.convert_command_to_gcode(cmd)
                    if gcode is not None and gcode.strip():
                        additional_lines.append(gcode)
            else:
                Path.Log.debug("No bCNC postamble commands to process")

            # Add POST-JOB block
            if self._machine and self._machine.postprocessor_properties.get("post_job"):
                post_job_lines = [
                    line
                    for line in self._machine.postprocessor_properties["post_job"].split("\n")
                    if line.strip()
                ]
                if post_job_lines:
                    additional_lines.extend(post_job_lines)

            # Add POSTAMBLE section
            if self._machine and self._machine.postprocessor_properties.get("postamble"):
                postamble_lines = [
                    line
                    for line in self._machine.postprocessor_properties["postamble"].split("\n")
                    if line.strip()
                ]
                if postamble_lines:
                    additional_lines.extend(postamble_lines)

            # Append to last section if we have additional lines
            if additional_lines:
                # Build with \n separators
                additional_gcode_newlines = "\n".join(additional_lines)

                # Get configured line ending and apply transformation
                line_ending = self.values.get("END_OF_LINE_CHARS", "\n")

                if line_ending == "\n":
                    additional_gcode = "\n" + additional_gcode_newlines
                else:
                    additional_gcode = line_ending + additional_gcode_newlines.replace(
                        "\n", line_ending
                    )

                job_sections[-1] = (last_section_name, last_section_gcode + additional_gcode)

        # Add FOOTER section (comment-only)
        # TODO: Add footer generation if needed

        all_job_sections.extend(job_sections)

        # ===== STAGE 5: OUTPUT PRODUCTION =====
        # Return sections (file writing happens elsewhere)

        # Prepend safetyblock to the first section if present
        if (
            all_job_sections
            and self._machine
            and self._machine.postprocessor_properties.get("safetyblock")
        ):
            safety_lines = [
                line
                for line in self._machine.postprocessor_properties["safetyblock"].split("\n")
                if line.strip()
            ]
            if safety_lines:
                # Build with \n separators
                safety_gcode_newlines = "\n".join(safety_lines)

                # Get configured line ending and apply transformation
                line_ending = self.values.get("END_OF_LINE_CHARS", "\n")

                if line_ending == "\n":
                    safety_gcode = safety_gcode_newlines + "\n"
                else:
                    safety_gcode = safety_gcode_newlines.replace("\n", line_ending) + line_ending

                first_section_name, first_section_gcode = all_job_sections[0]
                all_job_sections[0] = (first_section_name, safety_gcode + first_section_gcode)

        Path.Log.debug(f"Returning {len(all_job_sections)} sections")
        Path.Log.debug(f"Sections: {all_job_sections}")

        # ===== STAGE 6: REMOTE POSTING =====
        # Call remote_post method for subclasses to override
        try:
            self.remote_post(all_job_sections)
        except Exception as e:
            Path.Log.error(f"Remote posting failed: {e}")
            # Don't fail the entire post-processing for remote posting errors

        return all_job_sections

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

        # Get early_tool_prep setting from machine config
        early_tool_prep = False
        if self._machine and hasattr(self._machine, "processing"):
            early_tool_prep = getattr(self._machine.processing, "early_tool_prep", False)

        postables = self._buildPostList(early_tool_prep)
        Path.Log.debug(f"postables {postables}")

        # Process canned cycles for drilling operations
        for _, section in enumerate(postables):
            _, sublist = section
            for obj in sublist:
                if hasattr(obj, "Path"):
                    obj.Path = PostUtils.cannedCycleTerminator(obj.Path)

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

    def remote_post(self, gcode_sections):
        """
        Hook for remote posting functionality.

        This method is called after all G-code has been generated and written.
        Base implementation does nothing, but subclasses can override to implement
        remote posting, file uploads, or other post-processing actions.

        Args:
            gcode_sections: List of (section_name, gcode) tuples containing all generated G-code
        """
        pass

    def pre_processing_dialog(self):
        """
        Hook for pre-processing dialog functionality.

        This method is called before any post-processing begins, allowing post processors
        to collect user input through dialogs or other interactive means.
        Base implementation does nothing, but subclasses can override to implement
        configuration dialogs, validation checks, or user input collection.

        Returns:
            bool: True to continue with post-processing, False to cancel
        """
        return True

    def convert_command_to_gcode(self, command: Path.Command) -> str:
        """
        Converts a single-line command to gcode.

        This method dispatches to specialized hook methods based on command type.
        Derived postprocessors can override individual hook methods to customize
        specific command handling without reimplementing the entire function.

        Hook methods available for override:
          - _convert_comment() - Comment handling
          - _convert_rapid_move() - G0/G00 rapid positioning
          - _convert_linear_move() - G1/G01 linear interpolation
          - _convert_arc_move() - G2/G3 circular interpolation
          - _convert_drill_cycle() - G73, G81-G89 canned cycles
          - _convert_probe() - G38.2 probing
          - _convert_dwell() - G4 dwell
          - _convert_tool_change() - M6 tool change
          - _convert_spindle_command() - M3/M4/M5 spindle control
          - _convert_coolant_command() - M7/M8/M9 coolant control
          - _convert_program_control() - M0/M1/M2/M30 program control
          - _convert_fixture() - G54-G59.x work coordinate systems
          - _convert_modal_command() - Other modal G-codes
          - _convert_generic_command() - Fallback for unhandled commands

        Example override in derived class:
            def _convert_drill_cycle(self, command):
                if command.Name == 'G84':
                    # Custom G84 handling
                    return "custom G84 code"
                # Fall back to parent for other drill cycles
                return super()._convert_drill_cycle(command)
        """
        import CONSTANTS

        # Validate command is supported
        supported = CONSTANTS.GCODE_SUPPORTED + CONSTANTS.GCODE_FIXTURES + CONSTANTS.MCODE_SUPPORTED
        if (
            command.Name not in supported
            and not command.Name.startswith("(")
            and not command.Name.startswith("T")
        ):
            raise ValueError(f"Unsupported command: {command.Name}")

        # Dispatch to appropriate hook method based on command type
        command_name = command.Name

        # Comments
        if command_name.startswith("("):
            return self._convert_comment(command)

        # Rapid moves
        if command_name in CONSTANTS.GCODE_MOVE_RAPID:
            return self._convert_rapid_move(command)

        # Linear moves
        if command_name in CONSTANTS.GCODE_MOVE_STRAIGHT:
            return self._convert_linear_move(command)

        # Arc moves
        if command_name in CONSTANTS.GCODE_MOVE_ARC:
            return self._convert_arc_move(command)

        # Drill cycles
        if command_name in CONSTANTS.GCODE_MOVE_DRILL + CONSTANTS.GCODE_DRILL_EXTENDED:
            return self._convert_drill_cycle(command)

        # Probe
        if command_name in CONSTANTS.GCODE_PROBE:
            return self._convert_probe(command)

        # Dwell
        if command_name in CONSTANTS.GCODE_DWELL:
            return self._convert_dwell(command)

        # Tool change
        if command_name in CONSTANTS.MCODE_TOOL_CHANGE:
            return self._convert_tool_change(command)

        # Spindle control
        if command_name in CONSTANTS.MCODE_SPINDLE_ON + CONSTANTS.MCODE_SPINDLE_OFF:
            return self._convert_spindle_command(command)

        # Coolant control
        if command_name in CONSTANTS.MCODE_COOLANT:
            return self._convert_coolant_command(command)

        # Program control
        if (
            command_name
            in CONSTANTS.MCODE_STOP
            + CONSTANTS.MCODE_OPTIONAL_STOP
            + CONSTANTS.MCODE_END
            + CONSTANTS.MCODE_END_RESET
        ):
            return self._convert_program_control(command)

        # Fixtures
        if command_name in CONSTANTS.GCODE_FIXTURES:
            return self._convert_fixture(command)

        # Modal commands (G43, G80, G90, G91, G92, G93, G94, G95, G96, G97, G98, G99, etc.)
        if (
            command_name
            in CONSTANTS.GCODE_TOOL_LENGTH_OFFSET
            + CONSTANTS.GCODE_CYCLE_CANCEL
            + CONSTANTS.GCODE_DISTANCE_MODE
            + CONSTANTS.GCODE_OFFSET
            + CONSTANTS.GCODE_FEED_INVERSE_TIME
            + CONSTANTS.GCODE_FEED_UNITS_PER_MIN
            + CONSTANTS.GCODE_FEED_UNITS_PER_REV
            + CONSTANTS.GCODE_SPINDLE_CSS
            + CONSTANTS.GCODE_SPINDLE_RPM
            + CONSTANTS.GCODE_RETURN_MODE
        ):
            return self._convert_modal_command(command)

        # Fallback for any unhandled commands
        return self._convert_generic_command(command)

    def _convert_comment(self, command: Path.Command) -> str:
        """
        Converts a comment command to gcode.

        This method can be overridden by derived postprocessors to customize comment handling.
        """
        annotations = command.Annotations

        # Check if comments should be output
        if self.values.get("OUTPUT_BCNC", False) and annotations.get("bcnc"):
            # bCNC commands should be output even if OUTPUT_COMMENTS is false
            pass
        elif not self.values.get("OUTPUT_COMMENTS", True):
            # Comments are disabled and this is not a bCNC command - suppress it
            return None

        # Check for blockdelete annotation
        block_delete_string = "/" if annotations.get("blockdelete") else ""

        # Get comment symbol
        comment_symbol = self.values.get("COMMENT_SYMBOL", "(")

        # Extract comment text from command name
        # Command names come in as "(comment text)" so strip the outer delimiters
        comment_text = (
            command.Name[1:-1]
            if command.Name.startswith("(") and command.Name.endswith(")")
            else command.Name[1:]
        )

        # Sanitize nested parentheses when using parenthesis comment symbol
        # Note: Path.Command truncates at first ) after opening (, so (text (nested)) becomes (text nested))
        # We can't recover the lost opening (, but we can clean up what remains
        if comment_symbol == "(":
            # Replace any remaining parentheses with square brackets
            comment_text = comment_text.replace("(", "[").replace(")", "]")

        Path.Log.debug(
            f"Formatting comment with symbol: '{comment_symbol}', text: '{comment_text}'"
        )
        if comment_symbol == "(":
            return f"{block_delete_string}({comment_text})"
        else:
            return f"{block_delete_string}{comment_symbol} {comment_text}"

    def _convert_move(self, command: Path.Command) -> str:
        """
        Converts a rapid move command to gcode.

        This method can be overridden by derived postprocessors to customize rapid move handling.
        """
        from Path.Post.UtilsParse import format_command_line

        # Extract command components
        command_name = command.Name
        params = command.Parameters
        annotations = command.Annotations

        # Check for blockdelete annotation
        block_delete_string = "/" if annotations.get("blockdelete") else ""

        # Build command line
        command_line: CommandLine = []
        command_line.append(command_name)

        # Format parameters with clean, stateless implementation
        parameter_order = self.values.get(
            "PARAMETER_ORDER", ["X", "Y", "Z", "F", "I", "J", "K", "R", "Q", "P"]
        )

        def format_axis_param(value):
            """Format axis parameter with unit conversion and precision."""
            # Apply unit conversion based on machine units setting
            is_imperial = False
            if self._machine and hasattr(self._machine, "output"):
                from Machine.models.machine import OutputUnits

                is_imperial = self._machine.output.units == OutputUnits.IMPERIAL
            else:
                # Fallback to legacy UNITS value
                units = self.values.get("UNITS", "G21")
                is_imperial = units == "G20"

            if is_imperial:
                converted_value = value / 25.4  # Convert mm to inches
            else:
                converted_value = value  # Keep as mm

            precision = self.values.get("AXIS_PRECISION") or 3
            return f"{converted_value:.{precision}f}"

        def format_feed_param(value):
            """Format feed parameter with speed precision and unit conversion."""
            # Convert from mm/sec to mm/min (multiply by 60)
            feed_value = value * 60.0

            # Apply unit conversion if imperial
            is_imperial = False
            if self._machine and hasattr(self._machine, "output"):
                from Machine.models.machine import OutputUnits

                is_imperial = self._machine.output.units == OutputUnits.IMPERIAL
            else:
                # Fallback to legacy UNITS value
                units = self.values.get("UNITS", "G21")
                is_imperial = units == "G20"

            if is_imperial:
                feed_value = feed_value / 25.4  # Convert mm/min to in/min

            precision = self.values.get("FEED_PRECISION") or 3
            return f"{feed_value:.{precision}f}"

        def format_spindle_param(value):
            """Format spindle parameter with spindle decimals."""
            decimals = self.values.get("SPINDLE_DECIMALS")
            if decimals is None:
                decimals = 0
            return f"{value:.{decimals}f}"

        def format_int_param(value):
            """Format integer parameter."""
            return str(int(value))

        # Parameter type mappings
        param_formatters = {
            # Axis parameters
            "X": format_axis_param,
            "Y": format_axis_param,
            "Z": format_axis_param,
            "U": format_axis_param,
            "V": format_axis_param,
            "W": format_axis_param,
            "A": format_axis_param,
            "B": format_axis_param,
            "C": format_axis_param,
            # Arc parameters
            "I": format_axis_param,
            "J": format_axis_param,
            "K": format_axis_param,
            "R": format_axis_param,
            "Q": format_axis_param,
            # Feed and spindle
            "F": format_feed_param,
            "S": format_spindle_param,
            # P parameter - use axis formatting to support decimal values (e.g., G4 P2.5)
            "P": format_axis_param,
            # Integer parameters
            "D": format_int_param,
            "H": format_int_param,
            "L": format_int_param,
            "T": format_int_param,
        }

        for parameter in parameter_order:
            if parameter in params:
                # Check if we should suppress duplicate parameters
                if not self.values.get("OUTPUT_DOUBLES", False):  # Changed default value to False
                    # Suppress parameters that haven't changed
                    current_value = params[parameter]
                    if (
                        parameter in self._modal_state
                        and self._modal_state[parameter] == current_value
                    ):
                        continue  # Skip this parameter

                if parameter in param_formatters:
                    formatted_value = param_formatters[parameter](params[parameter])
                    command_line.append(f"{parameter}{formatted_value}")
                    # Update modal state
                    self._modal_state[parameter] = params[parameter]
                else:
                    # Default formatting for unhandled parameters
                    command_line.append(f"{parameter}{params[parameter]}")
                    # Update modal state for unhandled parameters too
                    self._modal_state[parameter] = params[parameter]

        # Handle tool length offset (G43) suppression
        if command_name in ("G43",):
            if not self.values.get("OUTPUT_TOOL_LENGTH_OFFSET", True):
                # Tool length offset disabled - suppress G43 command
                return None

        # Format the command line
        formatted_line = format_command_line(self.values, command_line)

        # Combine block delete and formatted command (no line numbers)
        gcode_string = f"{block_delete_string}{formatted_line}"

        return gcode_string

    def _convert_rapid_move(self, command: Path.Command) -> str:
        """
        Converts a rapid move command to gcode.

        This method can be overridden by derived postprocessors to customize rapid move handling.
        """
        return self._convert_move(command)

    def _convert_linear_move(self, command: Path.Command) -> str:
        """
        Converts a linear move command to gcode.

        This method can be overridden by derived postprocessors to customize linear move handling.
        """
        return self._convert_move(command)

    def _convert_arc_move(self, command: Path.Command) -> str:
        """
        Converts an arc move command to gcode.

        This method can be overridden by derived postprocessors to customize arc move handling.
        """
        return self._convert_move(command)

    def _convert_drill_cycle(self, command: Path.Command) -> str:
        """
        Converts a drill cycle command to gcode.

        This method can be overridden by derived postprocessors to customize drill cycle handling.
        """
        return self._convert_move(command)

    def _convert_probe(self, command: Path.Command) -> str:
        """
        Converts a probe command to gcode.

        This method can be overridden by derived postprocessors to customize probe handling.
        """
        return self._convert_move(command)

    def _convert_dwell(self, command: Path.Command) -> str:
        """
        Converts a dwell command to gcode.

        This method can be overridden by derived postprocessors to customize dwell handling.
        """
        return self._convert_move(command)

    def _convert_tool_change(self, command: Path.Command) -> str:
        """
        Converts a tool change command to gcode.

        This method can be overridden by derived postprocessors to customize tool change handling.
        """
        return self._convert_move(command)

    def _convert_spindle_command(self, command: Path.Command) -> str:
        """
        Converts a spindle command to gcode.

        This method can be overridden by derived postprocessors to customize spindle handling.
        """
        return self._convert_move(command)

    def _convert_coolant_command(self, command: Path.Command) -> str:
        """
        Converts a coolant command to gcode.

        This method can be overridden by derived postprocessors to customize coolant handling.
        """
        return self._convert_move(command)

    def _convert_program_control(self, command: Path.Command) -> str:
        """
        Converts a program control command to gcode.

        This method can be overridden by derived postprocessors to customize program control handling.
        """
        return self._convert_move(command)

    def _convert_fixture(self, command: Path.Command) -> str:
        """
        Converts a fixture command to gcode.

        This method can be overridden by derived postprocessors to customize fixture handling.
        """
        return self._convert_move(command)

    def _convert_modal_command(self, command: Path.Command) -> str:
        """
        Converts a modal command to gcode.

        This method can be overridden by derived postprocessors to customize modal handling.
        """
        return self._convert_move(command)

    def _convert_generic_command(self, command: Path.Command) -> str:
        """
        Converts a generic command to gcode.

        This method can be overridden by derived postprocessors to customize generic handling.
        """
        return self._convert_move(command)


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

        # Get early_tool_prep setting from machine config
        early_tool_prep = False
        if self._machine and hasattr(self._machine, "processing"):
            early_tool_prep = getattr(self._machine.processing, "early_tool_prep", False)

        postables = self._buildPostList(early_tool_prep)
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
