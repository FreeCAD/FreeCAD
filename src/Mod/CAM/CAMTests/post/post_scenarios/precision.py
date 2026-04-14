# SPDX-License-Identifier: LGPL-2.1-or-later
"""Numeric precision scenarios."""

import Path
from CAMTests.post.post_harness import Scenario


def _xyz_move(job, op, tc):
    op.Path = Path.Path([Path.Command("G0 X10 Y20 Z30")])


scenarios = [
    Scenario(
        name="precision",
        variant="default",
        post_args="--no-header --no-show-editor",
        build=_xyz_move,
    ),
    Scenario(
        name="precision",
        variant="p2",
        post_args="--no-header --precision=2 --no-show-editor",
        build=_xyz_move,
        dialect_args={
            "centroid_legacy": "--no-header --axis-precision=2 --no-show-editor",
        },
    ),
]
