# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/Measure/App/MeasureBase.h",
    Namespace="Measure",
    Constructor=True,
)
class MeasureBase(DocumentObject):
    """
    User documentation here

    Author: David Friedli(hlorus) (david@friedli-be.ch)
    License: LGPL-2.1-or-later
    """
