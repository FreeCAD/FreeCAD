# SPDX-License-Identifier: LGPL-2.1-or-later
"""Imperial-unit output scenarios."""

import Path
from CAMTests.post.post_harness import Scenario


def _xyz_move(job, op, tc):
    op.Path = Path.Path([Path.Command("G0 X10 Y20 Z30")])


scenarios = [
    Scenario(
        name="inches",
        variant="default",
        post_args="--no-header --inches --no-show-editor",
        build=_xyz_move,
    ),
    Scenario(
        name="inches",
        variant="p2",
        post_args="--no-header --inches --precision=2 --no-show-editor",
        build=_xyz_move,
        dialect_args={
            "centroid_legacy": "--no-header --inches --axis-precision=2 --no-show-editor",
        },
    ),
]
