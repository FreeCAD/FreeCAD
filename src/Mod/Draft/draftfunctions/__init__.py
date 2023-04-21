# ***************************************************************************
# *   (c) 2020 Carlo Pavan <carlopav@gmail.com>                             *
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
"""Modules that contain functions for use with scripted objects and commands.

These functions provide support for dealing with the custom objects
defined within the workbench.
The functions are meant to be used in the creation step of the objects,
by the functions in the `draftmake` package, but also by the graphical
GuiCommands in the `draftguitools` and `drafttaskpanels` packages.

These functions should deal with the internal shapes of the objects,
and their special properties. They should not be very generic;
if they are very generic then they are more appropriate to be included
in the `draftutils` package.

These functions, together with those defined in the `draftmake` package,
represent the public application programming interface (API)
of the Draft Workbench, and should be made available in the `Draft`
namespace by importing them in the `Draft` module.
"""
## \defgroup draftfunctions draftfunctions
# \ingroup DRAFT
# \brief Modules with functions for use with scripted objects and GuiCommands.
