"""Provide the object code for Draft Array."""
## @package orthoarray
# \ingroup DRAFT
# \brief Provide the object code for Draft Array.

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

import FreeCAD as App
import Draft


def make_ortho_array(obj,
                     v_x=App.Vector(10, 0, 0),
                     v_y=App.Vector(0, 10, 0),
                     v_z=App.Vector(0, 0, 10),
                     n_x=2,
                     n_y=2,
                     n_z=1,
                     use_link=False):
    """Create an orthogonal array from the given object."""
    obj = Draft.makeArray(obj,
                          arg1=v_x, arg2=v_y, arg3=v_z,
                          arg4=n_x, arg5=n_y, arg6=n_z,
                          use_link=use_link)
    return obj


def make_ortho_array2(obj,
                      v_x=App.Vector(10, 0, 0),
                      v_y=App.Vector(0, 10, 0),
                      n_x=2,
                      n_y=2,
                      use_link=False):
    """Create a 2D orthogonal array from the given object."""
    obj = Draft.makeArray(obj,
                          arg1=v_x, arg2=v_y,
                          arg3=n_x, arg4=n_y,
                          use_link=use_link)
    return obj
