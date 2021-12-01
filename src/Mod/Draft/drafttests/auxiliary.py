# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Auxiliary functions for the unit tests of the Draft Workbench."""
## @package auxiliary
# \ingroup drafttests
# \brief Auxiliary functions for the unit tests of the Draft Workbench.

## \addtogroup drafttests
# @{
import traceback

from draftutils.messages import _msg


def draw_header():
    """Draw a header for the tests."""
    _msg("")
    _msg(78*"-")


def import_test(module):
    """Try importing a module."""
    _msg("  Try importing '{}'".format(module))
    try:
        imported = __import__("{}".format(module))
    except ImportError as exc:
        imported = False
        _msg("  {}".format(exc))
        _msg(traceback.format_exc())
    return imported


def no_gui(module):
    """Print a message that there is no user interface."""
    _msg("  #-----------------------------------------------------#\n"
         "  #    No GUI; cannot test for '{}'\n"
         "  #-----------------------------------------------------#\n"
         "  Automatic PASS".format(module))


def no_test():
    """Print a message that the test is not currently implemented."""
    _msg("  #-----------------------------------------------------#\n"
         "  #    This test is not implemented currently\n"
         "  #-----------------------------------------------------#\n"
         "  Automatic PASS")


def fake_function(p1=None, p2=None, p3=None, p4=None, p5=None):
    """Print a message for a test that doesn't actually exist."""
    _msg("  Arguments to placeholder function")
    _msg("  p1={0}; p2={1}".format(p1, p2))
    _msg("  p3={0}; p4={1}".format(p3, p4))
    _msg("  p5={}".format(p5))
    no_test()
    return True

## @}
