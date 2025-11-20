#!/usr/bin/env python3
# SPDX-License-Identifier: EUPL-1.2
# A FreeCAD postprocessor targeting Snapmaker machines with CNC capabilities
# ********************************************************************************
# *  Copyright (c) 2025 Clair-Loup Sergent <clsergent@free.fr>                   *
# *                                                                              *
# *  Licensed under the EUPL-1.2 with the specific provision                     *
# *  (EUPL articles 14 & 15) that the applicable law is the French law           *
# *  and the Jurisdiction Paris.                                                 *
# *  Any redistribution must include the specific provision above.               *
# *                                                                              *
# *  You may obtain a copy of the Licence at:                                    *
# *  https://interoperable-europe.ec.europa.eu/collection/eupl/eupl-text-eupl-12 *
# *                                                                              *
# *  Unless required by applicable law or agreed to in writing, software         *
# *  distributed under the Licence is distributed on an "AS IS" basis,           *
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or             *
# *  implied. See the Licence for the specific language governing                *
# *  permissions and limitations under the Licence.                              *
# ********************************************************************************


import argparse
import base64
import datetime
import os
import pathlib
import re
import tempfile
from copy import deepcopy

import FreeCAD
import Path
import Path.Base.Util
import Path.Post.Processor
import Path.Post.UtilsArguments
import Path.Post.UtilsExport
import Path.Post.Utils
import Path.Post.UtilsParse
import Path.Main.Job

translate = FreeCAD.Qt.translate

if DEBUG := False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


