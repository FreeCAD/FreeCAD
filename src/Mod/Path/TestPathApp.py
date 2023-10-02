# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import TestApp

from PathTests.TestPathProfile import TestPathProfile

from PathTests.TestPathAdaptive import TestPathAdaptive
from PathTests.TestPathCore import TestPathCore
from PathTests.TestPathDepthParams import depthTestCases
from PathTests.TestPathDressupDogbone import TestDressupDogbone
from PathTests.TestPathDressupDogboneII import TestDressupDogboneII
from PathTests.TestPathDressupHoldingTags import TestHoldingTags
from PathTests.TestPathDrillable import TestPathDrillable
from PathTests.TestPathDrillGenerator import TestPathDrillGenerator
from PathTests.TestPathGeneratorDogboneII import TestGeneratorDogboneII
from PathTests.TestPathGeom import TestPathGeom
from PathTests.TestPathLanguage import TestPathLanguage
from PathTests.TestPathOpDeburr import TestPathOpDeburr

# from PathTests.TestPathHelix import TestPathHelix
from PathTests.TestPathHelpers import TestPathHelpers
from PathTests.TestPathHelixGenerator import TestPathHelixGenerator
from PathTests.TestPathLog import TestPathLog
from PathTests.TestPathOpUtil import TestPathOpUtil

# from PathTests.TestPathPost import TestPathPost
from PathTests.TestPathPost import TestPathPostUtils
from PathTests.TestPathPost import TestBuildPostList
from PathTests.TestPathPost import TestOutputNameSubstitution

from PathTests.TestPathPreferences import TestPathPreferences
from PathTests.TestPathProfile import TestPathProfile
from PathTests.TestPathPropertyBag import TestPathPropertyBag
from PathTests.TestPathRotationGenerator import TestPathRotationGenerator
from PathTests.TestPathSetupSheet import TestPathSetupSheet
from PathTests.TestPathStock import TestPathStock
from PathTests.TestPathThreadMilling import TestPathThreadMilling
from PathTests.TestPathThreadMillingGenerator import TestPathThreadMillingGenerator
from PathTests.TestPathToolBit import TestPathToolBit
from PathTests.TestPathToolChangeGenerator import TestPathToolChangeGenerator
from PathTests.TestPathToolController import TestPathToolController
from PathTests.TestPathUtil import TestPathUtil
from PathTests.TestPathVcarve import TestPathVcarve
from PathTests.TestPathVoronoi import TestPathVoronoi

from PathTests.TestCentroidPost import TestCentroidPost
from PathTests.TestGrblPost import TestGrblPost
from PathTests.TestLinuxCNCPost import TestLinuxCNCPost
from PathTests.TestMach3Mach4Post import TestMach3Mach4Post
from PathTests.TestRefactoredCentroidPost import TestRefactoredCentroidPost
from PathTests.TestRefactoredGrblPost import TestRefactoredGrblPost
from PathTests.TestRefactoredLinuxCNCPost import TestRefactoredLinuxCNCPost
from PathTests.TestRefactoredMach3Mach4Post import TestRefactoredMach3Mach4Post
from PathTests.TestRefactoredTestPost import TestRefactoredTestPost

# dummy usage to get flake8 and lgtm quiet
False if depthTestCases.__name__ else True
False if TestApp.__name__ else True
False if TestBuildPostList.__name__ else True
False if TestDressupDogbone.__name__ else True
False if TestDressupDogboneII.__name__ else True
False if TestGeneratorDogboneII.__name__ else True
False if TestHoldingTags.__name__ else True
False if TestPathLanguage.__name__ else True
False if TestOutputNameSubstitution.__name__ else True
False if TestPathAdaptive.__name__ else True
False if TestPathCore.__name__ else True
False if TestPathOpDeburr.__name__ else True
False if TestPathDrillable.__name__ else True
False if TestPathGeom.__name__ else True
False if TestPathHelpers.__name__ else True
# False if TestPathHelix.__name__ else True
False if TestPathLog.__name__ else True
False if TestPathOpUtil.__name__ else True
# False if TestPathPost.__name__ else True
False if TestPathPostUtils.__name__ else True
False if TestPathPreferences.__name__ else True
False if TestPathProfile.__name__ else True
False if TestPathPropertyBag.__name__ else True
False if TestPathRotationGenerator.__name__ else True
False if TestPathSetupSheet.__name__ else True
False if TestPathStock.__name__ else True
False if TestPathThreadMilling.__name__ else True
False if TestPathThreadMillingGenerator.__name__ else True
False if TestPathToolBit.__name__ else True
False if TestPathToolChangeGenerator.__name__ else True
False if TestPathToolController.__name__ else True
False if TestPathUtil.__name__ else True
False if TestPathVcarve.__name__ else True
False if TestPathVoronoi.__name__ else True
False if TestPathDrillGenerator.__name__ else True
False if TestPathHelixGenerator.__name__ else True

False if TestCentroidPost.__name__ else True
False if TestGrblPost.__name__ else True
False if TestLinuxCNCPost.__name__ else True
False if TestMach3Mach4Post.__name__ else True
False if TestRefactoredCentroidPost.__name__ else True
False if TestRefactoredGrblPost.__name__ else True
False if TestRefactoredLinuxCNCPost.__name__ else True
False if TestRefactoredMach3Mach4Post.__name__ else True
False if TestRefactoredTestPost.__name__ else True
