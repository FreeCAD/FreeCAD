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

# from PathTests.TestPathPost import PathPostTestCases
from PathTests.TestPathAdaptive import TestPathAdaptive
from PathTests.TestPathCore import TestPathCore
from PathTests.TestPathDeburr import TestPathDeburr
from PathTests.TestPathDepthParams import depthTestCases
from PathTests.TestPathDressupDogbone import TestDressupDogbone
from PathTests.TestPathDressupHoldingTags import TestHoldingTags
from PathTests.TestPathGeom import TestPathGeom
from PathTests.TestPathHelix import TestPathHelix
from PathTests.TestPathLog import TestPathLog
from PathTests.TestPathOpTools import TestPathOpTools
from PathTests.TestPathPreferences import TestPathPreferences
from PathTests.TestPathPropertyBag import TestPathPropertyBag
from PathTests.TestPathSetupSheet import TestPathSetupSheet
from PathTests.TestPathStock import TestPathStock
from PathTests.TestPathThreadMilling import TestPathThreadMilling
from PathTests.TestPathTool import TestPathTool
from PathTests.TestPathToolBit import TestPathToolBit
from PathTests.TestPathToolController import TestPathToolController
from PathTests.TestPathTooltable import TestPathTooltable
from PathTests.TestPathUtil import TestPathUtil
from PathTests.TestPathVcarve import TestPathVcarve
from PathTests.TestPathVoronoi import TestPathVoronoi

# dummy usage to get flake8 and lgtm quiet
False if depthTestCases.__name__ else True
False if TestApp.__name__ else True
False if TestDressupDogbone.__name__ else True
False if TestHoldingTags.__name__ else True
False if TestPathAdaptive.__name__ else True
False if TestPathCore.__name__ else True
False if TestPathDeburr.__name__ else True
False if TestPathGeom.__name__ else True
False if TestPathHelix.__name__ else True
False if TestPathLog.__name__ else True
False if TestPathOpTools.__name__ else True
False if TestPathPreferences.__name__ else True
False if TestPathPropertyBag.__name__ else True
False if TestPathSetupSheet.__name__ else True
False if TestPathStock.__name__ else True
False if TestPathThreadMilling.__name__ else True
False if TestPathTool.__name__ else True
False if TestPathToolBit.__name__ else True
False if TestPathToolController.__name__ else True
False if TestPathTooltable.__name__ else True
False if TestPathUtil.__name__ else True
False if TestPathVcarve.__name__ else True
False if TestPathVoronoi.__name__ else True
