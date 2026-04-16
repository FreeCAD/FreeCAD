# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 FreeCAD Project Association                        *
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

"""Unit tests for the Draft Workbench, GUI dimension tests."""

import math
import os
import re
import tempfile
import xml.etree.ElementTree as ET

import Draft
import DraftVecUtils
import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Vector

from drafttests import test_base


class DraftGuiDimension(test_base.DraftTestCaseDoc):
    """GUI regressions for Draft dimensions."""

    FREECAD_NAMESPACE = "https://www.freecad.org/ns"
    OVERSHOOT_TRANSFORM_RE = re.compile(
        r"rotate\((?P<angle>[-+0-9.eE]+),(?P<rot_x>[-+0-9.eE]+),(?P<rot_y>[-+0-9.eE]+)\)\s+"
        r"translate\((?P<tx>[-+0-9.eE]+),(?P<ty>[-+0-9.eE]+)\)"
    )

    def setUp(self):
        """Set up a temporary Draft document and keep a stable name for teardown."""
        super().setUp()
        self.doc_name = self.doc.Name

    def tearDown(self):
        """Close the temporary document even if save/reopen invalidated the Python wrapper."""
        if self.doc_name in App.listDocuments():
            App.closeDocument(self.doc_name)

    def getAngularOvershootState(self, dimension):
        """Return the angular overshoot scenegraph state used by the regression checks."""
        vobj = dimension.ViewObject
        proxy = vobj.Proxy
        return {
            "dim_children": proxy.marksDimOvershoot.getNumChildren(),
            "ext_children": proxy.marksExtOvershoot.getNumChildren(),
            "dim_scale_start": tuple(proxy.transDimOvershoot1.scaleFactor.getValue()),
            "dim_scale_end": tuple(proxy.transDimOvershoot2.scaleFactor.getValue()),
            "ext_scale_start": tuple(proxy.transExtOvershoot1.scaleFactor.getValue()),
            "ext_scale_end": tuple(proxy.transExtOvershoot2.scaleFactor.getValue()),
            "dim_translation_start": tuple(proxy.transDimOvershoot1.translation.getValue()),
            "dim_translation_end": tuple(proxy.transDimOvershoot2.translation.getValue()),
            "ext_translation_start": tuple(proxy.transExtOvershoot1.translation.getValue()),
            "ext_translation_end": tuple(proxy.transExtOvershoot2.translation.getValue()),
            "coord_start": tuple(proxy.coord1.point.getValues()[0].getValue()),
            "coord_end": tuple(proxy.coord2.point.getValues()[0].getValue()),
            "dim_rotation_start": tuple(proxy.transDimOvershoot1.rotation.getValue().getValue()),
            "dim_rotation_end": tuple(proxy.transDimOvershoot2.rotation.getValue().getValue()),
            "ext_rotation_start": tuple(proxy.transExtOvershoot1.rotation.getValue().getValue()),
            "ext_rotation_end": tuple(proxy.transExtOvershoot2.rotation.getValue().getValue()),
        }

    def assertRotationAlmostEqual(self, actual, expected):
        """Compare Coin quaternion values with a tolerance."""
        for actual_component, expected_component in zip(actual, expected):
            self.assertAlmostEqual(actual_component, expected_component, places=6)

    def normalizeAngleDegrees(self, angle):
        """Normalize an SVG rotation angle so wraparound is easy to compare."""
        normalized = math.fmod(angle, 360.0)
        if normalized < 0:
            normalized += 360.0
        return normalized

    def angleDeltaDegrees(self, actual, expected):
        """Return the shortest distance between two rotation angles."""
        delta = abs(self.normalizeAngleDegrees(actual) - self.normalizeAngleDegrees(expected))
        return min(delta, 360.0 - delta)

    def makeAngularDimension(self):
        """Create a stable angular dimension setup shared by the GUI regressions."""
        dimension = Draft.make_angular_dimension(Vector(0, 0, 0), [20, 70], Vector(3, 1, 0))
        vobj = dimension.ViewObject
        vobj.DisplayMode = "World"
        vobj.ArrowTypeStart = "Dot"
        vobj.ArrowTypeEnd = "Dot"
        return dimension

    def getAngularSvg(self, dimension):
        """Return the Draft SVG body used by TechDraw DraftView for this dimension."""
        Gui.updateGui()
        return Draft.get_svg(dimension, direction=Vector(0, 0, 1), techdraw=True)

    def extractOvershootLines(self, svg_text):
        """Extract overshoot line transforms from Draft SVG output."""
        root = ET.fromstring(f'<svg xmlns:freecad="{self.FREECAD_NAMESPACE}">{svg_text}</svg>')
        lines = []
        for element in root.iter():
            if not element.tag.endswith("line"):
                continue
            skip_value = next(
                (
                    value
                    for key, value in element.attrib.items()
                    if key == "freecad:skip" or key == "skip" or key.endswith("}skip")
                ),
                None,
            )
            if skip_value != "1":
                continue

            x1 = float(element.attrib.get("x1", "nan"))
            y1 = float(element.attrib.get("y1", "nan"))
            x2 = float(element.attrib.get("x2", "nan"))
            y2 = float(element.attrib.get("y2", "nan"))
            if abs(x1) > 1e-9 or abs(y1) > 1e-9 or abs(y2) > 1e-9 or x2 >= 0:
                continue

            transform = element.attrib.get("transform", "")
            match = self.OVERSHOOT_TRANSFORM_RE.fullmatch(transform)
            self.assertIsNotNone(match, msg=f"unexpected overshoot transform: {transform}")

            tx = float(match.group("tx"))
            ty = float(match.group("ty"))
            rot_x = float(match.group("rot_x"))
            rot_y = float(match.group("rot_y"))
            self.assertAlmostEqual(tx, rot_x, places=6)
            self.assertAlmostEqual(ty, rot_y, places=6)

            lines.append(
                {
                    "anchor": (tx, ty),
                    "angle": self.normalizeAngleDegrees(float(match.group("angle"))),
                    "length": abs(x2),
                }
            )
        return lines

    def getExpectedAngularSvgOvershoots(self, dimension):
        """Compute the expected TechDraw overshoot lines from the angular geometry."""
        vobj = dimension.ViewObject
        proxy = vobj.Proxy
        pointratio = 0.75
        expected = []

        if vobj.DimOvershoot.Value:
            shootsize = vobj.DimOvershoot.Value / pointratio
            tangent1 = proxy.circle.tangentAt(proxy.circle.FirstParameter)
            tangent2 = proxy.circle.tangentAt(proxy.circle.LastParameter)
            if not DraftVecUtils.isNull(tangent1):
                expected.append(
                    {
                        "anchor": (float(proxy.p2.x), float(proxy.p2.y)),
                        "angle": self.normalizeAngleDegrees(
                            math.degrees(-DraftVecUtils.angle(tangent1))
                        ),
                        "length": shootsize,
                    }
                )
            if not DraftVecUtils.isNull(tangent2):
                expected.append(
                    {
                        "anchor": (float(proxy.p3.x), float(proxy.p3.y)),
                        "angle": self.normalizeAngleDegrees(
                            math.degrees(-DraftVecUtils.angle(tangent2) + math.pi)
                        ),
                        "length": shootsize,
                    }
                )

        if vobj.ExtOvershoot.Value:
            shootsize = vobj.ExtOvershoot.Value / pointratio
            ext1 = proxy.p1 - proxy.p2
            ext2 = proxy.p4 - proxy.p3
            if not DraftVecUtils.isNull(ext1):
                expected.append(
                    {
                        "anchor": (float(proxy.p2.x), float(proxy.p2.y)),
                        "angle": self.normalizeAngleDegrees(
                            math.degrees(-DraftVecUtils.angle(ext1))
                        ),
                        "length": shootsize,
                    }
                )
            if not DraftVecUtils.isNull(ext2):
                expected.append(
                    {
                        "anchor": (float(proxy.p3.x), float(proxy.p3.y)),
                        "angle": self.normalizeAngleDegrees(
                            math.degrees(-DraftVecUtils.angle(ext2))
                        ),
                        "length": shootsize,
                    }
                )

        return expected

    def assertOvershootLinesMatch(self, actual_lines, expected_lines):
        """Match SVG overshoot lines by anchor, angle, and length."""
        self.assertEqual(len(actual_lines), len(expected_lines))
        remaining = list(actual_lines)
        for expected in expected_lines:
            match_index = None
            for index, actual in enumerate(remaining):
                same_anchor = (
                    abs(actual["anchor"][0] - expected["anchor"][0]) < 1e-6
                    and abs(actual["anchor"][1] - expected["anchor"][1]) < 1e-6
                )
                same_angle = self.angleDeltaDegrees(actual["angle"], expected["angle"]) < 1e-6
                same_length = abs(actual["length"] - expected["length"]) < 1e-6
                if same_anchor and same_angle and same_length:
                    match_index = index
                    break

            self.assertIsNotNone(match_index, msg=f"missing overshoot line {expected}")
            remaining.pop(match_index)

        self.assertEqual(remaining, [])

    def assertAngularSvgOvershoots(self, dimension, expected_lines=None, svg_text=None):
        """Assert the Draft SVG path exposes the angular overshoot lines."""
        if svg_text is None:
            svg_text = self.getAngularSvg(dimension)
        if expected_lines is None:
            expected_lines = self.getExpectedAngularSvgOvershoots(dimension)
        self.assertOvershootLinesMatch(self.extractOvershootLines(svg_text), expected_lines)

    def createTechDrawDraftView(self, doc, source):
        """Create a TechDraw DraftView for the provided angular dimension."""
        page = doc.addObject("TechDraw::DrawPage")
        template = doc.addObject("TechDraw::DrawSVGTemplate")
        template.Template = (
            App.getResourceDir() + "Mod/TechDraw/Templates/ISO/A3_Landscape_blank.svg"
        )
        page.Template = template
        view = doc.addObject("TechDraw::DrawViewDraft")
        view.Source = source
        view.Direction = Vector(0, 0, 1)
        page.addView(view)
        return view

    def assertAngularOvershoots(self, dimension, expected_state=None):
        """Assert that the angular dimension view provider drew both overshoot marks."""
        Gui.updateGui()
        state = self.getAngularOvershootState(dimension)
        self.assertEqual(state["dim_children"], 3)
        self.assertEqual(state["ext_children"], 3)
        self.assertEqual(state["dim_scale_start"], (1.0, 1.0, 1.0))
        self.assertEqual(state["dim_scale_end"], (1.0, 1.0, 1.0))
        self.assertEqual(state["ext_scale_start"], (2.0, 2.0, 2.0))
        self.assertEqual(state["ext_scale_end"], (2.0, 2.0, 2.0))
        self.assertEqual(state["dim_translation_start"], state["coord_start"])
        self.assertEqual(state["dim_translation_end"], state["coord_end"])
        self.assertEqual(state["ext_translation_start"], state["coord_start"])
        self.assertEqual(state["ext_translation_end"], state["coord_end"])

        if expected_state is not None:
            self.assertEqual(state["dim_children"], expected_state["dim_children"])
            self.assertEqual(state["ext_children"], expected_state["ext_children"])
            self.assertEqual(state["dim_scale_start"], expected_state["dim_scale_start"])
            self.assertEqual(state["dim_scale_end"], expected_state["dim_scale_end"])
            self.assertEqual(state["ext_scale_start"], expected_state["ext_scale_start"])
            self.assertEqual(state["ext_scale_end"], expected_state["ext_scale_end"])
            self.assertEqual(
                state["dim_translation_start"], expected_state["dim_translation_start"]
            )
            self.assertEqual(state["dim_translation_end"], expected_state["dim_translation_end"])
            self.assertEqual(
                state["ext_translation_start"], expected_state["ext_translation_start"]
            )
            self.assertEqual(state["ext_translation_end"], expected_state["ext_translation_end"])
            self.assertRotationAlmostEqual(
                state["dim_rotation_start"], expected_state["dim_rotation_start"]
            )
            self.assertRotationAlmostEqual(
                state["dim_rotation_end"], expected_state["dim_rotation_end"]
            )
            self.assertRotationAlmostEqual(
                state["ext_rotation_start"], expected_state["ext_rotation_start"]
            )
            self.assertRotationAlmostEqual(
                state["ext_rotation_end"], expected_state["ext_rotation_end"]
            )

    def test_angular_dimension_draws_overshoots_after_restore(self):
        """Angular dimensions keep both overshoot marks after saving and reopening a document."""
        original_name = self.doc.Name
        dimension = self.makeAngularDimension()
        vobj = dimension.ViewObject
        vobj.DimOvershoot = 1
        vobj.ExtOvershoot = 2

        self.doc.recompute()
        self.assertAngularOvershoots(dimension)
        expected_state = self.getAngularOvershootState(dimension)

        temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".FCStd")
        temp_file.close()
        reopened = None
        try:
            self.doc.saveAs(temp_file.name)
            self.doc = App.getDocument(original_name)
            reopened = App.openDocument(temp_file.name)
            Gui.updateGui()

            restored = reopened.getObject(dimension.Name)
            self.assertIsNotNone(restored)
            self.assertAngularOvershoots(restored, expected_state)

            reopened.recompute()
            self.assertAngularOvershoots(restored, expected_state)
        finally:
            if reopened is not None:
                App.closeDocument(reopened.Name)
            if original_name in App.listDocuments():
                self.doc = App.getDocument(original_name)
            os.unlink(temp_file.name)

    def test_angular_dimension_svg_overshoots_after_restore_and_in_techdraw(self):
        """Angular dimension overshoots survive restore in SVG and propagate into TechDraw DraftView."""
        original_name = self.doc.Name
        dimension = self.makeAngularDimension()
        vobj = dimension.ViewObject

        vobj.DimOvershoot = 0
        vobj.ExtOvershoot = 0
        self.doc.recompute()
        self.assertEqual(self.extractOvershootLines(self.getAngularSvg(dimension)), [])

        vobj.DimOvershoot = 1
        vobj.ExtOvershoot = 2
        self.doc.recompute()
        expected_lines = self.getExpectedAngularSvgOvershoots(dimension)
        self.assertAngularSvgOvershoots(dimension, expected_lines)

        temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".FCStd")
        temp_file.close()
        reopened = None
        try:
            self.doc.saveAs(temp_file.name)
            self.doc = App.getDocument(original_name)
            reopened = App.openDocument(temp_file.name)
            Gui.updateGui()

            restored = reopened.getObject(dimension.Name)
            self.assertIsNotNone(restored)
            self.assertAngularSvgOvershoots(restored, expected_lines)

            reopened.recompute()
            self.assertAngularSvgOvershoots(restored, expected_lines)

            view = self.createTechDrawDraftView(reopened, restored)
            reopened.recompute()
            Gui.updateGui()
            self.assertAngularSvgOvershoots(restored, expected_lines, view.Symbol)
        finally:
            if reopened is not None:
                App.closeDocument(reopened.Name)
            if original_name in App.listDocuments():
                self.doc = App.getDocument(original_name)
            os.unlink(temp_file.name)
