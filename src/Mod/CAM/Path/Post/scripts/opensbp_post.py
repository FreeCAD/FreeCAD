# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
# *   Copyright (c) 2025 Alan Grover <awgrover@gmail.com>                   *
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
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""FreeCAD CAM post-processor for Shopbot native opensbb, in the "refactored" style"""

import re
import argparse
from copy import copy
import operator
import math
import time

from typing import Any

from Path.Post.Processor import PostProcessor
import Path.Post.UtilsArguments as PostUtilsArguments
import Path.Post.UtilsExport as PostUtilsExport
import Path.Post.UtilsParse as PostUtilsParse

import Path
import FreeCAD
import FreeCADGui

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

#
# Define some types that are used throughout this file.
#
Defaults = dict[str | bool]
FormatHelp = str
GCodeOrNone = [str | None]
GCodeSections = list[tuple[str, GCodeOrNone]]
Parser = argparse.ArgumentParser
ParserArgs = [None | str | argparse.Namespace]
Postables = [list | list[tuple[str, list]]]
Section = tuple[str, list]
Sublist = list
Units = str
Values = dict[str, Any]
Visible = dict[str, bool]
nl = "\n"  # particularly useful in a f-string
PreventGuiTimeout = 1  # seconds

SpeedPrecisions = {"G20": 2, "G21": 1}  # for in/sec, for mm/sec
MinSpeeds = {"G20": 0.05, "G21": 1.3}  # for in/sec, for mm/sec


class Opensbp(PostProcessor):
    """For ShopBot (or other opensbp controllers), this is a CAM postprocessor.
    We have to translate gcode to opensbp, so the output is NOT gcode.
    Typically, you'd want a .sbp file extension.

    Special behaviors:
    * Many defaults have changed: read the tooltip for the Arguments
    * This always tests the ShopBot app/machine units (see --inches --metric), and on the machine will give an error and exit if in the wrong units. No more mismatches.
    * G54 (fixture/coordinate-system) is accepted, but is a noop (because it's the default in Operations). Others (G55 etc) are not-accepted.
    * Note the default of --tool-change == False. Use that for manual tool change, but you only get 1 tool per file. True means call C9. Note: C9 seems to not work when there is no auto-tool-changer (corrupts units and erases your settings file!)
    * This does arc gcodes (G02 and G03), so helical operations should work.
    * This does Probe (G38.2), defaults to the same folder as the .sbp, and .txt as the extension for output filename
    * Relative movement (G91) is not supported.
    * opensbp commands can be in-lined via a comment:
            (MC_RUN_COMMAND: <opensbpcommand>)
    * By default, this post-processor fails if it sees a gcode it doesn't understand. but see --abort-on-unknown and --allow-unknown
    * Do opensbp prompt (dialog-box) behavior like this:
        (this comment becomes a dialog-box if followed by M00)
        M00
    * --modal only applies to G00 and G01. Notably NOT to arcs.
    * Note that --preamble occurs before the machine-units are set, could be a problem! (not under our control)
    # preamble/postamble are G-Code.
    * Most optimizations assume that the instruction stream isn't interrupted with manually added commands (e.g. assumes the xyz position from the previous instructions). So, don't stop, and insert commands if you've used any of the optimizations.
    * This always sets the move & jog speeds (feed and rapid) at the beginning of each operation, by fetching the Tool speeds. If rapid-speeds are 0.0, it will not set the jog-speed, so will use the machine setting.
    * For spindle speed changes, your ShopBot C6 "cut" command should be configured for having-a-speed-control-module, or for not having. It will prompt you if configured for not-having the module.
    * The A and B axis are not supported
    """

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "see property below"),
        tooltipargs=[""],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        self.reinitialize()
        Path.Log.debug("Refactored opensbp post processor initialized.")

    def reinitialize(self) -> None:
        """Initialize or reinitialize the 'core' data structures for the postprocessor."""
        #
        # This is also used to reinitialize the data structures between tests.
        #
        self.values: Values = {}
        self.init_values(self.values)
        # debug_str = "\n".join(f"{k}:{self.values[k]}" for k in sorted(self.values.keys())); print(f"### .values\n{debug_str}")

        self.argument_defaults: Defaults = {}
        self.init_argument_defaults(self.argument_defaults)
        Path.Log.debug(f".argument_defaults\n{self.argument_defaults}")
        self.arguments_visible: Visible = {}
        self.init_arguments_visible(self.arguments_visible)
        self.parser: Parser = self.init_arguments(
            self.values, self.argument_defaults, self.arguments_visible
        )
        self.arguments = None  # the parser.parse_args result

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

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        PostUtilsArguments.init_shared_values(values)
        #
        # Set any values here that need to override the default values set
        # in the init_shared_values routine.
        #

        # Used in the argparser code as the "name" of the postprocessor program.
        values["MACHINE_NAME"] = "opensbp"

        values.update(
            {
                "DRILL_CYCLES_TO_TRANSLATE": ["G73", "G81", "G82", "G83", "G85"],
                "ENABLE_COOLANT": False,
                "ENABLE_MACHINE_SPECIFIC_COMMANDS": True,
                "LINE_INCREMENT": 1,
                "OUTPUT_PATH_LABELS": True,
                "OUTPUT_TOOL_CHANGE": False,
                # 'PARAMETER_ORDER' : don't care about order, we aren't gcode
                "POSTAMBLE": "",
                "POSTPROCESSOR_FILE_NAME": __name__,
                "PREAMBLE": "",
                "SPINDLE_WAIT": 3,  # for auto case
                "STOP_SPINDLE_FOR_TOOL_CHANGE": True,
                "SUPPRESS_COMMANDS": [
                    "G54"
                ],  # we don't have coord-systems (yet) # G99,G98,G80 added automatically
                # 'TOOL_CHANGE' : we have to generate this dynamically
                "TRANSLATE_DRILL_CYCLES": True,
                "UNITS": self._units,
                "UNIT_SPEED_FORMAT": "mm/s",
                "USE_TLO": False,
                "line_number": 1,
                "ALLOW_UNKNOWN": [],  # --allow-unknown
                "last_command": None,
                "first_probe": True,
                "SPEED_PRECISION": 2,  # updated at process_arguments
                "MIN_SPEED": 0.05,  # updated at process_arguments
            }
        )
        # FIXME: should be done by PostProcessor, isn't there yet in 1.0.
        if "G38.2" not in self.values["MOTION_COMMANDS"]:
            self.values["MOTION_COMMANDS"].append("G38.2")

    def init_argument_defaults(self, argument_defaults: Defaults) -> None:
        """Initialize which arguments (in a pair) are shown as the default argument."""
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
        PostUtilsArguments.init_argument_defaults(argument_defaults)
        #
        # Note:  You also need to modify the corresponding entries in the "values" hash
        #        to actually make the default value(s) change to match.
        #

        argument_defaults["tlo"] = False
        # argument_defaults['metric_inches'] = True if From doc
        argument_defaults["translate_drill"] = True
        argument_defaults["tool_change"] = False
        argument_defaults["wait-for-spindle"] = self.values["SPINDLE_WAIT"]

    def init_arguments_visible(self, arguments_visible: Visible) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        PostUtilsArguments.init_arguments_visible(arguments_visible)
        #
        # Modify the visibility of any arguments from the defaults here.
        #
        arguments_visible.update(
            {
                k: False
                for k in (
                    "bcnc",
                    "tlo",
                    "translate_drill",
                    "line-numbers",
                    "gcode-comments",  # FIXME: doesn't hide "our" arguments
                )
            }
        )
        arguments_visible.update(
            {
                k: True
                for k in (
                    "tool_change",
                    "return_to",
                    "line-numbers",
                    "wait-for-spindle",
                )
            }
        )

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
        # Add any argument definitions that are not shared with all other
        # postprocessors here.
        #
        _parser.add_argument(
            "--o1",
            action="store_true",
            help="turns on optimizations that wouldn't break if you interrupt the execution and did some command manually: --no-comments --no-header",
        )  # no such optimizations at this time
        _parser.add_argument("--o2", action="store_true", help="turns on --modal --axis-modal")

        _parser.add_argument(
            "--o3",
            action="store_true",
            help="turns on --no-comments --no-header --modal --axis-modal",
        )
        # _parser.add_argument(
        #    # this should probably be True for most shopbot installations
        #    "--ab-is-distance", action="store_true", help="A & B axis are distances, default=degrees"
        # )
        """ # FIXME: not-implemented
        _parser.add_argument(
            "--filter",
            "--filters",
            help="a ',' list of filters in FreeCAD.getUserMacroDir()/post to run on the gcode of each Path object, before we see it (i.e. cleanups). A class of same (camelcase) name as file, __init__(self,objectslist, filename, argstring), .filter(eachpathobj, its-.Commands) -> gcode",
        )
        """
        _parser.add_argument(
            "--abort-on-unknown",
            action=argparse.BooleanOptionalAction,
            help="Generate an error and fail if an unknown gcode is seen. default=True",
            default=True,
        )
        _parser.add_argument(
            "--allow-unknown",
            help="if --abort-on-unknown, allow these gcodes, but change them to a comment. E.g. --allow-unknown G55,G56. Always includes G54,G99,G98,G80",
        )
        _parser.add_argument(
            "--native-rapid-fallback",
            action=argparse.BooleanOptionalAction,
            help="Use machine's rapid speeds if ToolController rapid speeds are zero, default=true. --no-native-rapid-fallback causes error on zeros.",
            default=True,
        )
        _parser.add_argument(
            "--gcode-comments",
            action=argparse.BooleanOptionalAction,
            # help="Add the original gcode as a comment, for debugging",
            help=argparse.SUPPRESS,
            default=False,
        )

        return _parser

    def process_arguments(self) -> tuple[bool, ParserArgs]:
        """Process any arguments to the postprocessor."""
        #
        # This function is separated out to make it easier to inherit from this postprocessor
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
            self.values["UNIT_SPEED_FORMAT"] = (
                "mm/s" if self.values["UNIT_FORMAT"] == "mm" else "in/s"
            )
            self.values["SPEED_PRECISION"] = SpeedPrecisions[self.values["UNITS"]]
            self.values["MIN_SPEED"] = MinSpeeds[self.values["UNITS"]]

            if args.allow_unknown:
                self.values["SUPPRESS_COMMANDS"].extend(
                    # and canonicalize
                    [
                        re.sub(r"^([A-Z])(\d(\.|$))", r"\g<1>0\2", g)
                        for g in args.allow_unknown.split(",")
                    ]
                )

            # too late for .values, so do them by hand
            arg_sets = {
                "o1": {
                    "cli": {
                        "comments": False,
                        "no_comments": True,
                        "no_header": True,
                        "header": False,
                    },
                    "values": {"OUTPUT_HEADER": False, "OUTPUT_COMMENTS": False},
                },
                "o2": {
                    "cli": {
                        "modal": True,
                        "no_modal": False,
                        "axis_modal": True,
                        "no_axis_modal": False,
                    },
                    "values": {
                        "MODAL": True,
                        "OUTPUT_DOUBLES": False,
                    },
                },
                "o3": {
                    "cli": {
                        "comments": False,
                        "no_comments": True,
                        "no_header": True,
                        "header": False,
                        "modal": True,
                        "no_modal": False,
                        "axis_modal": True,
                        "no_axis_modal": False,
                    },
                    "values": {
                        "OUTPUT_HEADER": False,
                        "OUTPUT_COMMENTS": False,
                        "MODAL": True,
                        "OUTPUT_DOUBLES": False,
                    },
                },
            }

            for opt in ("o1", "o2", "o3"):
                if getattr(args, opt, None):
                    cli_set = arg_sets[opt]["cli"]
                    value_set = arg_sets[opt]["values"]
                    for arg_name, value in cli_set.items():
                        Path.Log.debug(f"--{opt} set {arg_name}={value}")
                        setattr(args, arg_name, value)
                    for value_key, value in value_set.items():
                        self.values[value_key] = value

        #
        # If the flag is False, then args is either None (indicating an error while
        # processing the arguments) or a string containing the argument list formatted
        # for output.  Either way the calling routine will need to handle the args value.
        #
        return (flag, args)

    def process_postables(self) -> GCodeSections:
        """Postprocess the 'postables' in the job to g code sections."""
        #
        # This function is separated out to make it easier to inherit from this postprocessor.
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
        # a section seems to be for one output file of gcode (a "split")
        for _, section in enumerate(postables):
            # partname is "allitems" if not splitting, and the operation/etc. if splitting
            # sublist is each "phase" of the operation, like Fixture, ToolController, Operation
            partname, sublist = section

            # We have to lie about self.values to get some expanded g-code that we rely on
            # e.g. we rely on comments
            was_values = copy(self.values)
            self.values.update(
                {
                    "MODAL": False,  # if true, this would elide the gcode "command", which screws us up. so, we do it later.
                    "OUTPUT_COMMENTS": True,  # we use this to detect operation and...
                    "OUTPUT_TOOL_CHANGE": True,  # we need this to ensure speeds
                    "SHOW_MACHINE_UNITS": True,  # we have to set the machine
                    "AXIS_PRECISION": 5,  # we do calculations, so more precision to prevent rounding errors
                    "FEED_PRECISION": 5,
                    "SPINDLE_WAIT": 0,  # we'll do the logic in the right place later
                }
            )
            try:
                # We get back a processed (expanded) set of gcode
                # things like pre/post amble, canned-drill expansion, coolant on/off
                # We could note where each `partname` begins, if we need that
                #   but we get no access to where each `sublist` starts/ends
                # `gcode` is a multi-line string
                gcode = PostUtilsExport.export_common(self.values, sublist, "-")
            finally:
                self.values = was_values

            # print(f"-gcode expanded-\n{gcode}--")

            # ToOpenSBP will modify our .values, and not restore them
            was_values = copy(self.values)
            try:
                # Treat each `sublist` as a unit/file, so new instance
                native = ToOpenSBP(self).translate(gcode)
            finally:
                self.values = was_values

            g_code_sections.append((partname, native))

        return g_code_sections

    def export(self) -> GCodeSections:
        """Process the parser arguments, then postprocess the 'postables'."""
        args: ParserArgs
        flag: bool

        Path.Log.debug("Exporting the job")
        self.reinitialize()

        (flag, args) = self.process_arguments()
        #
        # If the flag is True, then continue postprocessing the 'postables'
        #
        if flag:
            self.arguments = args
            Path.Log.debug(f"cli args {self._job.PostProcessorArgs}")
            Path.Log.debug(f"argparsed {args}")
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

    @property
    def tooltip(self):
        return self.__doc__

    @property
    def tooltipArgs(self) -> FormatHelp:
        return self.parser.format_help()

    @property
    def units(self) -> Units:
        return self._units


def gcode(*commands):
    """Only for use in class ToOpenSBP, for methods.
    Decorator to add the function to the translate-map.
        `commands` is list of Gcodes, i.e. "G54", "G55",
        specify two digit gcode: e.g. "G01", "M06"
    """

    def gcode(func):
        # we really just want to make a map with it
        # But, we want the map to belong to the class
        # which doesn't exist at "use" time
        # so, annotate the method
        # and collect/insert after the class
        func._gcode = []
        for c in commands:
            func._gcode.append(c)
            canonical = re.sub(r"^([A-Z])(\d(\.|$))", r"\g<1>0\2", c)
            if canonical != c:
                raise Exception(f"Internal: Must be a 2 digit gcode (e.g. G00): @gcode('{c}')")

        # null wrapper
        # FIXME: I think if we wrap it, then do obj.func(args), we'll get proper inhertance dispatch
        return func

    return gcode


def gcode_insertmap():
    # see gcode() above. this inserts the map
    for attr in ToOpenSBP.__dict__.values():
        if callable(attr) and hasattr(attr, "_gcode"):
            for g in attr._gcode:
                ToOpenSBP.DispatchMap[g] = attr


class ToOpenSBP:
    """Translate gcode to opensbp"""

    PositionAxis = "XYZAB"  # though AB is barely supported here

    DispatchMap = {}

    def __init__(self, postprocessor: Opensbp):
        # we will use specific features (.values[x] and arguments from the opensbp post)
        self.post = postprocessor

        # xyzf etc state
        self.current_location = {p: None for p in self.post.values["PARAMETER_ORDER"]}
        # for our speed-modal
        self.current_location["ms"] = ["", ""]
        self.current_location["js"] = ["", ""]
        self.end_location = [None for x in self.PositionAxis]

        self.set_units = None  # flag and memory of the first time we see a G20/G21 set-units
        self._postfix = []  # balancing things to add to end
        self.last_gui_update = 0
        self.first_tool = True
        self.first_no_F = True

    def translate(self, gcode):
        """Entry point, returns the translated contents, e.g. opensbp lines.
        We rely on self.post to hold some state vars: we reuse self.post.values.
        We have lost most context, and structuring of the gcode/job at this point,
        and the export_common() has potentially inserted and translated stuff.
        So, we repeat some logic from UtilsParse, like "is this a tool change?", etc.
        Line-numbers are also lost, and won't match the "inspect toolpath commands" g-code.
        We do have self.post._job
        """
        native = ""
        before_len = len(gcode.split(nl))

        # We reuse the postprocessor.values
        # So, we have to reset some values
        self.reset_values()
        Path.Log.debug(f"post.values {self.post.values}")

        for gcode_line in gcode.split(nl):

            # be nice
            if time.monotonic() - self.last_gui_update >= PreventGuiTimeout:
                if "Gui" in dir(FreeCADGui):
                    FreeCADGui.updateGui()
                self.last_gui_update = time.monotonic()

            self.post.values["line_number"] += 1  # actual line number

            # we need Path.Commands to work with
            path_command = self.to_path_command(
                gcode_line, f"expanded gcode line {self.post.values['line_number']}"
            )
            if path_command is None:
                continue

            # canonicalize to 2 digits
            if path_command.Name.startswith("("):
                pass
            else:
                # canonicalize to 2 digits
                # All the @gcode() methods can now assume 2 digits
                # We have conflated Path.Command.Name and general gcode. this allows us to expand drill, then "post process" that.
                path_command.Name = re.sub(r"^([A-Z])(\d(\.|$))", r"\g<1>0\2", path_command.Name)

            # print(f"### GCODE [{self.post.values['line_number']}] {path_command.toGCode()}")

            # And now we reproduce most of UtilsParse.parse_a_path

            self.track_by_comments(path_command)

            if self.post.arguments.gcode_comments and not path_command.Name.startswith("("):
                native += self.comment(
                    f"[{self.post.values['line_number']}] {path_command.toGCode()}"
                )

            new_location = [
                float(path_command.Parameters.get(a, self.current_location.get(a, 0.0)) or 0.0)
                for a in self.PositionAxis
            ]
            skip_modal = False

            if (
                path_command.Name in {"G00", "G01"}
                and self.post.values["MODAL"]
                and new_location == self.end_location
            ):
                skip_modal = True

            # one place to figure out our end, used by set_speed()
            if path_command.Name in self.post.values["MOTION_COMMANDS"]:
                if self.post.values["MOTION_MODE"] == "G90":
                    self.end_location = new_location
                else:
                    raise Exception("Relative G91 not supported yet")
                    # FIXME: not tested (relative mode not fully implemented):
                    # self.end_location = [
                    #    float(self.current_location[a] or 0.0) + float(path_command.Parameters.get(a, 0.0))
                    #    for a in self.current_location if a in self.PositionAxis
                    # ]
                # print(f"### end at {self.end_location}")

            if skip_modal:
                rez = ""
            else:
                # handle that gcode
                # print(f"### path_command is {path_command.__class__.__name__} {path_command}")
                rez = self.dispatch(path_command)
                # print(f"### translated: {rez.rstrip()}")

            # append to buffer
            native += rez

            if path_command.Name in self.post.values["MOTION_COMMANDS"]:
                # does the right thing for position axis, for relative
                for i, a in enumerate(self.PositionAxis):
                    self.current_location[a] = self.end_location[i]
                # all other parameters (especially F)
                for a, v in path_command.Parameters.items():
                    if a not in self.PositionAxis:
                        self.current_location[a] = v
                # print(f"### current {self.current_location}")

            self.post.values["last_command"] = path_command

        native += self.postfix()

        Path.Log.debug(f"GCode in {before_len} -> out {len(native.split(nl))}")
        return native

    def postfix(self):
        rez = ""

        if self._postfix:
            rez += nl.join(reversed(self._postfix))
            rez += nl

        return rez

    def reset_values(self):
        self.post.values.update(
            {
                "line_number": 0,
                "COMMENT_SYMBOL": "'",
            }
        )

    def track_by_comments(self, path_command):
        """Since we've lost the Path objects structure (we only have the gcode str),
        we'll track things of interest by comments here.
        """
        if path_command.Name.startswith("("):
            if m := (
                re.match(r"\(Path: ([^)]+)\)", path_command.Name)
                or re.match(r"\(Begin operation: ([^)]+)\)", path_command.Name)
                or re.match(r"\(Begin (preamble)\)", path_command.Name)
            ):
                self.post.values["Operation"] = m.group(1)

    def to_path_command(self, gcode: str, during: str):
        """Utility,
        One gcode line to a Path.Command, with some error handling
        e.g. to_path_command("G0 X50", "preamble")
        `during` is explanatory text if we get a conversion error
        """

        if gcode == "":
            return None

        pc = Path.Command()
        try:
            pc.setFromGCode(gcode)
        except ValueError as e:
            # can't tell if it is really 'Badly formatted GCode argument', so just add our gcode to the message
            message = f"During {during}, {self.location(gcode)}"
            FreeCAD.Console.PrintError(message)
            raise ValueError(message) from e
        return pc

    def dispatch(self, path_command):
        """The whole point of this class is to put the translate stuff in a modular-unit,
        and translate g-codes.
        So, each gcode maps to a function that handles it,
        so, we use @gcode decorators to collect the gcodes and functions,
        and we dispatch here.
        We return a string, which might be multi-line,
        or '' to mean nothing resulted
        """
        command = None
        if path_command.Name.startswith("("):
            # sadly, Path.Command doesn't set .Name to "comment", but rather to the comment string
            command = "comment"
        else:
            command = path_command.Name

        # Call the translate handler
        if command in self.DispatchMap:
            rez = self.DispatchMap[command](
                self, path_command
            )  # careful, doesn't do inheritance lookup
            return rez

        else:
            message = f"gcode not handled at {self.location(path_command)}"
            if (
                self.post.arguments.abort_on_unknown
                # FIXME: cf SUPPRESS_UNKNOWN
                and command not in self.post.values["ALLOW_UNKNOWN"]
            ):
                FreeCAD.Console.PrintError(message + "\n")
                raise NotImplementedError(message)
            else:
                FreeCAD.Console.PrintWarning("Skipped:  " + message + "\n")
                return self.comment(message)

    def location(self, path_command=None):
        """a message fragment of where we are, and the path_command if you want
        `path_command` can be a literal-string gcode, or usually a Path.Command
        """
        g = (
            f": {self.post.values['line_number']} {path_command if isinstance(path_command, str) else path_command.toGCode()}"
            if path_command
            else ""
        )
        return f"{self.post.values['Operation']}{g}"

    def comment(self, message, force=False):
        """if OUTPUT_COMMENTS, then generate the comment,
        `force` ignores OUTPUT_COMMENTS and always generates.
        INCLUDES the \n (nl)!
        """
        # note that comments from FreeCAD and export_common are strings with "()"
        # so, you can't tell what comments came from a Path operation object, or from Path/Post utils
        # and the parenthesis are preservered, so the final looks like '(blah blah)
        # Comments from this postprocessor are w/o (), so the final looks like 'our comment
        if self.post.values["OUTPUT_COMMENTS"] or force:
            return self.post.values["COMMENT_SYMBOL"] + message + nl
        else:
            return ""

    @gcode("comment")
    def t_comment(self, path_command):
        # leaves ()
        rez = ""

        rez += self.comment(path_command.Name)

        # fixups
        # We don't have access to the Path object, and we need/want to know where we are
        # e.g. probing. This should be fixed in the new "machine" style
        if path_command.Name.startswith("(Post Processor: "):
            rez += self.comment("  " + self.post._job.PostProcessorArgs)
        elif path_command.Name.startswith("(Cam File: "):
            rez += self.comment(f"Job: {self.post._job.Label}")
        elif m := re.match(r"\(\s*MC_RUN_COMMAND\s+(.+)\)$", path_command.Name):
            # let's leave the original as a comment (if comments are on)
            rez += m.group(1) + "\n"
        elif m := re.match(r"\(PROBEOPEN (.+)\)$", path_command.Name):
            filename = m.group(1)
            if "." not in filename:
                # default .txt (really "space delimited values")
                filename += ".txt"

            # the Probe operation

            # can't get &UserDataFolder to catenate properly anywhere...
            # so, just filename
            rez += self.comment(
                "Load the My_Variables file from Custom Cut 90 in C:\\SbParts\\Custom"
            )
            rez += "C#,90" + nl
            # if re.match(r'[^:]+:', filename):
            #    # "absolute"
            #    rez += f'OPEN "{filename}" FOR OUTPUT as #1' + nl
            # else:
            #    # "relative"
            #    rez += "GetUsrPath, &UserDataFolder" + nl
            #    rez += f'OPEN &UserDataFolder & "/{filename}" FOR OUTPUT as #1' + nl
            rez += f'OPEN "{filename}" FOR OUTPUT as #1' + nl

            rez += "&hit = 0" + nl
            # subroutines, cleanup
            # but only once per post
            if self.post.values["first_probe"]:
                self.post.values["first_probe"] = False
                self._postfix.append(
                    """GOTO SkipProbeSubRoutines
CaptureZPos:
  ' for g38.2 probe, write the data on probe-contact
  ' and set flag for didn't-fail
  ' xyzab
  WRITE #1; %(1); " "; %(2); " "; %(3); " "; %(4); " "; %(5)
  &hit = 1
  RETURN
FailedToTouch:
  ' for g38.2 probe, when
  ' failed to trigger w/in movement
  MSGBOX(Failed to touch...Exiting,16,Probe Failed)
  END
SkipProbeSubRoutines:"""
                )
        elif path_command.Name == "(PROBECLOSE)":
            rez += self.comment("Clear probe-switch-trigger")
            rez += "ON INPUT(&my_ZzeroInput, 1)" + nl
            rez += "CLOSE #1" + nl

        return rez

    @gcode("G20", "G21")  # inches, metric
    def t_units(self, path_command):
        if self.set_units:
            raise ValueError(
                "You can only set the units once, already {self.set_units['command']} at {self.set_units['at']}. You tried again at {self.location(path_command)}"
            )
        else:
            # remember where
            self.set_units = {"command": path_command.Name, "at": self.location()}

            undesired_units = {"G20": "1", "G21": "0"}[path_command.Name]  # OPPOSITE!
            rez = [
                f"IF %(25) = {undesired_units} THEN GOTO WrongUnits",
            ]

            pp_which = {"G20": "G20/--inches", "G21": "G21/--metric"}[path_command.Name]
            self._postfix.append(
                nl.join(
                    [
                        "GOTO AfterWrongUnits",
                        "WrongUnits:",
                        '  if %(25) = 0 THEN &shopbot_which="inches"',
                        '  if %(25) = 1 THEN &shopbot_which="mm"',
                        # NB: no commas in strings!
                        f'    MSGBOX("Post-processor wants {pp_which} but ShopBot is " & &shopbot_which & ". Change Units in ShopBot and try again.",0,"Change Units")',
                        "    ENDALL",
                        "AfterWrongUnits:",
                    ]
                )
            )

            return nl.join(rez) + nl

    @gcode("G90")  # no relative (G91) yet, have to fix modal handling for relative
    def t_absolute_mode(self, path_command):
        self.post.values["MOTION_MODE"] = path_command.Name
        return {"G90": "SA", "G91": "SR"}[path_command.Name] + nl

    @gcode("M06")
    def t_toolchange(self, path_command):
        tool_number = int(path_command.Parameters["T"])

        # check for tool actually existing
        tool_controller = next(
            (x for x in self.post._job.Tools.Group if x.ToolNumber == tool_number), None
        )
        if not tool_controller:
            # HACK: at least till 1.1, nothing enforces tool-numbers in the job to be unique
            #   and "Tn" doesn't have to match a ToolNumber
            #   we'll do a compatibility hack ONLY if all tools == 1
            if (
                all(x.ToolNumber == 1 for x in self.post._job.Tools.Group)
                and len(self.post._job.Tools.Group) >= tool_number
            ):
                tool_controller = self.post._job.Tools.Group[tool_number - 1]
                FreeCAD.Console.PrintWarning(
                    f"Job <{self.post._job.Label}> doesn't have unique tool-numbers? at {self.location(path_command)}"
                )
            else:
                raise ValueError(
                    f"Toolchange with non-existent tool_number {tool_number} at {self.location(path_command)}. Do tools have unique tool-numbers?"
                )

        tool_name = f"{tool_controller.Label}, {tool_controller.Tool.Label}"  # not sure if we want both .Label's, just trying to help the operator
        safe_tool_name = re.sub(r"[^A-Za-z0-9/_ .-]", "", tool_name)

        rez = []

        if not self.post.values["OUTPUT_TOOL_CHANGE"]:
            rez.append(
                self.comment(
                    f"First change tool, should already be #{tool_number}: {safe_tool_name}",
                    force=True,
                ).rstrip()
            )

        rez += [
            f"&Tool={tool_number}",
            f'&ToolName="{safe_tool_name}"',
        ]

        if self.post.values["OUTPUT_TOOL_CHANGE"]:
            # automatic no prompt, or manual prompt (depends on correct shopbot setup)
            rez.append("C9")

        else:
            if self.first_tool:
                self.first_tool = False
            else:
                raise NotImplementedError(
                    f"2nd tool can't be done, #{tool_number}, no way to change-tool when --no-tool-changer at {self.location(path_command)}. Try 'Order by Tool' or 'Order by Operation' in job's 'Output' tab."
                )

        rez.append(self.set_initial_speeds(tool_controller, path_command).rstrip())
        rez = nl.join((x for x in rez if x != "")) + nl

        return rez

    @gcode("G00", "G01")
    def t_move(self, path_command):
        """Oh boy.
        opensbp specifies the xy speed, and Z speed separately for a motion.
        e.g. a "VS,sxy,sz" then the move like "M3,x,y,z".
        But, gcode has a F which the speed of the vector
        (for rapid, it's whatever-the-machine-setting-is).
        FreeCAD has horizontal speed (xy), and vertical speed (z),
        which it uses to calculate F,
        (we already set rapid at set_initial_speed() time for each tool-change).
        Finally, we have to take the delta(x,y,z) vector and project the F speed onto xy, and z
        to generate the MS command before each move command.
        The Mx or Jx just uses the axis distances.
        FIXME? If the first motion is not G0, and axis are 'Z' + X|Y,
            (e.g. G1 X10 Y20 Z30)
            then we don't know how to calculate the MS speed,
            because we need to know the last-position to split F across Z & X|Y,
            and there isn't one (that we know about), though we init'd to 0,0,0.
            We assume a G1 happens before any other motion, to establish a position.
            We could abort on this...
        """
        rez = ""

        # Optimize the command, specifying 1..5 axis values
        axis = [path_command.Parameters.get(a, None) for a in self.PositionAxis]
        last_not_none = 0
        # XYZABC, but reversed
        for i in reversed(range(0, len(axis))):
            if axis[i] is not None:
                last_not_none = i
                break
        axis = axis[: last_not_none + 1]

        if feed_rate := path_command.Parameters.get("F", None):
            if path_command.Name == "G00":
                if self.post.arguments.abort_on_unknown:
                    raise ValueError(
                        f"Rapid moves (G0) can't have an F at {self.location(path_command)}"
                    )

        _, speed_command = self.set_speed(path_command)
        rez += speed_command

        native_command = "J" if path_command.Name == "G00" else "M"
        # print(f"  ### is a '{native_command}'")

        # nb, we don't have to do anything for --axis-modal, handled by common stuff earlier!

        axis_ct = len(axis)
        if axis_ct == 1:
            native_command += "X"
        else:
            native_command += str(axis_ct)

        formatted_axis = (
            (format(a, f".{self.post.values['FEED_PRECISION']}f") if a is not None else "")
            for a in axis
        )
        rez += f"{native_command},{','.join(formatted_axis)}" + nl

        return rez

    @gcode("M03")  # clockwise only. do the spindle-controlers do CCW?
    def t_spindle_speed(self, path_command):
        native = ""

        if "S" in path_command.Parameters:
            native += f"TR,{int(path_command.Parameters['S'])}\n"  # rpm units

        # macro will do the dialog-box if you don't have a controlled spindle
        native += "C6\n"

        if self.post.values["SPINDLE_WAIT"] > 0:
            native += f"PAUSE {int(self.post.values['SPINDLE_WAIT'])}\n"

        return native

    def format_value(self, value, precision_type="FEED_PRECISION"):
        """format for the precision (e.g. AXIS_PRECISION)
        notably dealing with slightly-less-than-zero == "0" not "-0"
        """
        # format rounds, so duplicate that effect
        if abs(value) < 0.5 * 10 ** (-self.post.values[precision_type]):
            value = 0.0
        return format(value, f".{self.post.values[precision_type]}f")

    @gcode("G02", "G03")
    def t_arc(self, path_command):
        # only center-format: IJ
        # only absolute mode
        # only xy plane
        # only P1 (or no P)
        # cases:
        #   current-position is start
        #   Z causes helical-arc
        #   Pn causes arc-as-defined + (n-1) whole circles: not handled
        #   XY is the end-position for a segment
        #   none of XY means whole circle
        #   IJ is the location of the arc-center: an offset. at least one of is required
        #   F is required

        # we would have to generate multiple CG's for repetitions (P)
        handled_parameters = "XYZIJFPK"  # notably, not R

        not_handled = []
        not_handled = [a for a in path_command.Parameters if a not in handled_parameters]
        # we handle K=0.0 by ignoring it (xy-plane), other K's we don't handle
        if "K" in path_command.Parameters and path_command.Parameters["K"] != 0.0:
            not_handled.append("K")
        if "P" in path_command.Parameters and path_command.Parameters["P"] != 1:
            not_handled.append("P")
        if not_handled:
            message = (
                f"We can't do parameters {not_handled} for an arc in {self.location(path_command)}"
            )
            FreeCAD.Console.PrintError(message)
            if self.post.arguments.abort_on_unknown:
                raise ValueError(message)
            else:
                return ""

        if self.post.values["MOTION_MODE"] != "G90":
            opname = self.post.values["Operation"].Label if self.post.values["Operation"] else ""
            message = f"We can't do relative mode for arcs in [{self.post.values['line_number']}] {opname} {path_command.toGCode()}"
            FreeCAD.Console.PrintError(message)
            if self.post.arguments.abort_on_unknown:
                raise NotImplementedError(message)
            else:
                return ""

        if path_command.Name == "G02":  # CW
            dirstring = "1"
        else:  # G3 means CCW
            dirstring = "-1"
        txt = ""

        dz, speed_command = self.set_speed(path_command)
        txt += speed_command

        txt += "CG,"
        txt += ","  # no diameter

        # end
        # Omitting XY has special meaning to ShopBot, it is not the same as modal-axis
        # The PostProcess code will drop the XYZ axis on --axis-modal, but we need it:
        x = path_command.Parameters.get("X", self.current_location["X"])
        y = path_command.Parameters.get("Y", self.current_location["Y"])

        txt += self.format_value(x) + ","
        txt += self.format_value(y) + ","

        # Center is at offset:
        txt += (
            self.format_value(
                path_command.Parameters["I"] if "I" in path_command.Parameters.keys() else 0.0
            )
            + ","
        )
        txt += (
            self.format_value(
                path_command.Parameters["J"] if "J" in path_command.Parameters.keys() else 0.0
            )
            + ","
        )
        txt += "T" + ","  # move on diameter
        txt += dirstring + ","

        if "Z" not in path_command.Parameters:
            dz = 0

        # Z causes a helical, "causes the defined plunge to be made gradually as the cutter is circling down"
        # Note, dz is actual distance vector, but ShopBot uses -dz to mean "plunge" relative
        txt += self.format_value(-dz) + ","

        txt += ","  # repetitions
        txt += ","  # proportion-x
        txt += ","  # proportion-y

        if dz != 0.0:
            # helical cases
            # we don't do "bottom pass" (4) because FreeCAD seems to do that and it's not a g-code thing anyway
            if "X" not in path_command.Parameters and "Y" not in path_command.Parameters:
                # circle
                feature = 3  # spiral
            else:
                feature = 3  # spiral
        else:
            feature = 0

        txt += f"{feature},"
        txt += "1,"  # continue the CG plunging (don't pull up)
        txt += "0"  # no move before plunge

        # actual Z, opensbp plunge is a delta, note the actual Z as a comment
        z = path_command.Parameters.get("Z", self.current_location["Z"])
        txt += " ' Z" + self.format_value(z)
        txt += "\n"
        return txt

    @gcode("M00", "M01")
    def t_prompt(self, command):
        # Prompt with "Continue?" and pause, wait for user-interaction
        # If a comment precedes M00, that is used as the prompt (to emulate opensbp behavior)

        txt = ""
        if not self.post.values["last_command"].Name.startswith("("):
            # default prompt
            where = []
            if self.post._job:
                where.append(f"<{self.post._job.Label}>")
            if self.post.values["Operation"]:
                where.append(f"<{self.post.values['Operation']}>")
            txt += self.comment(f"Continue {'.'.join(where)}?", force=True)
        elif not self.post.values["OUTPUT_COMMENTS"]:
            # Force inclusion of that preceding comment as a prompt
            txt += self.comment(self.post.values["last_command"].Name, force=True)
        txt += "PAUSE\n"
        return txt

    @gcode("M05")
    def t_stop_spindle(self, command):
        return "C7\n"

    @gcode("M02", "M30")
    def t_stop(self, command):
        return "END\n"

    @gcode("M08")
    def t_coolant_on(self, command):
        return "SO,3,1\n"

    @gcode("M09")
    def t_cooland_off(self, command):
        return "SO,3,0\n"

    @gcode("G38.2")
    def t_probe(self, command):

        speed = command.Parameters.get("F", None)
        if speed is not None:
            speed = float(speed)
        if speed == 0.0:
            FreeCAD.Console.PrintWarning(
                f"G38.2 with an F0.0, set Tool speeds? at {self.location(command)}\n"
            )
            return ""
        if speed is None:
            speed = self.current_location["ms"][1]  # out of [ xy, z ]

        axis = " ".join(
            [f"{a}{command.Parameters[a]}" for a in self.PositionAxis if a in command.Parameters]
        )

        # PROBEOPEN sets up the contact-detect, so G38.2 are just moves
        rez = ""

        rez += "&hit = 0" + nl  # for did-we-hit OR fail

        # for probing, we have to setup the on-input for every move
        rez += "ON INPUT(&my_ZzeroInput, 1) GOSUB CaptureZPos" + nl

        g = f"G01 F{speed} {axis}"
        rez += self.t_move(Path.Command(g))

        # and check for fail to contact
        rez += "IF &hit = 0 THEN GOTO FailedToTouch\n"

        return rez

    def set_speed(self, path_command):
        # For non-rapid, F applies to the vector of all the axis
        # For rapid, full speed on the axis from the toolchange settings
        #   (so no output here)
        # Projects F into XY plane, and Z plane for shopbot
        # Always uses "VS" command to not "punctuate" the stack (ms/js will cause ramp up/down in speed)
        # Elides VS values if not change or 0.0 (shopbot doesn't like 0)
        # Elides trailing ,
        # Elides whole VS if no values

        if path_command.Name == "G00":
            # Actually, we just use the full speed on xy and z axis
            # which was initialized at toolchange time
            # the args to vs would be: VS,xy,z,a,b,xy_job,z_job,a_job,b_jog
            return (0, "")

        # we output VS, but this lets us track MS vs JS
        which_speed = "MS"

        last_position = [float(self.current_location[a] or 0) for a in self.PositionAxis]

        def fmt_diff(l):
            return [(f"{p:9.3f}" if p is not None else f"{str(p):9s}") for p in l]

        # Linear move
        if path_command.Name == "G01":
            d_axis = list(map(operator.sub, self.end_location, last_position))

            squared_d_axis = [v**2 for v in d_axis]
            distance = math.sqrt(sum(squared_d_axis))
            z_distance = abs(d_axis[2])
            xy_distance = math.sqrt(sum(squared_d_axis[:2]))
            axis = [a for a in self.PositionAxis if a in path_command.Parameters]

        # Arcs
        elif path_command.Name in {"G02", "G03"}:

            def arc_length_3d(center, start, end, clockwise):
                """center, start, end: (x, y, z) tuples
                clockwise: True for G2, False for G3
                Returns the true 3D arc length.
                """

                cx, cy, cz = center
                sx, sy, sz = start
                ex, ey, ez = end

                # ---- linear Z interpolation ----
                dz = ez - sz

                # ---- XY arc angle ----
                r = math.hypot(sx - cx, sy - cy)

                a0 = math.atan2(sy - cy, sx - cx)
                a1 = math.atan2(ey - cy, ex - cx)

                dtheta = a1 - a0

                if dtheta == 0:
                    dtheta = 2 * math.pi
                elif clockwise:
                    if dtheta > 0:
                        dtheta -= 2 * math.pi
                else:
                    if dtheta < 0:
                        dtheta += 2 * math.pi

                arc_xy = abs(r * dtheta)

                # ---- true helical arc length ----
                return (arc_xy, math.hypot(arc_xy, dz))

            start_position = [float(self.current_location[a] or 0) for a in "XYZ"]
            center_offset = [
                float(path_command.Parameters.get(a, None) or 0) for a in "IJK"
            ]  # k always 0
            end_position = [
                float(path_command.Parameters.get(k, start_position[i]))
                for i, k in enumerate("XYZ")
            ]
            z_distance = start_position[2] - end_position[2]

            # If the XY is omitted, it means a whole circle, and arc-length-3d will give that
            xy_distance, distance = arc_length_3d(
                map(operator.add, start_position, center_offset),  # center
                start_position,
                end_position,
                path_command.Name == "G02",  # clockwise?
            )

            axis = list("XYZ")

        distances_for_speed = [abs(xy_distance), abs(z_distance)]  # abs(xy) shouldn't be necessary

        if path_command.Name == "G00":
            # see above, we don't get to here on G0
            # Rapid speed is the full speed on xy, and z
            # no projecting on to the vector
            # FIXME: AB not handled yet
            speeds = self.current_location["js"]

        # feed motions
        else:
            # FIXME: AB speeds not handled yet

            f = path_command.Parameters.get("F", None)
            if f is None:
                f = self.current_location["F"]
                # print(f"  ### no f, last= '{f}'")

            if f is None:
                # No F and no previous, which is not good. default to machine's feed speeds.
                f = None
                if self.first_no_F:
                    FreeCAD.Console.PrintWarning(
                        f"No F, and no previous F speed at {self.location(path_command)}. Using tool's feed speeds.\n"
                    )
                    self.first_no_F = False
                speeds = ["", ""]

            else:
                # have a F/previous: use it
                f = float(f)

                # if only in xy, or only in z, then speed=F
                if xy_distance != 0.0 and z_distance == 0.0:
                    speeds = [f, 0.0]
                elif xy_distance == 0.0 and z_distance != 0.0:
                    speeds = [0.0, f]

                else:
                    # FIXME: AB not handled yet
                    speeds = [
                        ((f * d / distance) if distance != 0 else 0) for d in distances_for_speed
                    ]
                    # print(f"  ### speed w/xyz {speeds}")
            # print(f"  ### speeds  {speeds}")

        min_speed = self.post.values["MIN_SPEED"]

        def gtmin(s):
            if s == "":
                return s
            elif abs(s) >= min_speed:
                return s
            elif s == 0.0:
                return ""
            elif abs(s) < min_speed:
                return min_speed * (-1 if s < 0 else 1)
            # that's all the cases

        speeds = [gtmin(s) for s in speeds]
        speeds = [
            (format(s, f'.{self.post.values["SPEED_PRECISION"]}f') if s != "" else "")
            for s in speeds
        ]

        if which_speed == "MS":
            # axis-modal against actual output (formatted to SPEED_PRECISION)
            # and save as current

            non_elided = copy(speeds)

            if True:  # speed-modal always true
                # compare to previous 'ms' speeds, so we can skip 'MS' if nothing changes
                for i, new_speed in enumerate(speeds):
                    old_speed = self.current_location[which_speed.lower()][i]
                    if old_speed == new_speed:
                        speeds[i] = ""

            # save it for next time, for --speed-modal
            self.current_location["ms"] = non_elided

        # cleans up trailing , when trailing speeds elided
        if which_speed == "MS":
            cmd = f"VS,{','.join(speeds)}".rstrip(",")
        elif which_speed == "JS":
            # again, shouldn't get here in this version of the code
            cmd = f"VS,,,,,{','.join(speeds)}".rstrip(",")

        # If there is no speed to set (e.g. the move ends up as delta-0), no MS needed
        if cmd == "VS":
            cmd = ""
        else:
            cmd += nl
        return (z_distance, cmd)

    def set_initial_speeds(self, tool_controller, path_command):
        # need to ensure initial values for speeds
        # rapid-speed is never emitted by gcode, but we need to set it!
        # and we just set the initial speed for "feed" too

        Path.Log.debug(f"Setspeeds {tool_controller.Label}")
        native = ""

        native += self.comment(f"set speeds: {tool_controller.Label}")
        speeds = {
            "ms": [],  # xy,z
            "js": [],  # xy,z
        }

        def append_speed(which_speed, which_speed_key):
            with_units = getattr(tool_controller, which_speed)
            speed = float(with_units.getValueAs(self.post.values["UNIT_SPEED_FORMAT"]))

            if abs(speed) >= 0.5 * 10 ** (-self.post.values["SPEED_PRECISION"]):  # i.e. not zero

                formatted = format(speed, f'.{self.post.values["SPEED_PRECISION"]}f')

                speeds[which_speed_key].append(formatted)
                return None
            else:
                FreeCAD.Console.PrintWarning(
                    f"ToolController <{self.post._job.Label}>.<{tool_controller.Label}> did not set {which_speed} speed, set the HorizFeed and VertFeed. ( for {self.location(path_command)} )\n"
                )
                speeds[which_speed_key].append("")
                if which_speed.endswith("Rapid"):
                    warn_rapid.append(which_speed)
                return self.comment(f"no {which_speed}", force=True)

        warn_rapid = []  # empty is no warning

        # tool's speeds -> speeds[ 'ms' & 'js' ]
        for which_speed_key, which_speed_properties in {
            "ms": ["HorizFeed", "VertFeed"],
            "js": ["HorizRapid", "VertRapid"],
        }.items():
            for which_speed in which_speed_properties:  # by property for warning messages
                comment = append_speed(which_speed, which_speed_key)
                if comment is not None:
                    native += comment

            # for --speed-modal
            self.current_location[which_speed_key] = speeds[which_speed_key]

            # add to command-stream
            command_prefix = which_speed_key.upper()
            command = (f"{command_prefix}," + ",".join(speeds[which_speed_key])).rstrip(",")
            if command != command_prefix:  # has actual speeds
                native += command + "\n"

        # fixme: where to get A&B values?
        # speeds['ms'].append('') # a-move-speed
        # speeds['ms'].append('') # b-move-speed

        if warn_rapid:
            if not self.post.arguments.native_rapid_fallback:
                raise ValueError(
                    f"ToolController <{self.post._job.Label}>.<{tool_controller.Label}> did not set xy&z rapid speeds, and you specified --no-native-rapid-fallback. Set the rapid speeds. {self.location(path_command)}"
                )
            else:
                FreeCAD.Console.PrintWarning(
                    f'Using machine\'s rapid ("jog") for {" and ".join(warn_rapid)}, for ToolController <{self.post._job.Label}>.<{tool_controller.Label}>\n'
                )

        Path.Log.debug(f"setspeeds {speeds}")

        return native


gcode_insertmap()  # fixup DispatchMap
