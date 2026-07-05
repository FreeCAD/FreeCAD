from __future__ import annotations

from rpc import *


@rpc_method
def get_parts_list(self):
    return _get_parts_list()
