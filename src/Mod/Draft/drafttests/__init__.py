# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Modules that define classes used for unit testing the workbench.

These modules contain classes and functions that are called
by the unit test module that is defined in `Init.py` and `InitGui.py`.

The unit tests are placed in separate modules in order to test features
that do not require the graphical user interface (GUI), from those
that do require it.

The unit tests are based on the standard Python `unittest` module.
See this module and `unittest.TestCase` for more information
on how to write unit tests.

::

    class NewTestType(unittest.TestCase):
        def test_new_tool(self):
            pass
"""
## \defgroup drafttests drafttests
# \ingroup DRAFT
# \brief Modules that define classes used for unit testing the workbench.
