# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides utility functions that wrap around the Console methods.

The Console module has long function names, so we define some shorthands
that are suitable for use in every workbench. These shorthands also include
a newline character at the end of the string, so it doesn't have to be
added manually.
"""
## @package messages
# \ingroup draftutils
# \brief Provides utility functions that wrap around the Console methods.

## \addtogroup draftutils
# @{
import FreeCAD as App


def _msg(text, end="\n"):
    """Write messages to the console including the line ending."""
    App.Console.PrintMessage(text + end)


def _wrn(text, end="\n"):
    """Write warnings to the console including the line ending."""
    App.Console.PrintWarning(text + end)


def _err(text, end="\n"):
    """Write errors to the console including the line ending."""
    App.Console.PrintError(text + end)


def _log(text, end="\n"):
    """Write messages to the log file including the line ending."""
    App.Console.PrintLog(text + end)

## @}
