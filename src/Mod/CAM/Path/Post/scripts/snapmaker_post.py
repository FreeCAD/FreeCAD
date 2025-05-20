#!/usr/bin/env python3
# A FreeCAD postprocessor targeting Snapmaker machines with CNC capabilities
# ***************************************************************************
# *  Copyright (c) 2025 Clair-Loup Sergent <clsergent@free.fr>              *
# *                                                                         *
# *  Licensed under the EUPL-1.2 with the specific provision                *
# *  (EUPL articles 14 & 15) that the applicable law is the French law.     *
# *  and the Jurisdiction Paris.                                            *
# *  Any redistribution must include the specific provision above.          *
# *                                                                         *
# *  You may obtain a copy of the Licence at:                               *
# *  https://interoperable-europe.ec.europa.eu/collection/eupl/eupl-text-eupl-12 *
# *                                                                         *
# *  Unless required by applicable law or agreed to in writing, software    *
# *  distributed under the Licence is distributed on an "AS IS" basis,      *
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        *
# *  implied. See the Licence for the specific language governing           *
# *  permissions and limitations under the Licence.                         *
# ***************************************************************************


import argparse
import base64
import copy
import datetime
import os
import pathlib
import re
import tempfile
from typing import Any, List, Tuple

import FreeCAD
import Path
import Path.Base.Util as PathUtil
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


def convert_option_to_attr(option_name):
    # transforms argparse options into identifiers
    if option_name.startswith("--"):
        option_name = option_name[2:]
    elif option_name.startswith("-"):
        option_name = option_name[1:]

    return option_name.replace("-", "_")


