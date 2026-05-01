# SPDX-License-Identifier: LGPL-2.1-or-later
"""Shared post-processor test scenarios.

Each submodule exports a `scenarios` list. `ALL_SCENARIOS` is the flat
registry consumed by the harness. Add new scenarios by creating a new
module and appending its list here.
"""

from . import (
    amble,
    comment,
    drilling,
    empty_path,
    inches,
    line_numbers,
    modal,
    precision,
    thread_tap,
    tool_change,
)

ALL_SCENARIOS = [
    *empty_path.scenarios,
    *precision.scenarios,
    *line_numbers.scenarios,
    *amble.scenarios,
    *inches.scenarios,
    *tool_change.scenarios,
    *thread_tap.scenarios,
    *comment.scenarios,
    *drilling.scenarios,
    *modal.scenarios,
]
