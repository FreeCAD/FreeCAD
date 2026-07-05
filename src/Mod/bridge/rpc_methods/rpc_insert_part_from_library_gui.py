from __future__ import annotations

from rpc import *


@rpc_method
def _insert_part_from_library_gui(self, relative_path):
    try:
        _insert_part_from_library(relative_path)
        return True
    except Exception as e:
        return str(e)
