# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Sauli Kiviranta
"""FreeCADCmd runner: exec tests_automated and tee prints to build/tests_gate.log."""
import os
import sys
import traceback

_here = os.path.dirname(os.path.abspath(__file__))
_root = os.path.dirname(os.path.dirname(_here))
LOG = os.path.join(_root, "build", "tests_gate.log")
_script = os.path.join(_here, "tests_automated.py")

os.makedirs(os.path.dirname(LOG), exist_ok=True)
_log = open(LOG, "w", encoding="utf-8")
_real_print = print


def _log_print(*args, **kwargs):
    kwargs.setdefault("flush", True)
    _real_print(*args, **kwargs)
    log_kwargs = {k: v for k, v in kwargs.items() if k != "file"}
    _real_print(*args, file=_log, **log_kwargs)


import builtins

builtins.print = _log_print

rc = 1
ns = {}
try:
    with open(_script, encoding="utf-8-sig") as f:
        exec(f.read(), ns)
    rc = ns["main"]()
except Exception:
    traceback.print_exc()
    traceback.print_exc(file=_log)
    rc = 1
finally:
    _log.write("EXIT_RC=%s\n" % rc)
    _log.flush()
    _log.close()

sys.exit(rc)
