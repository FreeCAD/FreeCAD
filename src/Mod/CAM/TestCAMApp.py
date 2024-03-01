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

from Tests.TestPathProfile import TestPathProfile

from Tests.TestPathAdaptive import TestPathAdaptive
from Tests.TestPathCore import TestPathCore
from Tests.TestPathDepthParams import depthTestCases
from Tests.TestPathDressupDogbone import TestDressupDogbone
from Tests.TestPathDressupDogboneII import TestDressupDogboneII
from Tests.TestPathDressupHoldingTags import TestHoldingTags
from Tests.TestPathDrillable import TestPathDrillable
from Tests.TestPathDrillGenerator import TestPathDrillGenerator
from Tests.TestPathGeneratorDogboneII import TestGeneratorDogboneII
from Tests.TestPathGeom import TestPathGeom
from Tests.TestPathLanguage import TestPathLanguage
from Tests.TestPathOpDeburr import TestPathOpDeburr

# from Tests.TestPathHelix import TestPathHelix
from Tests.TestPathHelpers import TestPathHelpers
from Tests.TestPathHelixGenerator import TestPathHelixGenerator
from Tests.TestPathLog import TestPathLog
from Tests.TestPathOpUtil import TestPathOpUtil

# from Tests.TestPathPost import TestPathPost
from Tests.TestPathPost import TestPathPostUtils
from Tests.TestPathPost import TestBuildPostList
from Tests.TestPathPost import TestOutputNameSubstitution

from Tests.TestPathPreferences import TestPathPreferences
from Tests.TestPathProfile import TestPathProfile
from Tests.TestPathPropertyBag import TestPathPropertyBag
from Tests.TestPathRotationGenerator import TestPathRotationGenerator
from Tests.TestPathSetupSheet import TestPathSetupSheet
from Tests.TestPathStock import TestPathStock
from Tests.TestPathThreadMilling import TestPathThreadMilling
from Tests.TestPathThreadMillingGenerator import TestPathThreadMillingGenerator
from Tests.TestPathToolBit import TestPathToolBit
from Tests.TestPathToolChangeGenerator import TestPathToolChangeGenerator
from Tests.TestPathToolController import TestPathToolController
from Tests.TestPathUtil import TestPathUtil
from Tests.TestPathVcarve import TestPathVcarve
from Tests.TestPathVoronoi import TestPathVoronoi

from Tests.TestCentroidPost import TestCentroidPost
from Tests.TestGrblPost import TestGrblPost
from Tests.TestLinuxCNCPost import TestLinuxCNCPost
from Tests.TestMach3Mach4Post import TestMach3Mach4Post
from Tests.TestRefactoredCentroidPost import TestRefactoredCentroidPost
from Tests.TestRefactoredGrblPost import TestRefactoredGrblPost
from Tests.TestRefactoredLinuxCNCPost import TestRefactoredLinuxCNCPost
from Tests.TestRefactoredMach3Mach4Post import TestRefactoredMach3Mach4Post
from Tests.TestRefactoredTestPost import TestRefactoredTestPost
from Tests.TestRefactoredTestPostGCodes import TestRefactoredTestPostGCodes
from Tests.TestRefactoredTestPostMCodes import TestRefactoredTestPostMCodes

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
False if TestRefactoredTestPostGCodes.__name__ else True
False if TestRefactoredTestPostMCodes.__name__ else True
