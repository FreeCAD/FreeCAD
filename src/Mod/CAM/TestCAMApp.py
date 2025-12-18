# SPDX-License-Identifier: LGPL-2.1-or-later

# flake8: noqa import
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
from CAMTests.TestLinkingGenerator import TestGetLinkingMoves
from CAMTests.TestPathProfile import TestPathProfile

from CAMTests.TestPathAdaptive import TestPathAdaptive
from CAMTests.TestPathCommandAnnotations import TestPathCommandAnnotations
from CAMTests.TestPathCore import TestPathCore
from CAMTests.TestPathDepthParams import depthTestCases
from CAMTests.TestPathDressupDogbone import TestDressupDogbone
from CAMTests.TestPathDressupDogboneII import TestDressupDogboneII
from CAMTests.TestPathDressupHoldingTags import TestHoldingTags
from CAMTests.TestPathDrillable import TestPathDrillable
from CAMTests.TestPathDrillGenerator import TestPathDrillGenerator
from CAMTests.TestPathFacingGenerator import TestPathFacingGenerator
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
from CAMTests.TestPathToolAsset import TestPathToolAsset
from CAMTests.TestPathToolAssetCache import (
    TestPathToolAssetCache,
    TestPathToolAssetCacheIntegration,
)
from CAMTests.TestPathToolAssetManager import TestPathToolAssetManager
from CAMTests.TestPathToolAssetStore import TestPathToolFileStore, TestPathToolMemoryStore
from CAMTests.TestPathToolAssetUri import TestPathToolAssetUri
from CAMTests.TestPathToolBit import TestPathToolBit
from CAMTests.TestPathToolShapeClasses import TestPathToolShapeClasses
from CAMTests.TestPathToolShapeDoc import TestPathToolShapeDoc
from CAMTests.TestPathToolShapeIcon import (
    TestToolBitShapeIconBase,
    TestToolBitShapeSvgIcon,
    TestToolBitShapePngIcon,
)
from CAMTests.TestPathToolBitSerializer import (
    TestCamoticsToolBitSerializer,
    TestFCTBSerializer,
    TestYamlToolBitSerializer,
)
from CAMTests.TestPathToolLibrary import TestPathToolLibrary
from CAMTests.TestPathToolLibrarySerializer import (
    TestCamoticsLibrarySerializer,
    TestLinuxCNCLibrarySerializer,
)
from CAMTests.TestPathToolChangeGenerator import TestPathToolChangeGenerator
from CAMTests.TestPathToolController import TestPathToolController
from CAMTests.TestPathToolMachine import TestPathToolMachine
from CAMTests.TestPathUtil import TestPathUtil
from CAMTests.TestPathVcarve import TestPathVcarve
from CAMTests.TestPathVoronoi import TestPathVoronoi

from CAMTests.TestGenericPost import TestGenericPost
from CAMTests.TestLinuxCNCPost import TestLinuxCNCPost
from CAMTests.TestFanucPost import TestFanucPost
from CAMTests.TestGrblPost import TestGrblPost
from CAMTests.TestMassoG3Post import TestMassoG3Post
from CAMTests.TestCentroidPost import TestCentroidPost
from CAMTests.TestMach3Mach4Post import TestMach3Mach4Post
from CAMTests.TestTestPost import TestTestPost
from CAMTests.TestPostGCodes import TestPostGCodes
from CAMTests.TestPostMCodes import TestPostMCodes
from CAMTests.TestDressupPost import TestDressupPost

from CAMTests.TestLinuxCNCLegacyPost import TestLinuxCNCLegacyPost
from CAMTests.TestGrblLegacyPost import TestGrblLegacyPost
from CAMTests.TestCentroidLegacyPost import TestCentroidLegacyPost
from CAMTests.TestMach3Mach4LegacyPost import TestMach3Mach4LegacyPost

from CAMTests.TestSnapmakerPost import TestSnapmakerPost
from CAMTests.TestTSPSolver import TestTSPSolver
