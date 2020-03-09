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

## \addtogroup FEM
#  @{


class Proxy(object):

    BaseType = "Fem::ConstraintPython"

    def __init__(self, obj):
        # self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to this object

    # a few objects had this method in their class before the move to this base class
    # these objects will give a setAttr failed error on document loading without this method
    def __setstate__(self, state):
        if state:
            self.Type = state

##  @}
