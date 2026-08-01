# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Callable

def QT_TRANSLATE_NOOP(context: str, source_text: str, /) -> str: ...

class QTimer:
    @staticmethod
    def singleShot(msec: int, slot: Callable[[], object], /) -> None: ...
