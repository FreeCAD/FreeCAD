# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

import json
import pathlib
import tempfile

import CAMTests.PathTestUtils as PathTestUtils
from Machine.models.machine import Machine, Toolhead, ToolheadType
from Machine.models.validate import (
    Severity,
    validate_machine,
    validate_file,
    validate_paths,
    format_findings,
    main,
    _normalize_path,
)


def _codes(findings, severity=None):
    """Finding codes, optionally filtered by severity."""
    return [f.code for f in findings if severity is None or f.severity is severity]


class TestMachineValidate(PathTestUtils.PathTestBase):
    """Validation of machine (.fcm) definitions."""

    def _valid_machine(self):
        """A machine that passes every check, as the starting point for each case.

        create_3axis_config() defines no toolhead, so one is added here --
        otherwise every test would also carry a no-toolheads warning.
        """
        machine = Machine.create_3axis_config()
        machine.name = "Validator Test Machine"
        machine.toolheads = [Toolhead(name="Spindle", toolhead_type=ToolheadType.ROTARY)]
        return machine

    # ------------------------------------------------------------------
    # validate_machine
    # ------------------------------------------------------------------

    def test000_clean_machine_has_no_errors(self):
        """A stock 3-axis machine produces no errors."""
        findings = validate_machine(self._valid_machine(), check_postprocessor=False)
        self.assertEqual(_codes(findings, Severity.ERROR), [])

    def test010_empty_name_is_an_error(self):
        """A machine with no name cannot be resolved by MachineFactory."""
        machine = self._valid_machine()
        machine.name = "   "
        self.assertIn("no-name", _codes(validate_machine(machine, check_postprocessor=False)))

    def test020_no_axes_is_an_error(self):
        machine = self._valid_machine()
        machine.linear_axes = {}
        machine.rotary_axes = {}
        self.assertIn("no-axes", _codes(validate_machine(machine, check_postprocessor=False)))

    def test030_inverted_axis_limits_are_an_error(self):
        """min_limit >= max_limit makes the envelope meaningless."""
        machine = self._valid_machine()
        machine.linear_axes["X"].min_limit = 500
        machine.linear_axes["X"].max_limit = 100
        findings = validate_machine(machine, check_postprocessor=False)
        self.assertIn("axis-limits", _codes(findings, Severity.ERROR))

    def test040_no_toolheads_is_a_warning(self):
        machine = self._valid_machine()
        machine.toolheads = []
        findings = validate_machine(machine, check_postprocessor=False)
        self.assertIn("no-toolheads", _codes(findings, Severity.WARNING))

    def test050_broken_kinematic_chain_is_an_error(self):
        """Errors from validate_kinematic_chain() are surfaced as findings."""
        machine = self._valid_machine()
        machine.linear_axes["X"].parent = "NoSuchAxis"
        findings = validate_machine(machine, check_postprocessor=False)
        self.assertIn("kinematics", _codes(findings, Severity.ERROR))

    def test060_missing_postprocessor_is_an_error(self):
        machine = self._valid_machine()
        machine.postprocessor_file_name = ""
        self.assertIn("no-postprocessor", _codes(validate_machine(machine)))

        machine.postprocessor_file_name = "no_such_postprocessor_exists"
        self.assertIn("postprocessor-missing", _codes(validate_machine(machine)))

    def test070_unknown_postprocessor_property_is_a_warning(self):
        """A misspelled property key is accepted by the model but never read."""
        machine = self._valid_machine()
        machine.postprocessor_file_name = "generic"
        machine.postprocessor_properties = {"preamble": "G90", "not_a_real_property": 1}
        findings = validate_machine(machine)
        self.assertIn("unknown-properties", _codes(findings, Severity.WARNING))
        self.assertIn("not_a_real_property", str(findings))

    # ------------------------------------------------------------------
    # dropped-data detection
    # ------------------------------------------------------------------

    def test100_keys_the_loader_ignores_are_reported(self):
        """Data present in the file but never read must not pass silently."""
        machine = self._valid_machine()
        raw = machine.to_dict()
        raw["machine"]["axes"]["X"]["min"] = 0
        raw["machine"]["axes"]["X"]["max"] = 500
        raw["invented_section"] = {"nested": True}

        findings = validate_machine(machine, raw=raw, check_postprocessor=False)
        self.assertIn("ignored-keys", _codes(findings, Severity.WARNING))
        message = str(findings)
        self.assertIn("machine.axes.X.max", message)
        self.assertIn("invented_section", message)

    def test110_round_trip_of_current_format_is_clean(self):
        """A file written by the current model reports no dropped data."""
        machine = self._valid_machine()
        raw = machine.to_dict()
        findings = validate_machine(machine, raw=raw, check_postprocessor=False)
        self.assertEqual(_codes(findings, Severity.WARNING), [])

    def test120_legacy_spellings_are_not_reported_as_dropped(self):
        """Keys the loader still reads under an older name are not warnings.

        Guards the rewrite table against becoming a source of false positives,
        which is what would make the warning tier ignorable.
        """
        machine = self._valid_machine()
        raw = machine.to_dict()

        # machine.spindles is still read as machine.toolheads
        raw["machine"]["spindles"] = raw["machine"].pop("toolheads")
        # output_header is read from the output level as well as from header
        raw["output"]["output_header"] = raw["output"]["header"].pop("output_header")
        # joints are accepted as an object as well as a pair of arrays
        for axis in raw["machine"]["axes"].values():
            origin, vector = axis["joint"]
            axis["joint"] = {"origin": origin, "axis": vector}

        reloaded = Machine.from_dict(raw)
        findings = validate_machine(reloaded, raw=raw, check_postprocessor=False)
        self.assertEqual(
            _codes(findings, Severity.WARNING),
            [],
            f"legacy spellings reported as dropped: {findings}",
        )

    def test130_normalize_path_chains_rewrites(self):
        """One rewrite can expose another; all of them must be applied."""
        self.assertEqual(
            _normalize_path("machine.spindles.spindle_wait"),
            "machine.toolheads.toolhead_wait",
        )
        self.assertEqual(_normalize_path("machine.spindles"), "machine.toolheads")
        self.assertEqual(_normalize_path("machine.axes.X.joint.origin"), "machine.axes.X.joint")
        self.assertEqual(_normalize_path("machine.name"), "machine.name")

    # ------------------------------------------------------------------
    # files and trees
    # ------------------------------------------------------------------

    def test200_invalid_json_is_a_single_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "broken.fcm"
            path.write_text("{ not json")
            findings = validate_file(path)
            self.assertEqual(_codes(findings), ["invalid-json"])

    def test210_unloadable_machine_is_a_single_error(self):
        """An unknown enum value stops validation rather than cascading."""
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "bad_toolhead.fcm"
            machine = self._valid_machine()
            raw = machine.to_dict()
            raw["machine"]["toolheads"][0]["toolhead_type"] = "not_a_toolhead_type"
            path.write_text(json.dumps(raw))
            findings = validate_file(path)
            self.assertEqual(_codes(findings), ["load-failed"])

    def test220_duplicate_names_across_files_are_an_error(self):
        """MachineFactory resolves by display name, so collisions matter."""
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            machine = self._valid_machine()
            for filename in ("one.fcm", "two.fcm"):
                (root / filename).write_text(json.dumps(machine.to_dict()))

            results = validate_paths([str(root)], check_postprocessor=False)
            self.assertEqual(len(results), 2)
            for findings in results.values():
                self.assertIn("duplicate-name", _codes(findings, Severity.ERROR))

    def test230_distinct_names_do_not_collide(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            for index, filename in enumerate(("one.fcm", "two.fcm")):
                machine = self._valid_machine()
                machine.name = f"Machine {index}"
                (root / filename).write_text(json.dumps(machine.to_dict()))

            results = validate_paths([str(root)], check_postprocessor=False)
            for findings in results.values():
                self.assertNotIn("duplicate-name", _codes(findings))

    def test240_directories_are_searched_recursively(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            (root / "vendor").mkdir()
            machine = self._valid_machine()
            (root / "vendor" / "nested.fcm").write_text(json.dumps(machine.to_dict()))

            results = validate_paths([str(root)], check_postprocessor=False)
            self.assertEqual(len(results), 1)

    # ------------------------------------------------------------------
    # reporting and CLI
    # ------------------------------------------------------------------

    def test300_format_hides_info_unless_verbose(self):
        machine = self._valid_machine()
        raw = machine.to_dict()
        del raw["processing"]  # makes the model look newer than the file
        results = {"m.fcm": validate_machine(machine, raw=raw, check_postprocessor=False)}

        self.assertNotIn("newer-model", format_findings(results))
        self.assertIn("newer-model", format_findings(results, verbose=True))

    def test310_cli_exit_codes(self):
        """0 when clean, 1 on error, 1 on warning only with --strict, 2 when empty."""
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            self.assertEqual(main([str(root)]), 2)

            machine = self._valid_machine()
            (root / "good.fcm").write_text(json.dumps(machine.to_dict()))
            self.assertEqual(main([str(root), "--no-postprocessor-check"]), 0)

            warned = machine.to_dict()
            warned["machine"]["name"] = "Warned Machine"
            warned["a_key_nobody_reads"] = True
            (root / "warn.fcm").write_text(json.dumps(warned))
            self.assertEqual(main([str(root), "--no-postprocessor-check"]), 0)
            self.assertEqual(main([str(root), "--no-postprocessor-check", "--strict"]), 1)

            (root / "broken.fcm").write_text("{ not json")
            self.assertEqual(main([str(root), "--no-postprocessor-check"]), 1)
