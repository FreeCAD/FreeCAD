# SPDX-License-Identifier: LGPL-2.1-or-later
"""Empty-path scenarios: preamble + postamble with no operations."""

import Path
from CAMTests.post.post_harness import Scenario


def _empty(job, op, tc):
    op.Path = Path.Path([])


def _empty_spindle_end(job, op, tc):
    op.Path = Path.Path([])


scenarios = [
    Scenario(
        name="empty_path",
        variant="no_header",
        post_args="--no-header --no-show-editor",
        build=_empty,
    ),
    Scenario(
        name="empty_path",
        variant="no_header_no_comments",
        post_args="--no-header --no-comments --no-show-editor",
        build=_empty,
    ),
    Scenario(
        name="empty_path_spindle_empty",
        variant="no_header",
        post_args="--no-header --no-show-editor --end-spindle-empty",
        build=_empty_spindle_end,
        skip_dialects=["generic", "mach3_mach4_legacy"],
    ),
    Scenario(
        name="empty_path_spindle_empty",
        variant="no_header_no_comments",
        post_args="--no-header --no-comments --no-show-editor --end-spindle-empty",
        build=_empty_spindle_end,
        skip_dialects=["generic", "mach3_mach4_legacy"],
    ),
]
