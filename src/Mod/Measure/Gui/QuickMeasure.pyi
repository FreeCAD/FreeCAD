# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Namespace="MeasureGui",
    Delete=True,
)
class QuickMeasure(PyObjectBase):
    """
    Selection Observer for the QuickMeasure label.

    Author: Ondsel (development@ondsel.com)
    License: LGPL-2.1-or-later
    """

    def __init__(self) -> None: ...
