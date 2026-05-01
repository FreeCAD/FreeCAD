# SPDX-License-Identifier: LGPL-2.1-or-later
"""Drilling-cycle scenarios (peck and peck+chipbreak)."""

import FreeCAD
import Part
import Path
import Path.Base.Generator.drill as drill
from CAMTests.post.post_harness import Scenario


def _drill_holes(op, chip_break):
    commandlist = []
    for x in (0, 20, 40):
        drillcommands = drill.generate(
            Part.makeLine(
                FreeCAD.Vector(x, 0, 10),
                FreeCAD.Vector(x, 0, 0),
            ),
            dwelltime=0.0,
            peckdepth=2.0,
            repeat=1,
            retractheight=None,
            chipBreak=chip_break,
            feedRetract=False,
        )
        commandlist.append(Path.Command(f"G0 X{x} Y0"))
        for command in drillcommands:
            params = command.Parameters
            params["F"] = 100.0
            command.Parameters = params
            commandlist.append(command)
    op.Path = Path.Path(commandlist)


def _peck(job, op, tc):
    _drill_holes(op, chip_break=False)


def _peck_chipbreak(job, op, tc):
    _drill_holes(op, chip_break=True)


scenarios = [
    Scenario(
        name="drilling_peck",
        variant="default",
        post_args="--no-header --no-show-editor",
        build=_peck,
        skip_dialects=["centroid_legacy", "generic", "mach3_mach4_legacy"],
    ),
    Scenario(
        name="drilling_peck_chipbreak",
        variant="default",
        post_args="--no-header --no-show-editor",
        build=_peck_chipbreak,
        skip_dialects=["centroid_legacy", "generic", "mach3_mach4_legacy"],
    ),
]
