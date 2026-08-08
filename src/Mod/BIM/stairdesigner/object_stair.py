# SPDX-License-Identifier: LGPL-2.1-or-later

"""The assembled Stair document proxy."""

import FreeCAD

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_stair_base import StairBaseMixin

from .object_steps import StairStepsMixin

from .object_stringers import StairStringerMixin

from .object_handrails import StairHandrailMixin


class StairProxy(
    StairStepsMixin,
    StairStringerMixin,
    StairHandrailMixin,
    StairBaseMixin,
):
    """Parametric proxy for a complete Stair Designer stair."""

    Type = "StairDesigner"

    def __init__(self, obj):
        self._updating = True
        obj.Proxy = self
        self.Object = obj
        self.set_properties(obj)
        self._updating = False
