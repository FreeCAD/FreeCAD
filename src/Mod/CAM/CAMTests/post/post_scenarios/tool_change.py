# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tool-change scenarios."""

import Path
from CAMTests.post.post_harness import Scenario


def _tc(job, op, tc):
    op.Path = Path.Path([Path.Command("M6 T1"), Path.Command("M3 S3000")])


scenarios = [
    Scenario(
        name="tool_change",
        variant="default",
        post_args="--no-header --no-show-editor",
        build=_tc,
    ),
    Scenario(
        name="tool_change",
        variant="no_tlo",
        post_args="--no-header --no-tlo --no-show-editor",
        build=_tc,
    ),
]
