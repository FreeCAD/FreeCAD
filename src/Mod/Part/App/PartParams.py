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
'''Auto code generator for Part related parameters
'''
import sys
import cog
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.dirname(path.dirname(path.abspath(__file__))))), 'Tools'))
import params_utils

from params_utils import Property, ParamBool, ParamInt, ParamString, ParamUInt, ParamFloat, ParamColor

NameSpace = 'Part'
ClassName = 'PartParams'
ParamPath = 'User parameter:BaseApp/Preferences/Mod/Part'
ClassDoc = 'Convenient class to obtain Part/PartDesign related parameters'

# import ../Gui/PartGuiParams.py
sys.path.append(path.join(path.dirname(path.dirname(path.abspath(__file__))),
'Gui'))
import PartGuiParams
_PartGuiParams = { param.name : param for param in PartGuiParams.Params }

# import the following parameters in PartGuiParams.py so that we don't need to
# maintain the same definition in two places. These parameters need to be in Gui
# namespace because we need realtime changes in view providers in response to
# changes in these parameters
_MinimumDeviation = _PartGuiParams['MinimumDeviation']
_MinimumDeviation.on_change = False
_MeshDeviation = _PartGuiParams['MeshDeviation']
_MeshDeviation.on_change = False
_MeshAngularDeflection = _PartGuiParams['MeshAngularDeflection']
_MeshAngularDeflection.on_change = False
_MinimumAngularDeflection = _PartGuiParams['MinimumAngularDeflection']
_MinimumAngularDeflection.on_change = False

Params = [
    ParamBool("ShapePropertyCopy", False),
    ParamBool("DisableShapeCache", False),
    ParamInt("CommandOverride", 2),
    ParamInt("EnableWrapFeature", 2),
    ParamBool("CopySubShape", False),
    ParamBool("UseBrepToolsOuterWire", True),
    ParamBool("UseBaseObjectName", False),
    ParamBool("AutoGroupSolids", False),
    ParamBool("SingleSolid", False),
    ParamBool("UsePipeForExtrusionDraft", False),
    ParamBool("LinearizeExtrusionDraft", True),
    ParamBool("AutoCorrectLink", False),
    ParamBool("RefineModel", False),
    ParamBool("AuxGroupUniqueLabel", False),
    ParamBool("SplitEllipsoid", True),
    ParamInt("ParallelRunThreshold", 100),
    ParamBool("AutoValidateShape", False),
    ParamBool("FixShape", False),
    ParamUInt("LoftMaxDegree", 5),
    _MinimumDeviation,
    _MeshDeviation,
    _MeshAngularDeflection,
    _MinimumAngularDeflection,
]

def declare():
    params_utils.declare_begin(sys.modules[__name__])
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])

PropertyGroup = 'ShapeContent'
Properties = [
    Property('ShapeContents',
             'App::PropertyLinkList',
             'Stores the expanded sub shape content objects',
             PropertyGroup),
    Property('ShapeContentSuppressed',
             'App::PropertyBool',
             'Suppress this sub shape content',
             PropertyGroup),
    Property('ShapeContentReplacement',
             'App::PropertyLinkHidden',
             'Refers to a shape replacement',
             PropertyGroup),
    Property('ShapeContentReplacementSuppressed',
             'App::PropertyBool',
             'Suppress shape content replacement',
             PropertyGroup),
    Property('ShapeContentDetached',
             'App::PropertyBool',
             'If detached, than the shape content will not be auto removed and parent shape is removed',
             PropertyGroup),
    Property('_ShapeContentOwner',
             'App::PropertyLinkHidden',
             'Refers to the shape owner',
             PropertyGroup,
             prop_flags='App::Prop_Hidden'),
]

def declare_properties():
    params_utils.declare_properties(Properties)

def define_properties():
    params_utils.define_properties(Properties, 'Part::Feature')
