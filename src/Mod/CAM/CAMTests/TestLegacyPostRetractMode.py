# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

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

"""Regression coverage for the "legacy" (non-machine, non-Processor.py) post
scripts and the G98/G99 retract-mode gap.

Path.Op.Drilling only *annotates* each canned-cycle command with its retract
mode (RetractMode: "G98"/"G99") for cannedCycleTerminator's benefit. These
scripts never called cannedCycleTerminator and only ever learned the retract
mode from a literal G98/G99 command in the stream -- so a KeepToolDown=True
(G99) Drilling operation was silently treated as G98 by every one of them.
Fixed by running the path through PathUtils.getPathWithPlacementAndTerminator
before their own per-command scan, so a literal G98/G99 precedes each cycle
group.

Two families of script are covered, and they need different assertions:

* Literal posts (linuxcnc, fanuc, uccnc, dynapath, mach3_mach4, centroid,
  KineticNCBeamicon2) pass G98/G99 straight through, so we look for it as a
  real command -- not inside a comment.

* Translating posts (grbl, marlin, rrf, estlcam) expand canned cycles into
  G0/G1 moves and comment out the G98/G99 they consume, so "G99" appears in
  the output either way. What actually changes is the retract height: G99
  retracts to the R plane (PeckRetract), G98 to the Z the cycle started from.
  We assert on that.

Tapping (G84/G74) is covered too: cannedCycleTerminator used to match only
GCODE_MOVE_DRILL, so a tapping op's RetractMode annotation was ignored and
it got neither a G98/G99 nor a G80.
"""

import re
from importlib import reload

import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
import Path.Op.Drilling as PathDrilling
import Path.Tool.Controller as PathToolController
from Path.Tool.toolbit import ToolBit
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

START_DEPTH = 5.0
FINAL_DEPTH = -10.0
# The canned-cycle R word. Must differ from the pre-cycle Z (below) or G98 and
# G99 retract to the same height and the translating posts cannot be told apart.
PECK_RETRACT = 5.0
# Drilling rapids to StartDepth + retract clearance before the cycle; that is
# the "initial Z" a G98 retract returns to.
INITIAL_Z = 14.0

LITERAL_POSTS = [
    "linuxcnc_legacy_post",
    "fanuc_legacy_post",
    "uccnc_legacy_post",
    "dynapath_legacy_post",
    "mach3_mach4_legacy_post",
    "centroid_legacy_post",
    "KineticNCBeamicon2_legacy_post",
]
TRANSLATING_POSTS = [
    "grbl_legacy_post",
    "marlin_legacy_post",
    "rrf_legacy_post",
    "estlcam_legacy_post",
]

# A G-word at the start of a line, outside any ( comment ). Number formatting
# differs between posts (Z5.000 vs Z5.0000), hence the regex on the Z value.
_CODE_LINE = re.compile(r"^\s*(?:N\d+\s+)?(G\d+)")
_G0_Z = re.compile(r"^\s*(?:N\d+\s+)?G0+\s+Z(-?\d+\.\d+)")


def _codes(gcode):
    """The leading G-word of every non-comment line, in order."""
    return [m.group(1) for m in map(_CODE_LINE.match, gcode.splitlines()) if m]


def _cycle_body(gcode):
    """The lines between the (possibly commented) cycle command and its G80."""
    lines = gcode.splitlines()
    start = next(i for i, l in enumerate(lines) if re.search(r"G8[34]\b", l))
    end = next(i for i, l in enumerate(lines) if i > start and "G80" in l)
    return lines[start + 1 : end]


