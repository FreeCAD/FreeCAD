# SPDX-License-Identifier: LGPL-2.1-or-later
"""Line-number output scenarios."""

import Path
from CAMTests.post.post_harness import Scenario


def _xyz_move(job, op, tc):
    op.Path = Path.Path([Path.Command("G0 X10 Y20 Z30")])


scenarios = [
    Scenario(
        name="line_numbers",
        variant="default",
        post_args="--no-header --line-numbers --no-show-editor",
        build=_xyz_move,
    ),
]
