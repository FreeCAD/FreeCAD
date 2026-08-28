# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python conversion metadata for Spreadsheet property classes."""

PROPERTY_CPP_NAMESPACE = "Spreadsheet"

import Spreadsheet

class PropertySheet:
    def get(self) -> Spreadsheet.PropertySheet: ...
    def set(self, value: Spreadsheet.PropertySheet) -> None: ...

class PropertyColumnWidths:
    READ_ONLY = True

    def get(self) -> Spreadsheet.PropertyColumnWidths: ...

class PropertyRowHeights:
    READ_ONLY = True

    def get(self) -> Spreadsheet.PropertyRowHeights: ...
