# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM constraint centrif document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

## @package constraint_centrif
#  \ingroup FEM
#  \brief constraint centrif object

from . import base_fempythonobject


class ConstraintCentrif(base_fempythonobject.BaseFemPythonObject):
    """
    The ConstraintCentrif object
    """

    Type = "Fem::ConstraintCentrif"

    def __init__(self, obj):
        super(ConstraintCentrif, self).__init__(obj)

        obj.addProperty(
            "App::PropertyFrequency",
            "RotationFrequency",
            "Constraint CENTRIF",
            "set rotation frequency f<sub>rot"
        )

        obj.addProperty(
            "App::PropertyLinkSubList",
            "RotationAxis",
            "Constraint CENTRIF",
            "set line as axis of rotation"
        )
