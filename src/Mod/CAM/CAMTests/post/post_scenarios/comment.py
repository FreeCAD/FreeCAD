# SPDX-License-Identifier: LGPL-2.1-or-later
"""Comment passthrough scenarios."""

import Path
from CAMTests.post.post_harness import Scenario


def _comment(job, op, tc):
    op.Path = Path.Path([Path.Command("(comment)")])


scenarios = [
    Scenario(
        name="comment",
        variant="default",
        post_args="--no-header --no-show-editor",
        build=_comment,
    ),
]
