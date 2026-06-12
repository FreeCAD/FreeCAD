# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2001,2002 Juergen Riegel <juergen.riegel@web.de>
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

# ******************************************************************************
# *                                                                            *
# *   FreeCAD is free software: you can redistribute it and/or modify          *
# *   it under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the     *
# *   License, or (at your option) any later version.                          *
# *                                                                            *
# *   FreeCAD is distributed in the hope that it will be useful, but           *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Lesser General Public License for more details.                      *
# *                                                                            *
# *   You should have received a copy of the GNU Lesser General Public         *
# *   License along with FreeCAD.  If not, see                                *
# *   <https://www.gnu.org/licenses/>.                                         *
# *                                                                            *
# ******************************************************************************

# FreeCAD init script of the test module

# Base system tests
FreeCAD.__unit_test__ += [
    "BaseTests",
    "UnitTests",
    "Document",
    "Metadata",
    "StringHasher",
    "UnicodeTests",
    "TestPythonSyntax",
]
