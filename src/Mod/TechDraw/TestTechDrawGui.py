# **************************************************************************
#   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
# **************************************************************************

#these tests require QApplication in order to process events while waiting for 
#threads to complete
from TDTest.DrawViewSectionTest import DrawViewSectionTest  # noqa: F401
from TDTest.DrawViewPartTest import DrawViewPartTest  # noqa: F401
from TDTest.DrawViewDetailTest import DrawViewDetailTest  # noqa: F401
from TDTest.DrawViewDimensionTest import DrawViewDimensionTest  # noqa: F401