SNAPMAKER_MACHINES = dict(
    Original=dict(
        key="Original",
        name="Snapmaker Original",
        boundaries_table=[
            # https://forum.snapmaker.com/t/cnc-work-area-size/5178
            dict(boundaries=dict(X=125, Y=125, Z=50), toolhead="Original_CNC", mods=set()),
        ],
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    Original_Z_Extension=dict(
        key="Original_Z_Extension",
        name="Snapmaker Original with Z extension",
        boundaries_table=[
            # https://forum.snapmaker.com/t/cnc-work-area-size/5178
            dict(boundaries=dict(X=125, Y=125, Z=146), toolhead="Original_CNC", mods=set()),
        ],
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    A150=dict(
        key="A150",
        name="Snapmaker 2 A150",
        boundaries_table=[
            # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
            dict(boundaries=dict(X=145, Y=160, Z=90), toolhead="50W_CNC", mods=set()),
            dict(boundaries=dict(X=145, Y=148, Z=90), toolhead="50W_CNC", mods={"BK"}),
        ],
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    A250=dict(
        key="A250",
        name="Snapmaker 2 A250",
        boundaries_table=[
            # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
            dict(boundaries=dict(X=230, Y=250, Z=180), toolhead="50W_CNC", mods=set()),
            dict(boundaries=dict(X=230, Y=238, Z=180), toolhead="50W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=230, Y=235, Z=180), toolhead="50W_CNC", mods={"QS"}),
            dict(boundaries=dict(X=230, Y=223, Z=180), toolhead="50W_CNC", mods={"BK", "QS"}),
            dict(boundaries=dict(X=230, Y=225, Z=180), toolhead="200W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=230, Y=210, Z=180), toolhead="200W_CNC", mods={"BK", "QS"}),
        ],
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    A250T=dict(
        key="A250T",
        name="Snapmaker 2 A250T",
        boundaries_table=[
            # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
            dict(boundaries=dict(X=230, Y=250, Z=180), toolhead="50W_CNC", mods=set()),
            dict(boundaries=dict(X=230, Y=238, Z=180), toolhead="50W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=230, Y=235, Z=180), toolhead="50W_CNC", mods={"QS"}),
            dict(boundaries=dict(X=230, Y=223, Z=180), toolhead="50W_CNC", mods={"BK", "QS"}),
            dict(boundaries=dict(X=230, Y=225, Z=180), toolhead="200W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=230, Y=210, Z=180), toolhead="200W_CNC", mods={"BK", "QS"}),
        ],
        lead=dict(X=20, Y=20, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    A350=dict(
        key="A350",
        name="Snapmaker 2 A350",
        boundaries_table=[
            # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
            dict(boundaries=dict(X=320, Y=350, Z=275), toolhead="50W_CNC", mods=set()),
            dict(boundaries=dict(X=320, Y=338, Z=275), toolhead="50W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=320, Y=335, Z=275), toolhead="50W_CNC", mods={"QS"}),
            dict(boundaries=dict(X=320, Y=323, Z=275), toolhead="50W_CNC", mods={"BK", "QS"}),
            dict(boundaries=dict(X=320, Y=325, Z=275), toolhead="200W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=320, Y=310, Z=275), toolhead="200W_CNC", mods={"BK", "QS"}),
        ],
        lead=dict(X=8, Y=8, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    A350T=dict(
        key="A350T",
        name="Snapmaker 2 A350T",
        boundaries_table=[
            # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
            dict(boundaries=dict(X=320, Y=350, Z=275), toolhead="50W_CNC", mods=set()),
            dict(boundaries=dict(X=320, Y=338, Z=275), toolhead="50W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=320, Y=335, Z=275), toolhead="50W_CNC", mods={"QS"}),
            dict(boundaries=dict(X=320, Y=323, Z=275), toolhead="50W_CNC", mods={"BK", "QS"}),
            dict(boundaries=dict(X=320, Y=325, Z=275), toolhead="200W_CNC", mods={"BK"}),
            dict(boundaries=dict(X=320, Y=310, Z=275), toolhead="200W_CNC", mods={"BK", "QS"}),
        ],
        lead=dict(X=20, Y=20, Z=8),  # Linear module screw pitch (mm/turn)
    ),
    Artisan=dict(
        key="Artisan",
        name="Snapmaker Artisan",
        boundaries_table=[
            dict(boundaries=dict(X=400, Y=400, Z=400), toolhead="200W_CNC", mods=set()),
        ],
        lead=dict(X=40, Y=40, Z=8),  # Linear module screw pitch (mm/turn)
    ),
)

# These modifications were released to upgrade the Snapmaker 2.0 machines
# which started on Kickstarter.
SNAPMAKER_MOD_KITS = {
    "QS": dict(
        key="QS",
        name="Quick Swap Kit",
        option_name="--quick-swap",
        option_help_text="Indicates that the quick swap kit is installed. Only compatible with Snapmaker 2 machines.",
    ),
    "BK": dict(
        key="BK",
        name="Bracing Kit",
        option_name="--bracing-kit",
        option_help_text="Indicates that the bracing kit is installed. Only compatible with Snapmaker 2 machines.",
    ),
}

# Could support other types of toolheads (laser, drag knife, 3DP, ...) in the future
# https://wiki.snapmaker.com/en/Snapmaker_Luban/manual/2_supported_gcode_references#m3m4-modified-cnclaser-on
SNAPMAKER_TOOLHEADS = {
    "Original_CNC": dict(
        key="Original_CNC",
        name="Original CNC module",
        speed_rpm=dict(min=0, max=7000),
        boundaries_delta=dict(X=0, Y=0, Z=0),
        has_percent=True,
        has_speed_s=False,
    ),
    "50W_CNC": dict(
        key="50W_CNC",
        name="50W CNC module",
        speed_rpm=dict(min=0, max=12000),
        boundaries_delta=dict(X=0, Y=0, Z=0),
        has_percent=True,
        has_speed_s=False,
    ),
    "200W_CNC": dict(
        key="200W_CNC",
        name="200W CNC module",
        speed_rpm=dict(min=0, max=18000),
        boundaries_delta=dict(X=0, Y=-13, Z=0),
        has_percent=True,
        has_speed_s=True,
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
            raise argparse.ArgumentError(None, message="invalid values provided, should be int,int")


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
        self.values: dict[str, Any] = dict()
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
        self.values["MACHINES"] = SNAPMAKER_MACHINES
        self.values["MOD_KITS_ALL"] = SNAPMAKER_MOD_KITS
        self.values["TOOLHEADS"] = SNAPMAKER_TOOLHEADS
        self.values["TOOLHEAD_NAME"] = None
        self.values["SPINDLE_SPEEDS"] = dict()
        self.values["SPINDLE_PERCENT"] = None

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
            "--no-thumbnail", action="store_false", dest="thumbnail", help="Remove thumbnail"
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
            choices=self.values["MACHINES"].keys(),
            help=f"Snapmaker machine. Choose from [{self.values['MACHINES'].keys()}].",
        )

        for key, value in SNAPMAKER_MOD_KITS.items():
            group.add_argument(
                value["option_name"],
                default=False,
                action="store_true",
                help=value["option_help_text"],
            )

        group.add_argument(
            "--toolhead",
            default=None,
            choices=self.values["TOOLHEADS"].keys(),
            help=f"Snapmaker toolhead. Choose from [{self.values['TOOLHEADS'].keys()}].",
        )

        group.add_argument(
            "--spindle-speeds",
            action=ExtremaAction,
            default=None,
            help="Set minimum/maximum spindle speeds as --spindle-speeds='min,max'",
        )

        group.add_argument(
            "--spindle-percent",
            action="store_true",
            default=None,
            help="use percent as toolhead spindle speed unit (default: use RPM if supported by toolhead, otherwise percent)",
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
    ) -> Tuple[bool, str | argparse.Namespace]:
        """Process any arguments to the postprocessor."""
        (flag, args) = Path.Post.UtilsArguments.process_shared_arguments(
            self.values, self.parser, self._job.PostProcessorArgs, self.visible_parser, filename
        )
        if flag:  # process extra arguments only if flag is True
            self._units = self.values["UNITS"]

            # --machine is a required "option"
            machine = self.values["MACHINES"][args.machine]
            self.values["MACHINE_KEY"] = machine["key"]
            self.values["MACHINE_NAME"] = machine["name"]

            compatible_toolheads = {bt["toolhead"] for bt in machine["boundaries_table"]}

            if args.toolhead:
                if args.toolhead not in compatible_toolheads:
                    FreeCAD.Console.PrintError(
                        f"Selected --toolhead={args.toolhead} is not compatible with machine {machine['name']}."
                        + f" Choose from [{compatible_toolheads}]\n"
                    )
                    flag = False
                    return (flag, args)
                toolhead = self.values["TOOLHEADS"][args.toolhead]
            elif len(compatible_toolheads) == 1:
                toolhead_key = next(iter(compatible_toolheads))
                toolhead = self.values["TOOLHEADS"][toolhead_key]
            else:
                FreeCAD.Console.PrintError(
                    f"Machine {machine['name']} has multiple compatible toolheads:\n"
                    f"{compatible_toolheads}\n"
                    "Please add --toolhead argument.\n"
                )
                flag = False
                return (flag, args)
            self.values["TOOLHEAD_KEY"] = toolhead["key"]
            self.values["TOOLHEAD_NAME"] = toolhead["name"]

            self.values["SPINDLE_SPEEDS"] = toolhead["speed_rpm"]

            if args.spindle_speeds:  # may override toolhead value, which is expected
                self.values["SPINDLE_SPEEDS"] = args.spindle_speeds

            if args.spindle_percent is not None:
                if toolhead["has_percent"]:
                    self.values["SPINDLE_PERCENT"] = True
                else:
                    FreeCAD.Console.PrintError(
                        f"Requested spindle speed in percent, but toolhead {toolhead['name']}"
                        + " does not support speed as percent.\n"
                    )
                    flag = False
                    return (flag, args)
            else:
                # Prefer speed S over percent P
                self.values["SPINDLE_PERCENT"] = (
                    toolhead["has_percent"] and not toolhead["has_speed_s"]
                )
            if self.values["SPINDLE_PERCENT"]:
                FreeCAD.Console.PrintWarning(
                    "Spindle speed will be controlled using using percentages.\n"
                )
            else:
                FreeCAD.Console.PrintWarning("Spindle speed will be controlled using using RPM.\n")

            self.values["MOD_KITS_INSTALLED"] = []
            if args.boundaries:  # may override machine boundaries, which is expected
                self.values["BOUNDARIES"] = args.boundaries
                self.values["MACHINE_NAME"] += " Boundaries override=" + str(args.boundaries)
            else:
                compatible_modkit_combos = [
                    bt["mods"]
                    for bt in machine["boundaries_table"]
                    if toolhead["key"] == bt["toolhead"]
                ]
                configured_modkits = set()

                # Determine which mod kits are requested from the options
                for mod_kit in self.values["MOD_KITS_ALL"].values():
                    if getattr(args, convert_option_to_attr(mod_kit["option_name"])):
                        configured_modkits.add(mod_kit["key"])
                        self.values["MACHINE_NAME"] += " " + mod_kit["name"]
                        self.values["MOD_KITS_INSTALLED"].append(mod_kit["key"])

                if configured_modkits not in compatible_modkit_combos:
                    FreeCAD.Console.PrintError(
                        f"Machine {machine['name']} with toolhead {toolhead['name']}"
                        + f" is not compatible with modkit {configured_modkits if configured_modkits else None}.\n"
                        + f" Choose from {compatible_modkit_combos}."
                    )
                    flag = False
                    return (flag, args)

                # Update machine dimensions based on installed toolhead and mod kits
                boundaries_table_entry_l = [
                    bt
                    for bt in machine["boundaries_table"]
                    if bt["toolhead"] == toolhead["key"] and bt["mods"] == configured_modkits
                ]
                assert len(boundaries_table_entry_l) == 1
                boundaries_table_entry = boundaries_table_entry_l[0]

                # The deepcopy is necessary to avoid modifying the boundaries in the MACHINES dict.
                self.values["BOUNDARIES"] = copy.deepcopy(boundaries_table_entry["boundaries"])
                self.values["MACHINE_NAME"] += " " + toolhead["name"]

            self.values["THUMBNAIL"] = args.thumbnail
            self.values["ALLOW_GUI"] = args.gui
            self.values["line_number"] = args.line_number
            self.values["LINE_INCREMENT"] = args.line_increment

            if args.boundaries_check and not self.values["BOUNDARIES"]:
                FreeCAD.Console.PrintError("Boundary check skipped: no valid boundaries supplied\n")
                self.values["BOUNDARIES_CHECK"] = False
            else:
                self.values["BOUNDARIES_CHECK"] = args.boundaries_check

        return flag, args

    def snapmaker_process_postables(self, filename: str = "-") -> List[Tuple[str, str]]:
        """process job sections to gcode"""
        sections: List[Tuple[str, str]] = list()

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
        if self.values["THUMBNAIL"] is False:
            return "thumbnail: deactivated."

        if not (self.values["ALLOW_GUI"] and FreeCAD.GuiUp):
            FreeCAD.Console.PrintError(
                "GUI access required: thumbnail generation skipped. Consider adding --gui\n"
            )
            return "thumbnail: GUI required."

        # get FreeCAD references
        import FreeCADGui

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

    def output_header(self, gcode: List[str]):
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

    def convert_spindle(self, gcode: List[str]) -> List[str]:
        """convert spindle speed values from RPM to percent (%) (M3/M4 commands)"""
        if self.values["SPINDLE_PERCENT"] is False:
            return gcode

        # https://wiki.snapmaker.com/en/Snapmaker_Luban/manual/2_supported_gcode_references#m3m4-modified-cnclaser-on
        # Speed as percentage in [0,100]% range
        for index, commandline in enumerate(
            gcode
        ):  # .split(self.values["END_OF_LINE_CHARACTERS"]):
            if match := re.match(r"(?P<command>M0?[34])\D.*(?P<spindle>S\d+.?\d*)", commandline):
                percent = (
                    float(match.group("spindle")[1:]) * 100 / self.values["SPINDLE_SPEEDS"]["max"]
                )
                gcode[index] = (
                    gcode[index][: match.span("spindle")[0]]
                    + f'P{percent:.{self.values["SPINDLE_DECIMALS"]}f}'
                    + gcode[index][match.span("spindle")[1] :]
                )
        return gcode

    def check_boundaries(self, gcode: List[str]) -> bool:
        """Check boundaries and return whether it succeeded"""
        status = True
        FreeCAD.Console.PrintLog("Boundaries check\n")

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

        return status

    def export_common(self, objects: List, filename: str | pathlib.Path) -> str:
        """custom method derived from Path.Post.UtilsExport.export_common"""
        final: str
        gcode: List = []
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
            if not PathUtil.activeForOp(obj):
                continue
            coolant_mode = PathUtil.coolantModeForOp(obj)
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

        # add the appropriate end-of-line characters to the gcode, including after the last line
        gcode.append("")
        final = self.values["END_OF_LINE_CHARACTERS"].join(gcode)

        if FreeCAD.GuiUp and self.values["SHOW_EDITOR"]:
            # size limit removed as irrelevant on my computer - see if issues occur
            dia = PostUtils.GCodeEditorDialog()
            # the editor expects lines to end in "\n", and returns lines ending in "\n"
            if self.values["END_OF_LINE_CHARACTERS"] == "\n":
                dia.editor.setText(final)
                if dia.exec_():
                    final = dia.editor.toPlainText()
            else:
                final_for_editor = "\n".join(gcode)
                dia.editor.setText(final_for_editor)
                if dia.exec_():
                    final_for_editor = dia.editor.toPlainText()
                    # convert all "\n" to the appropriate end-of-line characters
                    final = final_for_editor.replace("\n", self.values["END_OF_LINE_CHARACTERS"])

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
