# SPDX-License-Identifier: LGPL-2.1-or-later
"""Rigid-tapping scenarios (G84 cycle)."""

import Path
from CAMTests.post.post_harness import Scenario


def _tap(job, op, tc):
    tc.Tool.ShapeType = "tap"
    op.Path = Path.Path(
        [
            Path.Command("G0 X10 Y10"),
            Path.Command("G84 X10 Y10 Z-10 R20 F1 P1 Q1"),
        ]
    )


scenarios = [
    Scenario(
        name="thread_tap",
        variant="default",
        post_args="--no-header --no-show-editor",
        build=_tap,
        skip_dialects=["centroid_legacy", "generic", "mach3_mach4_legacy"],
    ),
]
