# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
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

""" Utilities for working with licenses. Based on SPDX info downloaded from
https://github.com/spdx/license-list-data and stored as part of the FreeCAD repo, loaded into a Qt
resource. """

import json

# Get whatever version of PySide we can
try:
    import PySide  # Use the FreeCAD wrapper
except ImportError:
    try:
        import PySide6  # Outside FreeCAD, try Qt6 first

        PySide = PySide6
    except ImportError:
        import PySide2  # Fall back to Qt5 (if this fails, Python will kill this module's import)

        PySide = PySide2

from PySide import QtCore


class SPDXLicenseManager:
    """A class that loads a list of licenses from an internal Qt resource and provides access to
    some information about those licenses."""

    def __init__(self):
        self.license_data = {}
        self._load_license_data()

    def _load_license_data(self):
        qf = QtCore.QFile(f":/licenses/spdx.json")
        if qf.exists():
            qf.open(QtCore.QIODevice.ReadOnly)
            byte_data = qf.readAll()
            qf.close()

            string_data = str(byte_data, encoding="utf-8")
            raw_license_data = json.loads(string_data)

            self._process_raw_spdx_json(raw_license_data)

    def _process_raw_spdx_json(self, raw_license_data: dict):
        """The raw JSON data is a list of licenses, with the ID as an element of the contained
        data members. More useful for our purposes is a dictionary with the SPDX IDs as the keys
        and the remaining data as the values."""
        for entry in raw_license_data["licenses"]:
            self.license_data[entry["licenseId"]] = entry

    def is_osi_approved(self, spdx_id: str) -> bool:
        """Check to see if the license is OSI-approved, according to the SPDX database. Returns
        False if the license is not in the database, or is not marked as "isOsiApproved"."""
        if spdx_id not in self.license_data:
            return False
        return self.license_data[spdx_id]["isOsiApproved"]

    def is_fsf_libre(self, spdx_id: str) -> bool:
        """Check to see if the license is FSF Free/Libre, according to the SPDX database. Returns
        False if the license is not in the database, or is not marked as "isFsfLibre"."""
        if spdx_id not in self.license_data:
            return False
        return self.license_data[spdx_id]["isFsfLibre"]

    def name(self, spdx_id: str) -> str:
        if spdx_id not in self.license_data:
            return ""
        return self.license_data[spdx_id]["name"]

    def url(self, spdx_id: str) -> str:
        if spdx_id not in self.license_data:
            return ""
        return self.license_data[spdx_id]["reference"]

    def details_json_url(self, spdx_id: str):
        """The "detailsUrl" entry in the SPDX database, which is a link to a JSON file containing
        the details of the license. As of SPDX v3 the fields are:
          * isDeprecatedLicenseId
          * isFsfLibre
          * licenseText
          * standardLicenseHeaderTemplate
          * standardLicenseTemplate
          * name
          * licenseId
          * standardLicenseHeader
          * crossRef
          * seeAlso
          * isOsiApproved
          * licenseTextHtml
          * standardLicenseHeaderHtml"""
        if spdx_id not in self.license_data:
            return ""
        return self.license_data[spdx_id]["detailsUrl"]


_LICENSE_MANAGER = None  # Internal use only, see get_license_manager()


def get_license_manager() -> SPDXLicenseManager:
    """Get the license manager. Prevents multiple re-loads of the license list by keeping a
    single copy of the manager."""
    global _LICENSE_MANAGER
    if _LICENSE_MANAGER is None:
        _LICENSE_MANAGER = SPDXLicenseManager()
    return _LICENSE_MANAGER
