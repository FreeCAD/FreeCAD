# ***************************************************************************
# *   Copyright (c) 2018 - FreeCAD Developers                               *
# *   Author: Przemo Firszt <przemo@firszt.eu>                              *
# *   Author: Bernd Hahnebach <bernd@bimstatik.org>                         *
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
# ***************************************************************************/


# Unit test for the FEM module
# the order should be as follows:
# common-, object-, mesh-, inout-, ccxtools-, solverframworktests
from femtest.testcommon import TestFemCommon
from femtest.testobject import TestObjectCreate
from femtest.testobject import TestObjectType
#from femtest.testmaterial import TestMaterialUnits
#from femtest.testmesh import TestMeshCommon
#from femtest.testmesh import TestMeshEleTetra10
#from femtest.testresult import TestResult
#from femtest.testccxtools import TestCcxTools
#from femtest.testsolverframework import TestSolverFrameWork


# For more information on how to run a specific test class or a test method see
# file src/Mod/Test/__init__ and forum https://forum.freecadweb.org/viewtopic.php?f=10&t=22190#p175546
