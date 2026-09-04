# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

from __future__ import annotations

"""
This module is the Materials module.
"""


def addMaterialObserver(observer: object) -> None:
    """
    Register an observer object for material lifecycle callbacks.

    Observers may implement any of:
    slotCreatedMaterial(material), slotChangedMaterial(material),
    slotDeletedMaterial(material).
    """
    ...


def removeMaterialObserver(observer: object) -> None:
    """
    Remove an observer previously registered with addMaterialObserver().
    """
    ...
