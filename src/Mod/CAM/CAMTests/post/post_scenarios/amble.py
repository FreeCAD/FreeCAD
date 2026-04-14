# SPDX-License-Identifier: LGPL-2.1-or-later
"""Custom preamble / postamble scenarios."""

import Path
from CAMTests.post.post_harness import Scenario


def _empty(job, op, tc):
    op.Path = Path.Path([])


scenarios = [
    Scenario(
        name="pre_amble",
        variant="custom",
        post_args="--no-header --no-comments --preamble='G18 G55' --no-show-editor",
        build=_empty,
    ),
    Scenario(
        name="post_amble",
        variant="custom",
        post_args="--no-header --no-comments --postamble='G0 Z50\nM30' --no-show-editor",
        build=_empty,
    ),
]
