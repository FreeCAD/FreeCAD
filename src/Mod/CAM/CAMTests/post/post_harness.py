# SPDX-License-Identifier: LGPL-2.1-or-later
"""
Parameterized post-processor test harness.

Each dialect test file is reduced to a single call:

    TestFanucPost = make_post_test_class("fanuc_legacy", "fanuc_legacy")

Scenarios live under `post_scenarios/` and declare how to build the job/op
state plus the post-processor arguments. Expected output is stored per
dialect under `post_golden/<dialect>/<scenario>__<variant>.gcode`.

Regenerating goldens:
    CAM_REGEN_GOLDEN=1 ./bin/FreeCADCmd -t TestCAMApp.TestFanucPost

The regen path writes whatever the post currently emits. Review the
resulting files in the diff before committing.
"""

import os
import pathlib
import unittest
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional

from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory

GOLDEN_ROOT = pathlib.Path(__file__).parent / "post_golden"
REGEN = os.environ.get("CAM_REGEN_GOLDEN") == "1"


@dataclass
class Scenario:
    """One combination of state + args that produces a golden G-code file.

    build(job, op, tool_controller) mutates the job/op state in place
    (typically by setting op.Path). It must not set PostProcessorArgs —
    that is handled by the harness so the same build can drive multiple
    arg variants.
    """

    name: str
    variant: str
    post_args: str
    build: Callable
    skip_dialects: List[str] = field(default_factory=list)
    # Per-dialect post_args overrides. Use when the same intent requires
    # a different flag in another dialect (e.g. --precision vs --axis-precision).
    dialect_args: Dict[str, str] = field(default_factory=dict)

    @property
    def test_method_name(self) -> str:
        return f"test_{self.name}__{self.variant}"

    def golden_path(self, dialect: str) -> pathlib.Path:
        return GOLDEN_ROOT / dialect / f"{self.name}__{self.variant}.gcode"

    def args_for(self, dialect: str) -> str:
        return self.dialect_args.get(dialect, self.post_args)


def _run_scenario(test_case, scenario: Scenario):
    job, op, tc = test_case.job, test_case.op, test_case.tc
    scenario.build(job, op, tc)
    job.PostProcessorArgs = scenario.args_for(test_case.dialect_id)
    test_case.post.reinitialize()
    gcode = test_case.post.export()[0][1]

    golden = scenario.golden_path(test_case.dialect_id)
    if REGEN:
        golden.parent.mkdir(parents=True, exist_ok=True)
        golden.write_text(gcode)
        return
    if not golden.exists():
        raise AssertionError(
            f"Missing golden file: {golden}\n" f"Run with CAM_REGEN_GOLDEN=1 to create it."
        )
    expected = golden.read_text()
    test_case.assertEqual(gcode, expected, f"Output differs from {golden}")


def make_post_test_class(
    dialect_id: str,
    post_name: str,
    scenarios: Optional[List[Scenario]] = None,
    base: type = PathTestUtils.PathTestBase,
):
    """Factory: build a unittest.TestCase subclass wired to one dialect.

    dialect_id:   short name used as the golden subdirectory (e.g. "fanuc_legacy")
    post_name:    argument passed to PostProcessorFactory.get_post_processor
    scenarios:    list of Scenario; defaults to the full shared registry
    """

    if scenarios is None:
        from CAMTests.post.post_scenarios import ALL_SCENARIOS

        scenarios = ALL_SCENARIOS

    class _HarnessPostTest(base):
        dialect_id = None  # set below

        def setUp(self):
            self.job, self.op, self.tc = PostTestMocks.create_default_job_with_operation()
            self.post = PostProcessorFactory.get_post_processor(self.job, post_name)
            self.maxDiff = None
            self.post.reinitialize()

    _HarnessPostTest.dialect_id = dialect_id
    _HarnessPostTest.__name__ = f"TestPost_{dialect_id}"
    _HarnessPostTest.__qualname__ = _HarnessPostTest.__name__

    for scenario in scenarios:
        if dialect_id in scenario.skip_dialects:
            continue

        def _make(sc):
            def _test(self):
                _run_scenario(self, sc)

            _test.__name__ = sc.test_method_name
            _test.__doc__ = f"{sc.name} / {sc.variant}"
            return _test

        setattr(_HarnessPostTest, scenario.test_method_name, _make(scenario))

    return _HarnessPostTest