class TestLegacyPostRetractMode(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestLegacyPostRetractMode")
        base = self.doc.addObject("Part::Feature", "Base")
        base.Shape = Part.makeBox(20, 20, 10)
        self.job = PathJob.Create("Job", [base], None)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _controller(self, shape_id, number):
        tool = ToolBit.from_shape_id(shape_id).attach_to_doc(doc=self.doc)
        tc = PathToolController.Create(f"TC{number}", tool, number)
        tc.HorizFeed = 100
        tc.VertFeed = 100
        tc.HorizRapid = 200
        tc.VertRapid = 200
        tc.SpindleSpeed = 1000
        return tool, tc

    def _operation(self, strategy, keepToolDown):
        if strategy == "Tapping":
            tool, tc = self._controller("tap.fcstd", 2)
            tool.Pitch = 1.25  # the shipped tap shape has Pitch 0, which skips the hole
        else:
            tool, tc = self._controller("drill.fcstd", 1)
        # One controller per job: findToolController() cannot pick between
        # several without a GUI.
        self.job.Tools.Group = [tc]

        op = PathDrilling.Create("Op", parentJob=self.job)
        op.ToolController = tc
        op.Strategy = strategy
        op.Locations = [FreeCAD.Vector(2, 2, 0)]
        op.setExpression("StartDepth", None)
        op.StartDepth = START_DEPTH
        op.setExpression("FinalDepth", None)
        op.FinalDepth = FINAL_DEPTH
        if strategy == "Drilling":
            op.PeckEnabled = True
            op.PeckDepth = 2.0
            op.setExpression("PeckRetract", None)  # defaults to =SafeHeight
            op.PeckRetract = PECK_RETRACT
        op.KeepToolDown = keepToolDown
        op.Proxy.execute(op)
        return op

    def _export(self, module_name, op):
        module = __import__(f"Path.Post.scripts.{module_name}", fromlist=[module_name])
        reload(module)
        gcode = module.export([op], "-", "")
        self.assertIsNotNone(gcode, f"{module_name}.export() returned None")
        return gcode

    # -- literal posts ------------------------------------------------------

    def _assertLiteralRetractMode(self, module_name, strategy, keepToolDown):
        want, unwanted = ("G99", "G98") if keepToolDown else ("G98", "G99")
        cycle = "G84" if strategy == "Tapping" else "G83"
        gcode = self._export(module_name, self._operation(strategy, keepToolDown))
        codes = _codes(gcode)
        self.assertIn(want, codes, f"{module_name}: no literal {want} in:\n{gcode}")
        self.assertNotIn(unwanted, codes, f"{module_name}: stray {unwanted} in:\n{gcode}")
        self.assertLess(
            codes.index(want),
            codes.index(cycle),
            f"{module_name}: {want} must precede {cycle} in:\n{gcode}",
        )
        self.assertIn("G80", codes[codes.index(cycle) :], f"{module_name}: cycle never terminated")

    def test_literal_drilling_g99(self):
        for name in LITERAL_POSTS:
            with self.subTest(post=name):
                self._assertLiteralRetractMode(name, "Drilling", keepToolDown=True)

    def test_literal_drilling_g98(self):
        for name in LITERAL_POSTS:
            with self.subTest(post=name):
                self._assertLiteralRetractMode(name, "Drilling", keepToolDown=False)

    def test_literal_tapping_g99(self):
        for name in LITERAL_POSTS:
            with self.subTest(post=name):
                self._assertLiteralRetractMode(name, "Tapping", keepToolDown=True)

    def test_literal_tapping_g98(self):
        for name in LITERAL_POSTS:
            with self.subTest(post=name):
                self._assertLiteralRetractMode(name, "Tapping", keepToolDown=False)

    def test_fanuc_tapping_single_g80(self):
        # fanuc appends its own G80 after G84; make sure it does not stack
        # on top of the one cannedCycleTerminator now inserts.
        gcode = self._export("fanuc_legacy_post", self._operation("Tapping", True))
        codes = _codes(gcode)
        body = codes[codes.index("G84") : codes.index("G84") + 3]
        self.assertEqual(body.count("G80"), 1, f"fanuc: stacked G80 in:\n{gcode}")

    # -- translating posts --------------------------------------------------

    def _retracts(self, gcode):
        return [float(m.group(1)) for m in map(_G0_Z.match, _cycle_body(gcode)) if m]

    def test_translating_drilling_g99_retracts_to_r_plane(self):
        for name in TRANSLATING_POSTS:
            with self.subTest(post=name):
                gcode = self._export(name, self._operation("Drilling", True))
                retracts = self._retracts(gcode)
                self.assertTrue(retracts, f"{name}: no expanded pecks in:\n{gcode}")
                self.assertIn(PECK_RETRACT, retracts, f"{name}: never retracted to R:\n{gcode}")
                self.assertNotIn(
                    INITIAL_Z, retracts, f"{name}: G99 retracted to initial Z:\n{gcode}"
                )

    def test_translating_drilling_g98_retracts_to_initial_z(self):
        for name in TRANSLATING_POSTS:
            with self.subTest(post=name):
                gcode = self._export(name, self._operation("Drilling", False))
                retracts = self._retracts(gcode)
                self.assertTrue(retracts, f"{name}: no expanded pecks in:\n{gcode}")
                self.assertIn(
                    INITIAL_Z, retracts, f"{name}: G98 never retracted to initial Z:\n{gcode}"
                )
