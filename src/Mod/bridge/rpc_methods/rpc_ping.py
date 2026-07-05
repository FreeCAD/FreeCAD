from __future__ import annotations

from rpc import *


@rpc_method
def ping(self):
    return True
