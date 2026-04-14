# SPDX-License-Identifier: LGPL-2.1-or-later
"""Modal-command suppression scenarios.

`modal` suppresses a repeated command name (G0 G0 -> G0 ...). `axis_modal`
suppresses a repeated axis coordinate whose value didn't change.

Some posts ignore these flags; the golden captures actual behavior.
"""

import Path
from CAMTests.post.post_harness import Scenario


def _two_moves(job, op, tc):
    op.Path = Path.Path(
        [
            Path.Command("G0 X10 Y20 Z30"),
            Path.Command("G0 X10 Y30 Z30"),
        ]
    )


scenarios = [
    Scenario(
        name="modal",
        variant="default",
        post_args="--no-header --modal --no-show-editor",
        build=_two_moves,
        skip_dialects=["fanuc_legacy"],
    ),
    Scenario(
        name="axis_modal",
        variant="default",
        post_args="--no-header --axis-modal --no-show-editor",
        build=_two_moves,
        skip_dialects=["fanuc_legacy"],
    ),
]
