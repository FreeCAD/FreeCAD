# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

# flake8: noqa import

import TestApp

# --- sanity ---
from CAMTests.sanity.TestCAMSanity import TestCAMSanity

# --- machine ---
from CAMTests.machine.TestMachine import (
    TestMachineDataclass,
    TestMachineFactory,
    TestToolhead,
)

# --- core ---
from CAMTests.core.TestPathCommandAnnotations import TestPathCommandAnnotations
from CAMTests.core.TestPathCore import TestPathCore
from CAMTests.core.TestPathDepthParams import depthTestCases
from CAMTests.core.TestPathGeom import TestPathGeom
from CAMTests.core.TestPathHelpers import TestPathHelpers
from CAMTests.core.TestPathLanguage import TestPathLanguage
from CAMTests.core.TestPathLog import TestPathLog
from CAMTests.core.TestPathPreferences import TestPathPreferences
from CAMTests.core.TestPathPropertyBag import TestPathPropertyBag
from CAMTests.core.TestPathSetupSheet import TestPathSetupSheet
from CAMTests.core.TestPathStock import TestPathStock
from CAMTests.core.TestPathUtil import TestPathUtil
from CAMTests.core.TestPathVoronoi import TestPathVoronoi
from CAMTests.core.TestTSPSolver import TestTSPSolver

# --- generators ---
from CAMTests.generator.TestDrillCycleExpander import TestDrillCycleExpander
from CAMTests.generator.TestLinkingGenerator import TestGetLinkingMoves
from CAMTests.generator.TestPathDrillGenerator import TestPathDrillGenerator
from CAMTests.generator.TestPathFacingGenerator import TestPathFacingGenerator
from CAMTests.generator.TestPathGeneratorDogboneII import TestGeneratorDogboneII
from CAMTests.generator.TestPathHelixGenerator import TestPathHelixGenerator
from CAMTests.generator.TestPathRotationGenerator import TestPathRotationGenerator
from CAMTests.generator.TestPathSpiralGenerator import TestPathSpiralGenerator
from CAMTests.generator.TestPathTapGenerator import TestPathTapGenerator
from CAMTests.generator.TestPathThreadMillingGenerator import TestPathThreadMillingGenerator
from CAMTests.generator.TestPathToolChangeGenerator import TestPathToolChangeGenerator

# --- operations ---
from CAMTests.ops.TestPathAdaptive import TestPathAdaptive
from CAMTests.ops.TestPathDrillable import TestPathDrillable
from CAMTests.ops.TestPathHelix import TestPathHelix
from CAMTests.ops.TestPathOpDeburr import TestPathOpDeburr
from CAMTests.ops.TestPathOpUtil import TestPathOpUtil
from CAMTests.ops.TestPathPocket import TestPathPocket
from CAMTests.ops.TestPathProfile import TestPathProfile
from CAMTests.ops.TestPathThreadMilling import TestPathThreadMilling
from CAMTests.ops.TestPathVcarve import TestPathVcarve

# --- dressups ---
from CAMTests.dressup.TestPathDressupDogboneII import TestDressupDogboneII
from CAMTests.dressup.TestPathDressupHoldingTags import TestHoldingTags

# --- tools ---
from CAMTests.tool.TestPathToolAsset import TestPathToolAsset
from CAMTests.tool.TestPathToolAssetCache import (
    TestPathToolAssetCache,
    TestPathToolAssetCacheIntegration,
)
from CAMTests.tool.TestPathToolAssetManager import TestPathToolAssetManager
from CAMTests.tool.TestPathToolAssetStore import TestPathToolFileStore, TestPathToolMemoryStore
from CAMTests.tool.TestPathToolAssetUri import TestPathToolAssetUri
from CAMTests.tool.TestPathToolBit import TestPathToolBit
from CAMTests.tool.TestPathToolBitSerializer import (
    TestCamoticsToolBitSerializer,
    TestFCTBSerializer,
    TestYamlToolBitSerializer,
)
from CAMTests.tool.TestPathToolController import TestPathToolController
from CAMTests.tool.TestPathToolLibrary import TestPathToolLibrary
from CAMTests.tool.TestPathToolLibrarySerializer import (
    TestCamoticsLibrarySerializer,
    TestLinuxCNCLibrarySerializer,
)
from CAMTests.tool.TestPathToolShapeClasses import TestPathToolShapeClasses
from CAMTests.tool.TestPathToolShapeDoc import TestPathToolShapeDoc
from CAMTests.tool.TestPathToolShapeIcon import (
    TestToolBitShapeIconBase,
    TestToolBitShapeSvgIcon,
    TestToolBitShapePngIcon,
)
from CAMTests.tool.TestToolBitRecomputeState import TestToolBitRecomputeState

# --- post-processors ---
from CAMTests.post.TestPostCore import (
    TestPathPostUtils,
    TestBuildPostList,
    TestJobPropertyOverrides,
)
from CAMTests.post.TestPostProcessor import (
    TestPostProcessorFactory,
    TestResolvingPostProcessorName,
    TestHeaderBuilder,
    TestConfigurationBundle,
)
from CAMTests.post.TestPostOutput import (
    TestFileNameGenerator,
    TestExport2Integration,
)
from CAMTests.post.TestPostToolProcessing import TestToolLengthOffset, TestToolProcessing
from CAMTests.post.TestPostGCodes import TestPostGCodes
from CAMTests.post.TestPostMCodes import TestPostMCodes
from CAMTests.post.TestGcodeProcessingUtils import (
    TestInsertLineNumbers,
    TestSuppressRedundantAxesWords,
    TestFilterInefficientMoves,
    TestNumberGenerator,
)
from CAMTests.post.TestDressupPost import TestDressupPost

from CAMTests.post.TestGenericPost import TestGenericPost
from CAMTests.post.TestGenericPlasma import TestGenericPlasma
from CAMTests.post.TestLinuxCNCPost import TestLinuxCNCPost
from CAMTests.post.TestMarlinPost import TestMarlinPost
from CAMTests.post.TestDxfPost import TestDxfPost
from CAMTests.post.TestFanucPost import TestFanucPost
from CAMTests.post.TestGrblPost import TestGrblPost
from CAMTests.post.TestMassoG3Post import TestMassoG3Post
from CAMTests.post.TestCentroidPost import TestCentroidPost
from CAMTests.post.TestMach3Mach4Post import TestMach3Mach4Post
from CAMTests.post.TestTestPost import TestTestPost
from CAMTests.post.TestCentroidLegacyPost import TestCentroidLegacyPost
from CAMTests.post.TestMach3Mach4LegacyPost import TestMach3Mach4LegacyPost
from CAMTests.post.TestSnapmakerPost import TestSnapmakerPost
from CAMTests.post.TestSVGPost import TestSVGPost
