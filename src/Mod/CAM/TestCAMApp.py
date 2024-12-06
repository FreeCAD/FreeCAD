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

from CAMTests.TestCAMSanity import TestCAMSanity
from CAMTests.TestPathProfile import TestPathProfile

from CAMTests.TestPathAdaptive import TestPathAdaptive
from CAMTests.TestPathCore import TestPathCore
from CAMTests.TestPathDepthParams import depthTestCases
from CAMTests.TestPathDressupDogbone import TestDressupDogbone
from CAMTests.TestPathDressupDogboneII import TestDressupDogboneII
from CAMTests.TestPathDressupHoldingTags import TestHoldingTags
from CAMTests.TestPathDrillable import TestPathDrillable
from CAMTests.TestPathDrillGenerator import TestPathDrillGenerator
from CAMTests.TestPathGeneratorDogboneII import TestGeneratorDogboneII
from CAMTests.TestPathGeom import TestPathGeom
from CAMTests.TestPathLanguage import TestPathLanguage
from CAMTests.TestPathOpDeburr import TestPathOpDeburr
from CAMTests.TestPathHelpers import TestPathHelpers
from CAMTests.TestPathHelix import TestPathHelix
from CAMTests.TestPathHelixGenerator import TestPathHelixGenerator
from CAMTests.TestPathLog import TestPathLog
from CAMTests.TestPathOpUtil import TestPathOpUtil

# from CAMTests.TestPathPost import TestPathPost
from CAMTests.TestPathPost import TestPathPostUtils
from CAMTests.TestPathPost import TestBuildPostList

# from CAMTests.TestPathPost import TestOutputNameSubstitution
from CAMTests.TestPathPost import TestPostProcessorFactory
from CAMTests.TestPathPost import TestResolvingPostProcessorName
from CAMTests.TestPathPost import TestFileNameGenerator

from CAMTests.TestPathPreferences import TestPathPreferences
from CAMTests.TestPathProfile import TestPathProfile
from CAMTests.TestPathPropertyBag import TestPathPropertyBag
from CAMTests.TestPathRotationGenerator import TestPathRotationGenerator
from CAMTests.TestPathSetupSheet import TestPathSetupSheet
from CAMTests.TestPathStock import TestPathStock
from CAMTests.TestPathTapGenerator import TestPathTapGenerator
from CAMTests.TestPathThreadMilling import TestPathThreadMilling
from CAMTests.TestPathThreadMillingGenerator import TestPathThreadMillingGenerator
from CAMTests.TestPathToolBit import TestPathToolBit
from CAMTests.TestPathToolChangeGenerator import TestPathToolChangeGenerator
from CAMTests.TestPathToolController import TestPathToolController
from CAMTests.TestPathUtil import TestPathUtil
from CAMTests.TestPathVcarve import TestPathVcarve
from CAMTests.TestPathVoronoi import TestPathVoronoi

from CAMTests.TestCentroidPost import TestCentroidPost
from CAMTests.TestGrblPost import TestGrblPost
from CAMTests.TestLinuxCNCPost import TestLinuxCNCPost
from CAMTests.TestMach3Mach4Post import TestMach3Mach4Post
from CAMTests.TestRefactoredCentroidPost import TestRefactoredCentroidPost
from CAMTests.TestRefactoredGrblPost import TestRefactoredGrblPost
from CAMTests.TestRefactoredLinuxCNCPost import TestRefactoredLinuxCNCPost
from CAMTests.TestRefactoredMach3Mach4Post import TestRefactoredMach3Mach4Post
from CAMTests.TestRefactoredTestPost import TestRefactoredTestPost
from CAMTests.TestRefactoredTestPostGCodes import TestRefactoredTestPostGCodes
from CAMTests.TestRefactoredTestPostMCodes import TestRefactoredTestPostMCodes

# dummy usage to get flake8 and lgtm quiet
False if TestCAMSanity.__name__ else True
False if depthTestCases.__name__ else True
False if TestApp.__name__ else True
False if TestBuildPostList.__name__ else True
False if TestDressupDogbone.__name__ else True
False if TestDressupDogboneII.__name__ else True
False if TestFileNameGenerator.__name__ else True
False if TestGeneratorDogboneII.__name__ else True
False if TestHoldingTags.__name__ else True
False if TestPathLanguage.__name__ else True
# False if TestOutputNameSubstitution.__name__ else True
False if TestPathAdaptive.__name__ else True
False if TestPathCore.__name__ else True
False if TestPathOpDeburr.__name__ else True
False if TestPathDrillable.__name__ else True
False if TestPathGeom.__name__ else True
False if TestPathHelpers.__name__ else True
False if TestPathHelix.__name__ else True
False if TestPathLog.__name__ else True
False if TestPathOpUtil.__name__ else True
# False if TestPathPost.__name__ else True
False if TestPostProcessorFactory.__name__ else True
False if TestResolvingPostProcessorName.__name__ else True
False if TestPathPostUtils.__name__ else True
False if TestPathPreferences.__name__ else True
False if TestPathProfile.__name__ else True
False if TestPathPropertyBag.__name__ else True
False if TestPathRotationGenerator.__name__ else True
False if TestPathSetupSheet.__name__ else True
False if TestPathStock.__name__ else True
False if TestPathTapGenerator.__name__ else True
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
False if TestRefactoredTestPostGCodes.__name__ else True
False if TestRefactoredTestPostMCodes.__name__ else True
