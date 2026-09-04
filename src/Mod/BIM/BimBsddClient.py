# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************
"""Compatibility shim for the bSDD network client module."""

from BimBsdd import (
    BSDD_API_BASE_URL,
    BSDD_API_URL_KEY,
    BSDD_IFC_DICTIONARY_URI,
    BSDD_INACTIVE_KEY,
    BSDD_PREFERENCES_PATH,
    BSDD_PREVIEW_KEY,
    BSDD_TEST_KEY,
    BsddNetworkClient,
    BsddSettings,
    get_bsdd_network_client,
)

__all__ = [
    "BSDD_API_BASE_URL",
    "BSDD_API_URL_KEY",
    "BSDD_IFC_DICTIONARY_URI",
    "BSDD_INACTIVE_KEY",
    "BSDD_PREFERENCES_PATH",
    "BSDD_PREVIEW_KEY",
    "BSDD_TEST_KEY",
    "BsddNetworkClient",
    "BsddSettings",
    "get_bsdd_network_client",
]
