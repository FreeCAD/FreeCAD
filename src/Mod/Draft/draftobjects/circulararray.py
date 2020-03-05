"""This module provides the object code for Draft CircularArray.
"""
## @package circulararray
# \ingroup DRAFT
# \brief This module provides the object code for Draft CircularArray.

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

import FreeCAD as App
import Draft


def make_circular_array(obj,
                        r_distance=100, tan_distance=100,
                        axis=App.Vector(0, 0, 1), center=App.Vector(0, 0, 0),
                        number=2, symmetry=1,
                        use_link=False):
    """Create a circular array from the given object.
    """
    obj = Draft.makeArray(obj,
                          arg1=r_distance, arg2=tan_distance,
                          arg3=axis, arg4=center, arg5=number, arg6=symmetry,
                          use_link=use_link)
    return obj
