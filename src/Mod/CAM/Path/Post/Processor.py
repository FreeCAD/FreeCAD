# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2014 Yorik van Havre <yorik@uncreated.net>
# SPDX-FileCopyrightText: 2014 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileCopyrightText: 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>
# SPDX-FileCopyrightText: 2024 Ondsel <development@ondsel.com>

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
The base classes for post processors in the CAM workbench.
"""
import argparse
import importlib.util
import json
import os
from PySide import QtCore, QtGui
import re
import sys
from typing import Any, Dict, List, Optional, Tuple, Union
import Constants

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

                except (FileNotFoundError, ImportError, ModuleNotFoundError) as e:
                    Path.Log.debug(f"Failed to load {module_path}: {e}")
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

        Path.Log.warning(
            f"Post processor '{postname}' not found in any search path. "
            f"Searched for '{module_name}.py' in {len(paths)} paths."
        )
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
        # Use centralized command lists from Constants
        all_supported_commands = (
            Constants.GCODE_SUPPORTED + Constants.MCODE_SUPPORTED + Constants.GCODE_NON_CONFORMING
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
                "default": "\n".join(Constants.GCODE_MOVE_DRILL),
                "help": translate(
                    "CAM",
                    "List of drill cycle commands to translate to G0/G1 moves (one per line). "
                    f"Standard drill cycles: {', '.join(Constants.GCODE_MOVE_DRILL)}. "
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
            {
                "name": "show_dialog",
                "type": "bool",
                "label": translate("CAM", "Show Pre-processing Dialogs"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Show interactive dialogs during post-processing. "
                    "Disable for automated operation or testing.",
                ),
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

    def _buildPostList(self):
        """Determine the specific objects and order to postprocess.

        Reads early_tool_prep from self._machine.processing if available.

        Returns:
            List of (name, operations) tuples where every item is a PostList.Postable.
        """
        return PostList.buildPostList(self)

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

    def _apply_schema_defaults(self):
        """Populate postprocessor_properties with schema defaults for missing keys.

        The .fcm file only stores properties explicitly changed by the user.
        Properties not present in the file (e.g. preamble, safetyblock, postamble)
        still have defaults defined in the postprocessor's property schema.

        This method merges those defaults so that export2 and other consumers
        can read them from postprocessor_properties without special-case fallback
        logic.

        Must be called after self._machine is set and before any code reads
        postprocessor_properties.
        """
        if not self._machine:
            return

        schema = self.get_full_property_schema()
        for prop in schema:
            name = prop.get("name", "")
            if name and name not in self._machine.postprocessor_properties:
                default = prop.get("default", "")
                self._machine.postprocessor_properties[name] = default
                Path.Log.debug(f"Schema default applied: {name} = {repr(default)}")

    def build_configuration_bundle(self, overrides=None):
        """Build the complete postprocessor configuration as a flat dict.

        Pure computation with no side effects — suitable for testing.

        Merges in priority order:
          1. Machine postprocessor_properties (from .fcm file)
          2. Schema defaults (for keys not already present)
          3. Overrides (from job or caller)
          4. other job configuration elements (author, comment, selected fixtures)

        If *overrides* is None the overrides are read from
        Job.PostProcessorPropertyOverrides.  If *overrides* is a dict
        it is used directly, allowing the dialog to inject values
        without touching the job object.

        Args:
            overrides: Optional dict of property overrides.

        Returns:
            dict: The final postprocessor property bundle.
        """
        # Stage 1 — machine postprocessor_properties
        bundle = {}
        if self._machine:
            bundle.update(self._machine.postprocessor_properties)

        # Stage 2 — schema defaults for missing keys
        schema = self.get_full_property_schema()
        for prop in schema:
            name = prop.get("name", "")
            if name and name not in bundle:
                bundle[name] = prop.get("default", "")

        # Stage 3 — job configuration elements (defaults from the document/job)
        # These are not schema properties but are needed by the postprocessor.
        # They provide sensible defaults that can be overridden in Stage 4.
        if self._job:
            doc = getattr(self._job, "Document", None)
            bundle.setdefault("job_author", getattr(doc, "CreatedBy", "") if doc else "")
            bundle.setdefault("comment", getattr(self._job, "Description", "") or "")
            bundle.setdefault("selected_fixtures", [])

        # Stage 4 — caller / dialog overrides (highest priority)
        if overrides is None:
            overrides = self._read_job_overrides()

        for key, value in overrides.items():
            if key in bundle:
                bundle[key] = value
            else:
                Path.Log.warning(f"override key '{key}' not in bundle, ignoring")

        return bundle

    def apply_configuration_bundle(self, overrides=None):
        """Build the bundle and apply it everywhere.

        1. Calls build_configuration_bundle() to get the final dict.
        2. Writes the result back to machine.postprocessor_properties.
        3. Merges machine output config into self.values (header, comments, etc.).
        4. Syncs every bundle key into self.values as UPPERCASE so that
           all consumers (G-code generation, sanity checks) read from
           one source of truth.

        Args:
            overrides: Optional dict passed through to build_configuration_bundle().
        """
        bundle = self.build_configuration_bundle(overrides)

        # Write back to machine postprocessor_properties
        if self._machine:
            self._machine.postprocessor_properties.update(bundle)

        # Merge machine output config (header, comments, formatting, etc.)
        self._merge_machine_config()

        # Sync all bundle keys into self.values as UPPERCASE
        for key, value in bundle.items():
            self.values[key.upper()] = value

        Path.Log.debug(f"Configuration bundle applied — " f"bundle: {bundle}")

    # ------------------------------------------------------------------
    # Bundle helpers
    # ------------------------------------------------------------------

    def _read_job_overrides(self):
        """Read overrides dict from Job.PostProcessorPropertyOverrides JSON."""
        if not self._job:
            return {}

        overrides_str = getattr(self._job, "PostProcessorPropertyOverrides", "{}")
        if not overrides_str or overrides_str == "{}":
            return {}

        try:
            overrides = json.loads(overrides_str)
        except (json.JSONDecodeError, TypeError) as e:
            Path.Log.warning(f"Invalid PostProcessorPropertyOverrides JSON: {e}")
            return {}

        if not isinstance(overrides, dict):
            Path.Log.warning("PostProcessorPropertyOverrides is not a dict, ignoring")
            return {}

        return overrides

    # Keep backward-compat aliases so nothing breaks during transition
    def _apply_job_property_overrides(self):
        self.apply_configuration_bundle()

    def _apply_overrides(self, overrides):
        self.apply_configuration_bundle(overrides)

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
                if self._machine.output.header.include_description:
                    description = self.values.get("COMMENT", "")
                    if description:
                        gcodeheader.add_description(description)

                # Add author if enabled
                author = self.values.get("JOB_AUTHOR", "")
                if author:
                    gcodeheader.add_author(author)

                # Add date/time if enabled
                if self._machine.output.header.include_date:
                    import datetime

                    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    gcodeheader.add_output_time(timestamp)

        # Collect tool and fixture info from postables.
        # Tool/fixture lists are part of the header block — gate on header_enabled,
        # not OUTPUT_HEADER (which tracks include_date and is a different field).
        if header_enabled:
            list_tools = True
            if (
                self._machine
                and hasattr(self._machine, "output")
                and hasattr(self._machine.output, "list_tools_in_header")
            ):
                list_tools = self._machine.output.list_tools_in_header

            list_fixtures = True
            if (
                self._machine
                and hasattr(self._machine, "output")
                and hasattr(self._machine.output, "list_fixtures_in_header")
            ):
                list_fixtures = self._machine.output.list_fixtures_in_header

            seen_tools = set()
            seen_fixtures = set()
            for section_name, sublist in postables:
                for item in sublist:
                    if item.item_type == "tool_controller":
                        if list_tools:
                            tool_key = item.data["tool_number"]
                            if tool_key not in seen_tools:
                                seen_tools.add(tool_key)
                                gcodeheader.add_tool(tool_key, item.label)
                    elif item.item_type == "fixture":
                        if list_fixtures:
                            if item.path and item.path.Commands:
                                fixture_name = item.path.Commands[0].Name
                                if fixture_name not in seen_fixtures:
                                    seen_fixtures.add(fixture_name)
                                    gcodeheader.add_fixture(fixture_name)

        return gcodeheader

    def _expand_canned_cycles(self, postables):
        """Terminate canned drill cycles in postable paths.

        Adds cycle termination commands (G80) after canned cycle sequences.
        Drill cycle translation is handled separately by the postprocessor
        via the drill_cycles_to_translate property.

        Subclasses can override to customize canned cycle handling.
        """
        Path.Log.track("Expanding canned cycles")
        for section_name, sublist in postables:
            for item in sublist:
                has_drill_cycles = False
                if item.path:
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
                    has_drill_cycles = any(cmd.Name in drill_commands for cmd in item.path.Commands)

                if has_drill_cycles:
                    item.path = PostUtils.cannedCycleTerminator(item.path)

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
                if item.path:
                    item.path = PostUtils.splitArcs(item.path)

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
                if item.path:
                    new_commands = []
                    for cmd in item.path.Commands:
                        new_commands.append(cmd)
                        if cmd.Name in Constants.MCODE_SPINDLE_ON:
                            pause_cmd = Path.Command("G4", {"P": wait_time})
                            new_commands.append(pause_cmd)
                    item.path = Path.Path(new_commands)

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
                if item.path:
                    new_commands = []
                    for cmd in item.path.Commands:
                        new_commands.append(cmd)
                        if cmd.Name in Constants.MCODE_COOLANT_ON:
                            pause_cmd = Path.Command("G4", {"P": spindle.coolant_delay})
                            new_commands.append(pause_cmd)
                    item.path = Path.Path(new_commands)

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
                if item.path:
                    new_commands = []
                    Path.Log.debug(f"Translating rapid moves for {item.label}")
                    for cmd in item.path.Commands:
                        if cmd.Name in Constants.GCODE_MOVE_RAPID:
                            cmd.Name = "G1"
                        new_commands.append(cmd)
                    item.path = Path.Path(new_commands)

    def _expand_translate_drill_cycles(self, postables):
        """Translate canned drill cycles to G0/G1 move sequences.

        When machine processing.translate_drill_cycles is True, expands
        canned drill cycle commands (G73, G81, G82, G83) into equivalent
        G0/G1 move sequences using the DrillCycleExpander.

        Subclasses can override to customize drill cycle translation.
        """
        Path.Log.track("Translating drill cycles")
        if not (
            self._machine
            and hasattr(self._machine, "processing")
            and self._machine.processing.translate_drill_cycles
        ):
            Path.Log.debug("Drill cycle translation disabled")
            return

        from Path.Post.DrillCycleExpander import DrillCycleExpander

        for section_name, sublist in postables:
            for item in sublist:
                Path.Log.track(f"Processing item: {item.label}")
                if item.path:
                    has_drill = any(
                        cmd.Name in DrillCycleExpander.EXPANDABLE_CYCLES
                        for cmd in item.path.Commands
                    )
                    if has_drill:
                        Path.Log.debug(f"Translating drill cycles for {item.label}")
                        expander = DrillCycleExpander()
                        item.path = expander.expand_path(item.path)

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
                if item.item_type == "tool_controller":
                    tool_change_seen = True
                    Path.Log.debug(f"Tool change detected: T{item.data['tool_number']}")
                    continue

                if item.path:
                    new_commands = []
                    first_move_processed = False

                    for cmd in item.path.Commands:
                        # Check if this is a tool change command (M6)
                        if cmd.Name in Constants.MCODE_TOOL_CHANGE:
                            new_commands.append(cmd)
                            tool_change_seen = True
                            first_move_processed = False
                            Path.Log.debug("M6 tool change detected in operation")
                            continue

                        # Check if this is the first move after tool change
                        if (
                            tool_change_seen
                            and not first_move_processed
                            and cmd.Name in Constants.GCODE_MOVE_ALL
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

                    if len(new_commands) != len(item.path.Commands):
                        item.path = Path.Path(new_commands)
                        Path.Log.debug(f"Updated path for {item.label}")

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
                    if item.item_type == "operation":
                        bcnc_start_cmd = Path.Command("(Block-name: " + item.label + ")")
                        bcnc_start_cmd.Annotations = {"bcnc": "block_start"}

                        bcnc_expand_cmd = Path.Command("(Block-expand: 0)")
                        bcnc_expand_cmd.Annotations = {"bcnc": "block_meta"}

                        bcnc_enable_cmd = Path.Command("(Block-enable: 1)")
                        bcnc_enable_cmd.Annotations = {"bcnc": "block_meta"}

                        if item.path:
                            original_commands = list(item.path.Commands)
                            new_commands = [bcnc_start_cmd, bcnc_expand_cmd, bcnc_enable_cmd]
                            new_commands.extend(original_commands)
                            item.path = Path.Path(new_commands)
        else:
            Path.Log.debug("Removing existing bCNC commands")
            for section_name, sublist in postables:
                for item in sublist:
                    if item.item_type == "operation" and item.path:
                        filtered_commands = []
                        for cmd in item.path.Commands:
                            if not (
                                cmd.Name.startswith("(Block-name:")
                                or cmd.Name.startswith("(Block-expand:")
                                or cmd.Name.startswith("(Block-enable:")
                            ):
                                filtered_commands.append(cmd)

                        if len(filtered_commands) != len(item.path.Commands):
                            item.path = Path.Path(filtered_commands)

    def _expand_tool_length_offset(self, postables):
        """Inject or remove G43 tool length offset commands.

        When OUTPUT_TOOL_LENGTH_OFFSET is True, adds G43 commands after M6
        tool change commands in operations and tool change items.

        When OUTPUT_TOOL_LENGTH_OFFSET is False, removes any existing G43
        commands from operation paths.

        Simplified single-pass implementation.
        """
        output_tool_length_offset = self.values.get("OUTPUT_TOOL_LENGTH_OFFSET", True)
        Path.Log.debug(f"OUTPUT_TOOL_LENGTH_OFFSET value: {output_tool_length_offset}")

        # Clear tracking dictionaries
        self._tool_change_g43_commands = {}
        self._suppress_tool_change_m6 = set()

        if not output_tool_length_offset:
            return

        Path.Log.debug("Creating G43 tool length offset commands")
        for section_name, sublist in postables:
            for item in sublist:
                Path.Log.debug(f"Processing item: {item.item_type}")
                if item.item_type == "tool_controller" and item.path:
                    commands_with_g43 = []
                    for cmd in item.path.Commands:
                        commands_with_g43.append(cmd)
                        if cmd.Name in ("M6", "M06") and "T" in cmd.Parameters:
                            tool_num = cmd.Parameters["T"]
                            g43_cmd = Path.Command("G43", {"H": tool_num})
                            g43_cmd.Annotations = {"tool_length_offset": True}
                            commands_with_g43.append(g43_cmd)
                            Path.Log.debug(
                                f"Added G43 H{tool_num} after M6 in operation {item.label}"
                            )

                    if len(commands_with_g43) != len(item.path.Commands):
                        item.path = Path.Path(commands_with_g43)

    def _get_property_lines(self, key: str) -> list:
        """Return non-empty lines from a postprocessor_properties entry."""
        if self._machine and self._machine.postprocessor_properties.get(key):
            return [
                line
                for line in self._machine.postprocessor_properties[key].split("\n")
                if line.strip()
            ]
        return []

    def _collect_header_lines(self, gcodeheader) -> list:
        """Build header comment lines from the gcodeheader object.

        Gated on machine.output.output_header. Returns formatted comment
        strings using the configured COMMENT_SYMBOL.
        """
        header_enabled = True
        if self._machine and hasattr(self._machine, "output"):
            header_enabled = self._machine.output.output_header

        header_lines = []
        if header_enabled:
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
        return header_lines

    def _collect_preamble_lines(self) -> list:
        """Return preamble lines from machine configuration."""
        return self._get_property_lines("preamble")

    def _collect_unit_command(self) -> list:
        """Return G20/G21 unit command based on output_units setting."""
        if self._machine and hasattr(self._machine, "output"):
            from Machine.models.machine import OutputUnits

            if self._machine.output.units == OutputUnits.METRIC:
                return ["G21"]
            elif self._machine.output.units == OutputUnits.IMPERIAL:
                return ["G20"]
        return []

    def _collect_pre_job_lines(self) -> list:
        """Return pre-job lines from machine configuration."""
        return self._get_property_lines("pre_job")

    def _build_section_prefix(
        self, header_lines, preamble_lines, unit_command, pre_job_lines
    ) -> list:
        """Assemble the per-section prefix lines.

        Each section becomes a separate output file and must be
        self-contained.
        """
        gcode_lines = []
        gcode_lines.extend(header_lines)
        gcode_lines.extend(preamble_lines)
        gcode_lines.extend(unit_command)
        gcode_lines.extend(pre_job_lines)
        return gcode_lines

    def _emit_item_pre_block(self, item, gcode_lines) -> bool:
        """Emit pre-block lines for a postable item based on its type.

        Handles tool_controller, fixture, and operation item types.
        Derived postprocessors can override to customize pre-block
        behavior.

        Returns True if the item should be skipped (no further
        processing), False to continue with command conversion and
        post-blocks.
        """
        if item.item_type == "tool_controller":
            if self._machine and hasattr(self._machine, "processing"):
                if not self._machine.processing.tool_change:
                    comment_symbol = self.values.get("COMMENT_SYMBOL", "(")
                    tool_num = item.data["tool_number"]
                    if comment_symbol == "(":
                        gcode_lines.append(f"(Tool change suppressed: M6 T{tool_num})")
                    else:
                        gcode_lines.append(
                            f"{comment_symbol} Tool change suppressed:" f" M6 T{tool_num}"
                        )
                    return True
            gcode_lines.extend(self._get_property_lines("pre_tool_change"))

        elif item.item_type == "fixture":
            gcode_lines.extend(self._get_property_lines("pre_fixture_change"))

        elif item.item_type == "operation":
            gcode_lines.extend(self._get_property_lines("pre_operation"))

        return False

    def _convert_item_commands(self, item, gcode_lines) -> None:
        """Convert Path.Commands to G-code strings for a single item.

        Tracks rotary move groups and inserts pre/post rotary blocks.
        Handles M6 tool change suppression when tool_change is disabled.
        """
        if not item.path:
            return

        in_rotary_group = False

        for cmd in item.path.Commands:
            try:
                has_rotary = any(param in cmd.Parameters for param in ["A", "B", "C"])

                if has_rotary and not in_rotary_group:
                    gcode_lines.extend(self._get_property_lines("pre_rotary_move"))
                    in_rotary_group = True
                elif not has_rotary and in_rotary_group:
                    gcode_lines.extend(self._get_property_lines("post_rotary_move"))
                    in_rotary_group = False

                gcode = self.convert_command_to_gcode(cmd)

                if cmd.Name in ("M6", "M06"):
                    if (
                        self._machine
                        and hasattr(self._machine, "processing")
                        and not self._machine.processing.tool_change
                    ):
                        comment_symbol = self.values.get("COMMENT_SYMBOL", "(")
                        if comment_symbol == "(":
                            gcode = f"(Tool change suppressed: {gcode})"
                        else:
                            gcode = f"{comment_symbol} Tool change" f" suppressed: {gcode}"

                if gcode is not None and gcode.strip():
                    gcode_lines.append(gcode)

            except (ValueError, AttributeError) as e:
                Path.Log.debug(f"Skipping command {cmd.Name}: {e}")

        if in_rotary_group:
            gcode_lines.extend(self._get_property_lines("post_rotary_move"))

    def _emit_item_post_block(self, item, gcode_lines) -> None:
        """Emit post-block lines for a postable item based on its type.

        Handles tool_controller, fixture, and operation item types.
        Derived postprocessors can override to customize post-block
        behavior.
        """
        if item.item_type == "tool_controller":
            gcode_lines.extend(self._get_property_lines("post_tool_change"))
            gcode_lines.extend(self._get_property_lines("tool_return"))
        elif item.item_type == "fixture":
            gcode_lines.extend(self._get_property_lines("post_fixture_change"))
        elif item.item_type == "operation":
            gcode_lines.extend(self._get_property_lines("post_operation"))

    def _optimize_gcode(self, header_lines, gcode_lines) -> str:
        """Apply G-code optimizations and produce a final string.

        Separates header comments from body, applies deduplication,
        redundant-axis suppression, inefficient-move filtering, and
        line numbering to the body only, then reassembles with the
        configured line ending.
        """
        from Path.Post.GcodeProcessingUtils import (
            deduplicate_repeated_commands,
            suppress_redundant_axes_words,
            filter_inefficient_moves,
            insert_line_numbers,
        )

        if not gcode_lines:
            return ""

        num_header_lines = len(header_lines)
        header_part = gcode_lines[:num_header_lines]
        body_part = gcode_lines[num_header_lines:]

        if body_part:
            if not self.values.get("OUTPUT_DUPLICATE_COMMANDS", True):
                body_part = deduplicate_repeated_commands(body_part)
            if not self.values.get("OUTPUT_DOUBLES", True):
                body_part = suppress_redundant_axes_words(body_part)

        if body_part and self._machine and hasattr(self._machine, "processing"):
            if hasattr(self._machine.processing, "filter_inefficient_moves"):
                if self._machine.processing.filter_inefficient_moves:
                    body_part = filter_inefficient_moves(body_part)

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

        final_lines = header_part + body_part
        gcode_with_newlines = "\n".join(final_lines)

        line_ending = self.values.get("END_OF_LINE_CHARS", "\n")
        if line_ending == "\n":
            return gcode_with_newlines
        else:
            return gcode_with_newlines.replace("\n", line_ending)

    def _append_trailing_lines(self, gcode_string) -> str:
        """Append post_job and postamble lines to a gcode section."""
        trailing = []
        trailing.extend(self._get_property_lines("post_job"))
        trailing.extend(self._get_property_lines("postamble"))

        if trailing:
            trailing_str = "\n".join(trailing)
            line_ending = self.values.get("END_OF_LINE_CHARS", "\n")
            if line_ending == "\n":
                gcode_string = gcode_string + "\n" + trailing_str
            else:
                gcode_string = gcode_string + line_ending + trailing_str.replace("\n", line_ending)
        return gcode_string

    def _append_bcnc_postamble(self, job_sections) -> None:
        """Append bCNC postamble to the last section only.

        bCNC postamble tracks global state across all sections; proper
        per-section bCNC support in split mode requires per-section
        tracking (future work).
        """
        if (
            job_sections
            and hasattr(self, "_bcnc_postamble_commands")
            and self._bcnc_postamble_commands is not None
        ):
            Path.Log.debug(
                f"Processing {len(self._bcnc_postamble_commands)}" " bCNC postamble commands"
            )
            bcnc_lines = []
            for cmd in self._bcnc_postamble_commands:
                gcode = self.convert_command_to_gcode(cmd)
                if gcode is not None and gcode.strip():
                    bcnc_lines.append(gcode)
            if bcnc_lines:
                last_name, last_gcode = job_sections[-1]
                line_ending = self.values.get("END_OF_LINE_CHARS", "\n")
                bcnc_gcode = "\n".join(bcnc_lines)
                if line_ending == "\n":
                    job_sections[-1] = (
                        last_name,
                        last_gcode + "\n" + bcnc_gcode,
                    )
                else:
                    job_sections[-1] = (
                        last_name,
                        last_gcode + line_ending + bcnc_gcode.replace("\n", line_ending),
                    )
        else:
            Path.Log.debug("No bCNC postamble commands to process")

    def _prepend_safety_block(self, all_job_sections) -> None:
        """Prepend safetyblock to the first section if configured."""
        if not all_job_sections:
            return

        safety_lines = self._get_property_lines("safetyblock")
        if not safety_lines:
            return

        safety_gcode_newlines = "\n".join(safety_lines)
        line_ending = self.values.get("END_OF_LINE_CHARS", "\n")

        if line_ending == "\n":
            safety_gcode = safety_gcode_newlines + "\n"
        else:
            safety_gcode = safety_gcode_newlines.replace("\n", line_ending) + line_ending

        first_name, first_gcode = all_job_sections[0]
        all_job_sections[0] = (first_name, safety_gcode + first_gcode)

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
        Path.Log.debug("Starting export2()")

        # ===== STAGE 0: PRE-PROCESSING DIALOG =====
        if not self.pre_processing_dialog():
            Path.Log.info("Pre-processing dialog cancelled - aborting export")
            return None

        if not getattr(self, "_bundle_applied", False):
            self.apply_configuration_bundle()

        # ===== STAGE 1: ORDERING =====
        all_job_sections = []
        postables = self._buildPostList()
        self._expand_postprocessor_commands(postables)

        # ===== STAGE 2: COMMAND EXPANSION =====
        gcodeheader = self._build_header(postables)
        self._expand_translate_drill_cycles(postables)
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
        header_lines = self._collect_header_lines(gcodeheader)
        preamble_lines = self._collect_preamble_lines()
        unit_command = self._collect_unit_command()
        pre_job_lines = self._collect_pre_job_lines()

        job_sections = []
        for section_name, sublist in postables:
            gcode_lines = self._build_section_prefix(
                header_lines, preamble_lines, unit_command, pre_job_lines
            )

            for item in sublist:
                if self._emit_item_pre_block(item, gcode_lines):
                    continue
                self._convert_item_commands(item, gcode_lines)
                self._emit_item_post_block(item, gcode_lines)

            # ===== STAGE 4: G-CODE OPTIMIZATION =====
            gcode_string = self._optimize_gcode(header_lines, gcode_lines)
            if gcode_string:
                gcode_string = self._append_trailing_lines(gcode_string)
                job_sections.append((section_name, gcode_string))

        self._append_bcnc_postamble(job_sections)
        all_job_sections.extend(job_sections)

        # ===== STAGE 5: OUTPUT PRODUCTION =====
        self._prepend_safety_block(all_job_sections)

        Path.Log.debug(f"Returning {len(all_job_sections)} sections")
        Path.Log.debug(f"Sections: {all_job_sections}")

        # ===== STAGE 6: REMOTE POSTING =====
        try:
            self.remote_post(all_job_sections)
        except Exception as e:
            Path.Log.error(f"Remote posting failed: {e}")

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

        postables = self._buildPostList()
        Path.Log.debug(f"postables {postables}")

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

    def _expand_postprocessor_commands(self, postables):
        """
        Hook for derived postprocessors to transform postables before command expansion.

        Called after Stage 1 (ordering) and before Stage 2 (command expansion).
        Derived postprocessors can override this to inject, remove, or modify
        Path commands in the postables list before they are converted to G-code.

        Args:
            postables: List of (section_name, items) tuples
        """
        pass

    def pre_processing_dialog(self):
        """
        Hook for pre-processing dialog functionality.

        This method is called before any post-processing begins, allowing post processors
        to collect user input through dialogs or other interactive means.
        Base implementation does nothing, but subclasses can override to implement
        configuration dialogs, validation checks, or user input collection.

        When the unified PostProcessDialog is used, it sets _dialog_handled = True
        on the postprocessor instance before calling export2(), so this method
        returns True immediately (the dialog already collected user input).

        Returns:
            bool: True to continue with post-processing, False to cancel
        """
        if getattr(self, "_dialog_handled", False):
            Path.Log.debug("pre_processing_dialog skipped (handled by unified dialog)")
            return True
        return True

    def get_sanity_checks(self, job):
        """
        Hook for postprocessor-specific sanity checks.

        This method allows postprocessors to define custom validation rules
        specific to their machine capabilities, configuration requirements,
        or operational constraints. These checks are integrated into the
        CAM_QuickValidation system and displayed alongside generic checks.

        Args:
            job: FreeCAD CAM job object to validate

        Returns:
            list: List of squawk dictionaries following the same format as CAMSanity
                  Each squawk should have: Date, Operator, Note, squawkType, squawkIcon

        Example:
            def get_sanity_checks(self, job):
                squawks = []

                # Check plasma cutter specific settings
                if self.values.get('pierce_delay', 0) < 300:
                    squawks.append(self._create_squawk(
                        "WARNING",
                        "Pierce delay may be too short for material piercing"
                    ))

                return squawks
        """
        return []  # Default implementation: no custom checks

    def _create_squawk(self, squawk_type, note):
        """
        Helper to create squawk dictionaries in CAMSanity format.

        Args:
            squawk_type: str - One of "NOTE", "WARNING", "CAUTION", "TIP"
            note: str - Human-readable message

        Returns:
            dict: Squawk dictionary compatible with CAMSanity
        """
        from datetime import datetime

        # Map to same icons used by CAMSanity
        icon_map = {
            "TIP": "Sanity_Bulb",
            "NOTE": "Sanity_Note",
            "WARNING": "Sanity_Warning",
            "CAUTION": "Sanity_Caution",
        }

        return {
            "Date": datetime.now().strftime("%c"),
            "Operator": self.__class__.__name__,
            "Note": note,
            "squawkType": squawk_type,
            "squawkIcon": f"{FreeCAD.getHomePath()}Mod/CAM/Path/Main/Sanity/{icon_map.get(squawk_type, 'Sanity_Note')}.svg",
        }

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

        # Validate command is supported
        supported = Constants.GCODE_SUPPORTED + Constants.GCODE_FIXTURES + Constants.MCODE_SUPPORTED
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
        if command_name in Constants.GCODE_MOVE_RAPID:
            return self._convert_rapid_move(command)

        # Linear moves
        if command_name in Constants.GCODE_MOVE_STRAIGHT:
            return self._convert_linear_move(command)

        # Arc moves
        if command_name in Constants.GCODE_MOVE_ARC:
            return self._convert_arc_move(command)

        # Drill cycles
        if command_name in Constants.GCODE_MOVE_DRILL + Constants.GCODE_DRILL_EXTENDED:
            return self._convert_drill_cycle(command)

        # Probe
        if command_name in Constants.GCODE_PROBE:
            return self._convert_probe(command)

        # Dwell
        if command_name in Constants.GCODE_DWELL:
            return self._convert_dwell(command)

        # Tool change
        if command_name in Constants.MCODE_TOOL_CHANGE:
            return self._convert_tool_change(command)

        # Spindle control
        if command_name in Constants.MCODE_SPINDLE_ON + Constants.MCODE_SPINDLE_OFF:
            return self._convert_spindle_command(command)

        # Coolant control
        if command_name in Constants.MCODE_COOLANT:
            return self._convert_coolant_command(command)

        # Program control
        if (
            command_name
            in Constants.MCODE_STOP
            + Constants.MCODE_OPTIONAL_STOP
            + Constants.MCODE_END
            + Constants.MCODE_END_RESET
        ):
            return self._convert_program_control(command)

        # Fixtures
        if command_name in Constants.GCODE_FIXTURES:
            return self._convert_fixture(command)

        # Modal commands (G43, G80, G90, G91, G92, G93, G94, G95, G96, G97, G98, G99, etc.)
        if (
            command_name
            in Constants.GCODE_TOOL_LENGTH_OFFSET
            + Constants.GCODE_CYCLE_CANCEL
            + Constants.GCODE_DISTANCE_MODE
            + Constants.GCODE_OFFSET
            + Constants.GCODE_FEED_INVERSE_TIME
            + Constants.GCODE_FEED_UNITS_PER_MIN
            + Constants.GCODE_FEED_UNITS_PER_REV
            + Constants.GCODE_SPINDLE_CSS
            + Constants.GCODE_SPINDLE_RPM
            + Constants.GCODE_RETURN_MODE
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
        command_line = []
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
