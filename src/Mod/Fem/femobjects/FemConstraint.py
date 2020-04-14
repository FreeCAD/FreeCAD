# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM base constraint object"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## @package _BaseObject
#  \ingroup FEM
#  \brief FreeCAD _Base Object for FEM workbench


class Proxy(object):

    BaseType = "Fem::ConstraintPython"

    def __init__(self, obj):
        # self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to this object

    # they are needed, see:
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=44021
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=44009
    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
