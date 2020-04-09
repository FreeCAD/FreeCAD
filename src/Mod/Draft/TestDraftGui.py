# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Unit tests for the Draft workbench, GUI only.

From the terminal, run the following:
FreeCAD -t TestDraftGui

From within FreeCAD, run the following:
import Test, TestDraftGui
Test.runTestsFromModule(TestDraftGui)

For the non-GUI tests see TestDraft.
"""

# ===========================================================================
# The unit tests can be run from the operating system terminal, or from
# within FreeCAD itself.
#
# The tests can be run using the full 'FreeCAD' executable
# or the console only 'FreeCADCmd' executable. In the latter case
# some functions cannot be tested as the view providers (visual properties)
# are not available.
#
# ===========================================================================
# In the following, first the command to run the test from the operating
# system terminal is listed, followed by the commands to run the test
# from the Python console within FreeCAD.
#
# ===========================================================================
# Run all Draft tests
# ----
# FreeCAD -t TestDraft
#
# >>> import Test, TestDraft
# >>> Test.runTestsFromModule(TestDraft)
#
# ===========================================================================
# Run tests from a specific module (all classes within this module)
# ----
# FreeCAD -t drafttests.test_creation
#
# >>> import Test, drafttests.test_creation
# >>> Test.runTestsFromModule(drafttests.test_creation)
#
# ===========================================================================
# Run tests from a specific class within a module
# ----
# FreeCAD -t drafttests.test_creation.DraftCreation
#
# >>> import Test, drafttests.test_creation
# >>> Test.runTestsFromClass(drafttests.test_creation.DraftCreation)
#
# ===========================================================================
# Run a specific unit test from a class within a module
# ----
# FreeCAD -t drafttests.test_creation.DraftCreation.test_line
#
# >>> import unittest
# >>> one_test = "drafttests.test_creation.DraftCreation.test_line"
# >>> all_tests = unittest.TestLoader().loadTestsFromName(one_test)
# >>> unittest.TextTestRunner().run(all_tests)

# ===========================================================================
# When the full test is run
#     FreeCAD -t TestDraft
#
# all classes that are found in this file are run.
#
# We import the classes from submodules. These classes contain
# the actual unit tests.
#
# The classes will be run in alphabetical order. So, to force
# a particular order of testing we import them with a name
# that follows a defined alphanumeric sequence.

# Import tests
from drafttests.test_import_gui import DraftGuiImport as DraftTestGui01
from drafttests.test_import_tools import DraftImportTools as DraftTestGui02
from drafttests.test_pivy import DraftPivy as DraftTestGui03

# Use the modules so that code checkers don't complain (flake8)
True if DraftTestGui01 else False
True if DraftTestGui02 else False
True if DraftTestGui03 else False
