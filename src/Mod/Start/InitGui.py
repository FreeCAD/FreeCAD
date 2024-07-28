# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2024 The FreeCAD Project Association AISBL               *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import StartGui  # Not unused, import has a side-effect of creating the "Start_Start" command
import StartMigrator

migrator = StartMigrator.StartMigrator2024()
migrator.run_migration()

import FreeCAD
import FreeCADGui


def apply_dark_theme():
    # Path to the QSS file
    qss_file = FreeCAD.getHomePath() + "data/themes/DarkTheme.qss"

    try:
        # Read the QSS file
        with open(qss_file, "r") as file:
            qss = file.read()

        # Apply the QSS
        FreeCADGui.getMainWindow().setStyleSheet(qss)
        print("Dark theme applied successfully.")
    except Exception as e:
        print(f"Failed to apply dark theme: {e}")


apply_dark_theme()
