# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2002 Jürgen Riegel
# SPDX-FileCopyrightText: 2025 Frank Martínez
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

# FreeCAD init module - Tests
#
# Gathering all the information to start FreeCAD.
# This is the third of four init scripts:
# +------+------------------+-----------------------------+
# | This | Script           | Runs                        |
# +------+------------------+-----------------------------+
# |      | CMakeVariables   | always                      |
# |      | FreeCADInit      | always                      |
# | >>>> | FreeCADTest      | only if test and not Gui    |
# |      | FreeCADGuiInit   | only if Gui is up           |
# +------+------------------+-----------------------------+

# Testing the function of the base system and run
# (if existing) the test function of the modules

import FreeCAD
import typing

if typing.TYPE_CHECKING:
    from __main__ import Log

Log("FreeCAD test running...\n\n")
Log("Init: starting App::FreeCADTest.py\n")
Log("░░░▀█▀░█▀█░▀█▀░▀█▀░░░▀█▀░█▀▀░█▀▀░▀█▀░█▀▀░░░\n")
Log("░░░░█░░█░█░░█░░░█░░░░░█░░█▀░░▀▀█░░█░░▀▀█░░░\n")
Log("░░░▀▀▀░▀░▀░▀▀▀░░▀░░░░░▀░░▀▀▀░▀▀▀░░▀░░▀▀▀░░░\n")

import sys
import TestApp

testResult = TestApp.RunConfiguredTextTest()

Log("FreeCAD test done\n")

sys.exit(0 if testResult.wasSuccessful() else 1)
