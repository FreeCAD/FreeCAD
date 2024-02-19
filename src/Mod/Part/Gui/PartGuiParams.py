# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com>*
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
'''Auto code generator for PartGui related parameters
'''
import sys
import cog
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.dirname(path.dirname(path.abspath(__file__))))), 'Tools'))
import params_utils

from params_utils import Property, ParamBool, ParamInt, ParamHex, ParamUInt, ParamFloat, ParamColor

NameSpace = 'PartGui'
ClassName = 'PartParams'
ParamPath = 'User parameter:BaseApp/Preferences/Mod/Part'
ClassDoc = 'Convenient class to obtain Part/PartDesign visual related parameters'

Params = [
    ParamBool("NormalsFromUVNodes", True),
    ParamBool("TwoSideRendering", True),
    ParamFloat("MinimumDeviation", 0.05, on_change=True),
    ParamFloat("MeshDeviation", 0.2, on_change=True),
    ParamFloat("MeshAngularDeflection", 28.65, on_change=True),
    ParamFloat("MinimumAngularDeflection", 5.0, on_change=True),
    ParamBool("OverrideTessellation", False, on_change=True),
    ParamBool("MapFaceColor", True),
    ParamBool("MapLineColor", False),
    ParamBool("MapPointColor", False),
    ParamBool("MapTransparency", False),
    ParamBool("AutoGridScale", False),
    ParamHex("PreviewAddColor", 0x64ffff30, proxy=ParamColor()),
    ParamHex("PreviewSubColor", 0xff646430, proxy=ParamColor()),
    ParamHex("PreviewDressColor", 0xff64ff30, proxy=ParamColor()),
    ParamHex("PreviewIntersectColor", 0x6464ff30, proxy=ParamColor()),
    ParamBool("PreviewOnEdit", True),
    ParamBool("PreviewWithTransparency", True),
    ParamBool("EditOnTop", False),
    ParamInt("EditRecomputeWait", 300),
    ParamBool("AdjustCameraForNewFeature", True),
    ParamHex("DefaultDatumColor", 0xFFD70099, proxy=ParamColor()),
    ParamBool("RespectSystemDPI", False, on_change=True),
    ParamInt("SelectionPickThreshold", 1000),
    ParamInt("SelectionPickThreshold2", 500),
    ParamBool("SelectionPickRTree", False),
]

def declare():
    params_utils.declare_begin(sys.modules[__name__])
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])