SNAPMAKER_MACHINES = {
    "Original": dict(
        name="Snapmaker Original",
        # https://forum.snapmaker.com/t/cnc-work-area-size/5178
        boundaries=dict(X=125, Y=125, Z=50),
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    "A150": dict(
        name="Snapmaker 2 A150",
        boundaries=dict(X=145, Y=160, Z=90),
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    "A250": dict(
        # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
        name="Snapmaker 2 A250",
        boundaries=dict(X=230, Y=250, Z=180),
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    "A250T": dict(
        name="Snapmaker 2 A250T",
        boundaries=dict(X=230, Y=250, Z=180),
        lead=dict(X=20, Y=20, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    "A350": dict(
        # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
        name="Snapmaker 2 A350",
        boundaries=dict(X=320, Y=350, Z=275),
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    "A350T": dict(
        # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
        name="Snapmaker 2 A350T",
        boundaries=dict(X=320, Y=350, Z=275),
        lead=dict(X=20, Y=20, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    "Artisan": dict(
        name="Snapmaker Artisan",
        boundaries=dict(X=400, Y=413, Z=400),  # Y is modified to accomodate 200W_CNC toolhead
        lead=dict(X=40, Y=40, Z=8),  # Linear module screw pitch (mm/turn)
    ),
}

# modifications for various machines
SNAPMAKER_MODKITS = {
    "quick_swap": dict(
        name="Quick Swap Kit",
        arg=("quick_swap", "QS",),
        help="quick swap kit (Snapmaker 2 only)",
        boundaries_delta=dict(X=0, Y=-15, Z=0),
        machines={"A250", "A250T", "A350", "A350T"},
    ),
    "bracing_kit": dict(
        name="Bracing Kit",
        arg=("bracing_kit", "BK",),
        help="Bracing kit (Snapmaker 2 only)",
        boundaries_delta=dict(X=0, Y=-12, Z=0),
        machines={"A150", "A250", "A250T", "A350", "A350T"},
    ),
    "z_extension": dict(
        name="Extended Z linear rail",
        arg=("z_extension", "z_ext"),
        help="Extended 2 linear rail (Snapmaker Original only)",
        boundaries_delta=dict(X=0, Y=0, Z=96),
        machines={"Original"},
    ),
}

# Could support other types of toolheads (laser, drag knife, 3DP, ...) in the future
# https://wiki.snapmaker.com/en/Snapmaker_Luban/manual/2_supported_gcode_references#m3m4-modified-cnclaser-on
SNAPMAKER_TOOLHEADS = {
    "Original_CNC": dict(
        name="Original CNC module",
        arg=("Original_CNC", "SM1_CNC"),
        boundaries_delta=dict(X=0, Y=0, Z=0),
        spindle_speeds=dict(min=0, max=7000),
        spindle_percent=True,
        spindle_rpm=False,
        machines={"Original"},
    ),
    "50W_CNC": dict(
        name="50W CNC module",
        arg=("50W_CNC",),
        boundaries_delta=dict(X=0, Y=0, Z=0),
        spindle_speeds=dict(min=0, max=12000),
        spindle_percent=True,
        spindle_rpm=False,
        machines={"A150", "A250", "A250T", "A350", "A350T"},
    ),
    "200W_CNC": dict(
        name="200W CNC module",
        arg=("200W_CNC",),
        boundaries_delta=dict(X=0, Y=-13, Z=0),
        spindle_speeds=dict(min=0, max=18000),
        spindle_percent=True,
        spindle_rpm=True,
        machines={"A150", "A250", "A250T", "A350", "A350T", "Artisan"},
    ),
}


class CoordinatesAction(argparse.Action):
    """argparse Action to handle coordinates (x,y,z)"""

    def __call__(self, parser, namespace, values, option_string=None):
        match = re.match(
            r"^\s*(?P<X>-?\d+\.?\d*),?\s*(?P<Y>-?\d+\.?\d*),?\s*(?P<Z>-?\d+\.?\d*)\s*$", values
        )
        if match:
            # setattr(namespace, self.dest, 'G0 X{0} Y{1} Z{2}'.format(*match.groups()))
            params = {key: float(value) for key, value in match.groupdict().items()}
            setattr(namespace, self.dest, params)
        else:
            raise argparse.ArgumentError(None, message="invalid coordinates provided")


class ExtremaAction(argparse.Action):
    """argparse Action to handle integer extrema (min,max)"""

    def __call__(self, parser, namespace, values, option_string=None):
        if match := re.match(r"^ *(\d+),? *(\d+) *$", values):
            # setattr(namespace, self.dest, 'G0 X{0} Y{1} Z{2}'.format(*match.groups()))
            params = {
                key: int(value)
                for key, value in zip(
                    (
                        "min",
                        "max",
                    ),
                    match.groups(),
                )
            }
            setattr(namespace, self.dest, params)
        else:
            raise argparse.ArgumentError(None,
                                         message="invalid values provided, should be int,int")


class Snapmaker(Path.Post.Processor.PostProcessor):
    """FreeCAD postprocessor targeting Snapmaker machines with CNC capabilities"""

    def __init__(self, job) -> None:
        super().__init__(
            job=job,
            tooltip=translate("CAM", "Snapmaker post processor"),
            tooltipargs=[""],
            units="Metric",
        )

        self.initialize()

    def initialize(self):
        """initialize values and arguments"""
        self.values: dict[str, object] = dict()
        self.argument_defaults: dict[str, bool] = dict()
        self.arguments_visible: dict[str, bool] = dict()
        self.parser = argparse.ArgumentParser()

        self.snapmaker_init_values()
        self.snapmaker_init_argument_defaults()
        self.snapmaker_init_arguments_visible()
        self.parser = self.snapmaker_init_parser(
            self.values, self.argument_defaults, self.arguments_visible
        )

        # create another parser with all visible arguments
        all_arguments_visible = dict()
        for key in iter(self.arguments_visible):
            all_arguments_visible[key] = True
        self.visible_parser = self.snapmaker_init_parser(
            self.values, self.argument_defaults, all_arguments_visible
        )

        FreeCAD.Console.PrintLog(f'{self.values["POSTPROCESSOR_FILE_NAME"]}: initialized.\n')

    def snapmaker_init_values(self):
        """Initialize values that are used throughout the postprocessor."""
        Path.Post.UtilsArguments.init_shared_values(self.values)

        # shared values
        self.values["POSTPROCESSOR_FILE_NAME"] = __name__
        self.values["COMMENT_SYMBOL"] = ";"
        self.values["ENABLE_MACHINE_SPECIFIC_COMMANDS"] = True
        self.values["END_OF_LINE_CHARACTERS"] = "\n"
        self.values["FINISH_LABEL"] = "End"
        self.values["LINE_INCREMENT"] = 1
        self.values["MACHINE_NAME"] = None
        self.values["MODAL"] = False
        self.values["OUTPUT_PATH_LABELS"] = True
        self.values["OUTPUT_HEADER"] = (
            True  # remove FreeCAD standard header and use a custom Snapmaker Header
        )
        self.values["OUTPUT_TOOL_CHANGE"] = True
        self.values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "C",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
            "H",
            "D",
            "P",
            "O",
        ]
        self.values["PREAMBLE"] = """G90\nG17"""
        self.values["PRE_OPERATION"] = """"""
        self.values["POST_OPERATION"] = """"""
        self.values["POSTAMBLE"] = """M400\nM5"""
        self.values["SHOW_MACHINE_UNITS"] = False
        self.values["SPINDLE_DECIMALS"] = 0
        self.values["SPINDLE_WAIT"] = 4.0
        self.values["TOOL_CHANGE"] = "M76"  # handle tool change by inserting an HMI pause
        self.values["TRANSLATE_DRILL_CYCLES"] = True  # drill cycle gcode must be translated
        self.values["USE_TLO"] = False  # G43 is not handled.

        # snapmaker values
        self.values["THUMBNAIL"] = True
        self.values["BOUNDARIES"] = None
        self.values["BOUNDARIES_CHECK"] = False
        self.values["MACHINE"] = None
        self.values["TOOLHEAD"] = None
        self.values["MODKITS"] = dict()
        self.values["SPINDLE_SPEEDS"] = None
        self.values["SPINDLE_PERCENT"] = None
        self.values["MACHINES_LIST"] = SNAPMAKER_MACHINES
        self.values["TOOLHEADS_LIST"] = SNAPMAKER_TOOLHEADS
        self.values["MODKITS_LIST"] = SNAPMAKER_MODKITS

    def snapmaker_init_argument_defaults(self) -> None:
        """Initialize which arguments (in a pair) are shown as the default argument."""
        Path.Post.UtilsArguments.init_argument_defaults(self.argument_defaults)

        self.argument_defaults["tlo"] = False
        self.argument_defaults["translate-drill"] = True

        # snapmaker arguments
        self.argument_defaults["thumbnail"] = True
        self.argument_defaults["gui"] = True
        self.argument_defaults["boundaries-check"] = True

    def snapmaker_init_arguments_visible(self) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        Path.Post.UtilsArguments.init_arguments_visible(self.arguments_visible)

        self.arguments_visible["axis-modal"] = False
        self.arguments_visible["header"] = False
        self.arguments_visible["return-to"] = True
        self.arguments_visible["tlo"] = False
        self.arguments_visible["tool_change"] = True
        self.arguments_visible["translate-drill"] = False
        self.arguments_visible["wait-for-spindle"] = True

        # snapmaker arguments (for record, always visible)
        self.arguments_visible["thumbnail"] = True
        self.arguments_visible["gui"] = True
        self.arguments_visible["boundaries"] = True
        self.arguments_visible["boundaries-check"] = True
        self.arguments_visible["machine"] = True
        self.arguments_visible["toolhead"] = True
        self.arguments_visible["line-increment"] = True
        self.arguments_visible["spindle-speeds"] = True

    def snapmaker_init_parser(
            self, values, argument_defaults, arguments_visible
    ) -> argparse.ArgumentParser:
        """Initialize the postprocessor arguments parser"""
        parser = Path.Post.UtilsArguments.init_shared_arguments(
            values, argument_defaults, arguments_visible
        )

        # snapmaker custom arguments
        group = parser.add_argument_group("Snapmaker only arguments")
        # add_flag_type_arguments function is not used as its behavior is inconsistent with argparse
        # handle thumbnail generation
        group.add_argument(
            "--thumbnail",
            action="store_true",
            default=argument_defaults["thumbnail"],
            help="Include a thumbnail (require --gui)",
        )
        group.add_argument(
            "--no-thumbnail",
            action="store_false",
            dest="thumbnail",
            help="Remove thumbnail"
        )

        group.add_argument(
            "--gui",
            action="store_true",
            default=argument_defaults["gui"],
            help="allow the postprocessor to execute GUI methods",
        )
        group.add_argument(
            "--no-gui",
            action="store_false",
            dest="gui",
            help="Execute postprocessor without requiring GUI",
        )

        group.add_argument(
            "--boundaries-check",
            action="store_true",
            default=argument_defaults["boundaries-check"],
            help="check boundaries according to the machine build area",
        )
        group.add_argument(
            "--no-boundaries-check",
            action="store_false",
            dest="boundaries_check",
            help="Disable boundaries check",
        )

        group.add_argument(
            "--boundaries",
            action=CoordinatesAction,
            default=None,
            help='Custom boundaries (e.g. "100, 200, 300"). Overrides boundaries from --machine',
        )

        group.add_argument(
            "--machine",
            default=None,
            required=True,
            choices=self.values["MACHINES_LIST"].keys(),
            help=f"Snapmaker machine",
        )

        group.add_argument(
            "--modkits",
            default=(),
            choices=[arg for kit in self.values["MODKITS_LIST"].values() for arg in kit['arg']],
            nargs="+",
            # action='append',
            help="Modification kits added to the machine:\n" + ", ".join(
                [f"{set(kit['arg'])}: {kit['help']}"
                 for kit in self.values["MODKITS_LIST"].values()]
            )
        )

        group.add_argument(
            "--toolhead",
            default=None,
            choices=self.values["TOOLHEADS_LIST"].keys(),
            help=f"Snapmaker toolhead",
        )

        group.add_argument(
            "--spindle-speeds",
            action=ExtremaAction,
            default=self.values["SPINDLE_SPEEDS"],
            help="Set minimum/maximum spindle speeds as --spindle-speeds='min,max'",
        )

        group.add_argument(
            "--spindle-percent",
            action="store_true",
            default=self.values["SPINDLE_PERCENT"],
            help="use percent as toolhead spindle speed unit (default: use RPM if supported by toolhead, otherwise percent)",
        )

        group.add_argument(
            "--spindle-rpm",
            action="store_false",
            dest="spindle_percent",
            help="Use RPM as toolhead spindle speed unit",
        )

        group.add_argument(
            "--line-number",
            type=int,
            default=self.values["line_number"],
            help="Set the line starting value",
        )

        group.add_argument(
            "--line-increment",
            type=int,
            default=self.values["LINE_INCREMENT"],
            help="Set the line increment value",
        )

        return parser

    def snapmaker_process_arguments(
            self, filename: str = "-"
    ) -> tuple[bool, str | argparse.Namespace]:
        """Process any arguments to the postprocessor."""
        (flag, args) = Path.Post.UtilsArguments.process_shared_arguments(
            self.values, self.parser, self._job.PostProcessorArgs, self.visible_parser, filename
        )
        if flag:  # process extra arguments only if flag is True
            self._units = self.values["UNITS"]

            # --machine is a required "option"
            machine = deepcopy(self.values["MACHINES_LIST"][args.machine])
            self.values["MACHINE"] = machine
            self.values["MACHINE_NAME"] = machine["name"]

            # get toolhead
            compatible_toolheads = {
                n: th for n, th in self.values["TOOLHEADS_LIST"].items()
                if args.machine in th["machines"]
            }
            toolheads_args = {
                arg: n for n, th in compatible_toolheads.items() for arg in th["arg"]
            }

            if args.toolhead:
                if args.toolhead in toolheads_args:
                    toolhead_key = toolheads_args[args.toolhead]
                else:
                    FreeCAD.Console.PrintError(
                        f"{args.toolhead} is not compatible with {machine['name']}.\n"
                        f"\tAllowed values: {[th['arg'] for th in compatible_toolheads.values()]}\n"
                    )
                    return False, args
            elif len(compatible_toolheads) == 1:
                toolhead_key = next(iter(compatible_toolheads))
                FreeCAD.Console.PrintWarning(
                    f"No toolhead argument provided. Using  "
                    f"{self.values['TOOLHEADS_LIST'][toolhead_key]['name']}.\n"
                )
            else:
                FreeCAD.Console.PrintError(
                    f"Missing --toolhead argument.\n"
                    + f"Compatible toolheads for {machine['name']}: "
                    + f"{set(compatible_toolheads)}\n"
                )
                return False, args
            toolhead = deepcopy(self.values["TOOLHEADS_LIST"][toolhead_key])
            self.values["TOOLHEAD"] = toolhead
            self.values["MACHINE_NAME"] += " " + toolhead["name"]

            self.values["SPINDLE_SPEEDS"] = toolhead["spindle_speeds"]
            if args.spindle_speeds:  # may override toolhead value, which is expected
                self.values["SPINDLE_SPEEDS"] = args.spindle_speeds

            # spindle in RPM or percent
            if args.spindle_percent is not None:  # spindle mode requested by the user
                if toolhead["spindle_percent"]:
                    self.values["SPINDLE_PERCENT"] = True
                else:
                    FreeCAD.Console.PrintError(
                        f"Requested spindle speed in percent, but toolhead {toolhead['name']}"
                        + " does not support speed as percent.\n"
                    )
                    return False, args
            else:  # default is RPM, percent otherwise
                self.values["SPINDLE_PERCENT"] = toolhead["spindle_percent"] and not toolhead[
                    "spindle_rpm"]

            # display warning only if there is a mismatch between the request and the result
            if args.spindle_percent is not self.values["SPINDLE_PERCENT"]:
                if self.values["SPINDLE_PERCENT"]:
                    FreeCAD.Console.PrintWarning(
                        "Spindle speed will be controlled using using percentages.\n")
                else:
                    FreeCAD.Console.PrintWarning(
                        "Spindle speed will be controlled using using RPM.\n")

            # addons
            compatible_modkits = {
                n: kit for n, kit in SNAPMAKER_MODKITS.items() if args.machine in kit["machines"]
            }
            modkits_args = {
                arg: n for n, th in compatible_modkits.items() for arg in th["arg"]
            }

            self.values["MODKITS"] = modkits = dict()
            for kit in args.modkits:
                if kit in modkits_args:
                    modkit_key = modkits_args[kit]
                    if modkit_key in modkits:
                        FreeCAD.Console.PrintWarning(
                            f"modkit {compatible_modkits[modkit_key]['name']} already provided"
                        )
                    else:
                        modkits[modkit_key] = deepcopy(compatible_modkits[modkit_key])
                else:
                    FreeCAD.Console.PrintError(
                        f"Modkit {kit} incompatible with {machine['name']}."
                        + f"\tAllowed modkits: {[k['arg'] for k in compatible_modkits.values()]}\n"
                    )
                    return False, args

            # update machine boundaries using toolhead and modkits
            for mod in (*modkits.values(), toolhead):
                machine["boundaries"]["X"] += mod["boundaries_delta"]["X"]
                machine["boundaries"]["Y"] += mod["boundaries_delta"]["Y"]
                machine["boundaries"]["Z"] += mod["boundaries_delta"]["Z"]

            if args.boundaries:  # may override machine boundaries, which is expected
                self.values["BOUNDARIES"] = args.boundaries
                self.values["MACHINE_NAME"] += " Boundaries override=" + str(args.boundaries)
                for (axis, calculated), custom in zip(
                        machine["boundaries"].items(), args.boundaries.values()
                ):
                    if custom > calculated:
                        FreeCAD.Console.PrintWarning(
                            f"Custom {axis} boundary ({custom}) exceed calculated {axis} boundary "
                            + f"({calculated})"
                        )
            else:
                self.values["BOUNDARIES"] = machine["boundaries"]

            self.values["THUMBNAIL"] = args.thumbnail
            self.values["ALLOW_GUI"] = args.gui
            self.values["line_number"] = args.line_number
            self.values["LINE_INCREMENT"] = args.line_increment

            if args.boundaries_check and not self.values["BOUNDARIES"]:
                FreeCAD.Console.PrintError(
                    "Boundary check skipped: no valid boundaries supplied\n")
                self.values["BOUNDARIES_CHECK"] = False
            else:
                self.values["BOUNDARIES_CHECK"] = args.boundaries_check

        return flag, args

    def snapmaker_process_postables(self, filename: str = "-") -> list[tuple[str, str]]:
        """process job sections to gcode"""
        sections: list[tuple[str, str]] = list()

        postables = self._buildPostList()

        # basic filename handling
        if len(postables) > 1 and filename != "-":
            filename = pathlib.Path(filename)
            filename = str(filename.with_stem(filename.stem + "_{name}"))

        for name, objects in postables:
            gcode = self.export_common(objects, filename.format(name=name))
            sections.append((name, gcode))

        return sections

    def get_thumbnail(self) -> str:
        """generate a thumbnail of the job from the given objects"""
        if not self.values["THUMBNAIL"]:
            return "thumbnail: deactivated."

        if not (self.values["ALLOW_GUI"] and FreeCAD.GuiUp):
            FreeCAD.Console.PrintError(
                "GUI access required: thumbnail generation skipped. Consider adding --gui\n"
            )
            return "thumbnail: GUI required."

        # get FreeCAD references
        import FreeCADGui
        from PySide import QtGui

        view = FreeCADGui.activeDocument().activeView()
        selection = FreeCADGui.Selection

        # save current selection
        selected = [
            obj.Object for obj in selection.getCompleteSelection() if hasattr(obj, "Object")
        ]
        selection.clearSelection()

        # clear view
        FreeCADGui.runCommand("Std_SelectAll", 0)
        all = []
        for obj in selection.getCompleteSelection():
            if hasattr(obj, "Object"):
                all.append((obj.Object, obj.Object.Visibility))
                obj.Object.ViewObject.hide()

        # select models to display
        for model in self._job.Model.Group:
            model.ViewObject.show()
            selection.addSelection(model.Document.Name, model.Name)
        view.fitAll()  # center selection
        view.viewIsometric()  # display as isometric
        selection.clearSelection()

        # update GUI to avoid a blank picture
        FreeCADGui.updateGui()
        QtGui.QApplication.processEvents()

        # generate thumbnail
        with tempfile.TemporaryDirectory() as temp:
            path = os.path.join(temp, "thumbnail.png")
            view.saveImage(path, 720, 480, "Transparent")
            with open(path, "rb") as file:
                data = file.read()

        # restore view
        for obj, visibility in all:
            if visibility:
                obj.ViewObject.show()

        # restore selection
        for obj in selected:
            selection.clearSelection()
            selection.addSelection(obj.Document.Name, obj.Name)

        return f"thumbnail: data:image/png;base64,{base64.b64encode(data).decode()}"

    def output_header(self, gcode: list[str]):
        """custom method derived from Path.Post.UtilsExport.output_header"""
        cam_file: str
        comment: str

        if not self.values["OUTPUT_HEADER"]:
            return

        def add_comment(text):
            com = Path.Post.UtilsParse.create_comment(self.values, text)
            gcode.append(f"{Path.Post.UtilsParse.linenumber(self.values)}{com}")

        add_comment("Header Start")
        add_comment("header_type: cnc")
        add_comment(f'machine: {self.values["MACHINE_NAME"]}')
        comment = Path.Post.UtilsParse.create_comment(
            self.values, f'Post Processor: {self.values["POSTPROCESSOR_FILE_NAME"]}'
        )
        gcode.append(f"{Path.Post.UtilsParse.linenumber(self.values)}{comment}")
        if FreeCAD.ActiveDocument:
            cam_file = os.path.basename(FreeCAD.ActiveDocument.FileName)
        else:
            cam_file = "<None>"
        add_comment(f"CAM File: {cam_file}")
        add_comment(f"Output Time: {datetime.datetime.now()}")
        add_comment(self.get_thumbnail())

    def convert_spindle(self, gcode: list[str]) -> list[str]:
        """convert spindle speed values from RPM to percent (%) (M3/M4 commands)"""
        if not self.values["SPINDLE_PERCENT"]:
            return gcode

        # https://wiki.snapmaker.com/en/Snapmaker_Luban/manual/2_supported_gcode_references#m3m4-modified-cnclaser-on
        # Speed as percentage in [0,100]% range
        for index, commandline in enumerate(
                gcode
        ):  # .split(self.values["END_OF_LINE_CHARACTERS"]):
            if match := re.match(r"(?P<command>M0?[34])\D.*(?P<spindle>S\d+.?\d*)", commandline):
                percent = (
                        float(match.group("spindle")[1:]) * 100 / self.values["SPINDLE_SPEEDS"][
                    "max"]
                )
                gcode[index] = (
                        gcode[index][: match.span("spindle")[0]]
                        + f'P{percent:.{self.values["SPINDLE_DECIMALS"]}f}'
                        + gcode[index][match.span("spindle")[1]:]
                )
        return gcode

    def check_boundaries(self, gcode: list[str]) -> bool:
        """Check boundaries and return whether it succeeded"""
        status = True

        extrema = dict(X=[0, 0], Y=[0, 0], Z=[0, 0])
        position = dict(X=0, Y=0, Z=0)
        relative = False

        for index, commandline in enumerate(gcode):
            if re.match(r"G90(?:\D|$)", commandline):
                relative = False
            elif re.match(r"G91(?:\D|$)", commandline):
                relative = True
            elif re.match(r"G0?[12](?:\D|$)", commandline):
                for axis, value in re.findall(
                        r"(?P<axis>[XYZ])(?P<value>-?\d+\.?\d*)(?:\D|$)", commandline
                ):
                    if relative:
                        position[axis] += float(value)
                    else:
                        position[axis] = float(value)
                    extrema[axis][0] = min(extrema[axis][0], position[axis])
                    extrema[axis][1] = max(extrema[axis][1], position[axis])

        for axis in extrema.keys():
            if abs(extrema[axis][1] - extrema[axis][0]) > self.values["BOUNDARIES"][axis]:
                # gcode.insert(0, f';WARNING: Boundary check: job exceeds machine limit on {axis} axis{self.values["END_OF_LINE_CHARACTERS"]}')
                FreeCAD.Console.PrintWarning(
                    f"Boundary check: job exceeds machine limit on {axis} axis\n"
                )
                status = False
        FreeCAD.Console.PrintLog("Boundaries checked\n")
        return status

    def export_common(self, objects: list, filename: str | pathlib.Path) -> str:
        """custom method derived from Path.Post.UtilsExport.export_common"""
        final: str
        gcode: list = []
        result: bool

        for obj in objects:
            if not hasattr(obj, "Path"):
                print(f"The object {obj.Name} is not a path.")
                print("Please select only path and Compounds.")
                return ""

        Path.Post.UtilsExport.check_canned_cycles(self.values)
        self.output_header(gcode)
        Path.Post.UtilsExport.output_safetyblock(self.values, gcode)
        Path.Post.UtilsExport.output_tool_list(self.values, gcode, objects)
        Path.Post.UtilsExport.output_preamble(self.values, gcode)
        Path.Post.UtilsExport.output_motion_mode(self.values, gcode)
        Path.Post.UtilsExport.output_units(self.values, gcode)

        for obj in objects:
            # Skip inactive operations
            if not Path.Base.Util.activeForOp(obj):
                continue
            coolant_mode = Path.Base.Util.coolantModeForOp(obj)
            Path.Post.UtilsExport.output_start_bcnc(self.values, gcode, obj)
            Path.Post.UtilsExport.output_preop(self.values, gcode, obj)
            Path.Post.UtilsExport.output_coolant_on(self.values, gcode, coolant_mode)
            # output the G-code for the group (compound) or simple path
            Path.Post.UtilsParse.parse_a_group(self.values, gcode, obj)

            Path.Post.UtilsExport.output_postop(self.values, gcode, obj)
            Path.Post.UtilsExport.output_coolant_off(self.values, gcode, coolant_mode)

        Path.Post.UtilsExport.output_return_to(self.values, gcode)
        #
        # This doesn't make sense to me.  It seems that both output_start_bcnc and
        # output_end_bcnc should be in the for loop or both should be out of the
        # for loop.  However, that is the way that grbl post code was written, so
        # for now I will leave it that way until someone has time to figure it out.
        #
        Path.Post.UtilsExport.output_end_bcnc(self.values, gcode)
        Path.Post.UtilsExport.output_postamble_header(self.values, gcode)
        Path.Post.UtilsExport.output_tool_return(self.values, gcode)
        Path.Post.UtilsExport.output_safetyblock(self.values, gcode)
        Path.Post.UtilsExport.output_postamble(self.values, gcode)
        gcode = self.convert_spindle(gcode)

        if self.values["BOUNDARIES_CHECK"]:
            self.check_boundaries(gcode)

        # EOL is "\n" for the editor, it will eventually be replaced after
        gcode.append("")
        final = "\n".join(gcode)

        if FreeCAD.GuiUp and self.values["SHOW_EDITOR"]:
            # size limit removed as irrelevant on my computer - see if issues occur
            dia = Path.Post.Utils.GCodeEditorDialog()
            dia.editor.setText(final)
            if dia.exec_():
                final = dia.editor.toPlainText()

        if self.values["END_OF_LINE_CHARACTERS"] != '\n':
            final = final.replace("\n", self.values["END_OF_LINE_CHARACTERS"])

        if not filename == "-":
            with open(filename, "w", encoding="utf-8", newline="") as gfile:
                gfile.write(final)

        return final

    def export(self, filename: str | pathlib.Path = "-"):
        """process gcode and export"""
        (flag, args) = self.snapmaker_process_arguments()
        if flag:
            return self.snapmaker_process_postables(filename)
        if args is None:
            return None
        return [("allitems", args)]

    @property
    def tooltip(self) -> str:
        tooltip = "Postprocessor of the FreeCAD CAM workbench for the Snapmaker machines"
        return tooltip

    @property
    def tooltipArgs(self) -> str:
        return self.parser.format_help()


if __name__ == "__main__":
    Snapmaker(None).visible_parser.format_help()
