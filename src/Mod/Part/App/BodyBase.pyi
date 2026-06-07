# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from PartFeature import PartFeature

@export(
    Twin="BodyBase",
)
class BodyBase(PartFeature):
    """
    Base class of all Body objects

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    ...
