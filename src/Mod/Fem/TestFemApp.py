# ***************************************************************************
# *   Copyright (c) 2018 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
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

# Unit test for the FEM module
# to get the right order import as is used
from femtest.app.test_femimport import TestFemImport as FemTest01
from femtest.app.test_common import TestFemCommon as FemTest02
from femtest.app.test_object import TestObjectCreate as FemTest03
from femtest.app.test_object import TestObjectType as FemTest04
from femtest.app.test_open import TestObjectOpen as FemTest05
from femtest.app.test_material import TestMaterialUnits as FemTest06
from femtest.app.test_mesh import TestMeshCommon as FemTest07
from femtest.app.test_mesh import TestMeshEleTetra10 as FemTest08
from femtest.app.test_result import TestResult as FemTest09
from femtest.app.test_ccxtools import TestCcxTools as FemTest10
from femtest.app.test_solverframework import TestSolverFrameWork as FemTest11

from femtest.app.test_tools import TestCreateObject
from femtest.app.test_tools import TestFindAnalysisOfMember
from femtest.app.test_tools import TestGetMember
from femtest.app.test_tools import TestIsDerivedFrom
from femtest.app.test_tools import TestGetSingleMember
from femtest.app.test_tools import TestGetSeveralMember
from femtest.app.test_tools import TestGetMeshToSolve
from femtest.app.test_tools import TestTypeOfObj
from femtest.app.test_tools import TestIsOfType
from femtest.app.test_tools import TestGetBoundBoxOfAllDocumentShapes
from femtest.app.test_tools import TestGetRefshapeType

# dummy usage to get flake8 and lgtm quiet
False if FemTest01.__name__ else True
False if FemTest02.__name__ else True
False if FemTest03.__name__ else True
False if FemTest04.__name__ else True
False if FemTest05.__name__ else True
False if FemTest06.__name__ else True
False if FemTest07.__name__ else True
False if FemTest08.__name__ else True
False if FemTest09.__name__ else True
False if FemTest10.__name__ else True
False if FemTest11.__name__ else True
