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
'''Auto code generator for parameters in Preferences/TreeView
'''
import sys
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.abspath(__file__))), 'Tools'))
import params_utils

from params_utils import ParamBool, ParamInt, ParamString, ParamUInt,\
                         ParamFloat, ParamSpinBox, ParamColor, ParamHex

NameSpace = 'Gui'
ClassName = 'TreeParams'
ParamPath = 'User parameter:BaseApp/Preferences/TreeView'
ClassDoc = 'Convenient class to obtain tree view related parameters'
SourceFile = 'Tree.cpp'

Params = [
    ParamBool('SyncSelection', True, on_change=True),
    ParamBool('CheckBoxesSelection',False, on_change=True, title="Show item checkbox"),
    ParamBool('SyncView', True),
    ParamBool('PreSelection', True),
    ParamBool('SyncPlacement', False),
    ParamBool('RecordSelection', True),
    ParamInt('DocumentMode', 2, on_change=True),
    ParamInt('StatusTimeout', 100),
    ParamInt('SelectionTimeout', 100),
    ParamInt('PreSelectionTimeout', 500),
    ParamInt('PreSelectionDelay', 700),
    ParamInt('PreSelectionMinDelay', 200),
    ParamBool('RecomputeOnDrop', True),
    ParamBool('KeepRootOrder', True),
    ParamBool('TreeActiveAutoExpand', True),
    ParamUInt('TreeActiveColor',  0xe6e6ffff, on_change=True),
    ParamUInt('TreeEditColor',  0x929200ff, on_change=True),
    ParamUInt('SelectingGroupColor',  0x408081ff, on_change=True),
    ParamBool('TreeActiveBold', True, on_change=True),
    ParamBool('TreeActiveItalic', False, on_change=True),
    ParamBool('TreeActiveUnderlined', False, on_change=True),
    ParamBool('TreeActiveOverlined', False, on_change=True),
    ParamInt('Indentation', 0, on_change=True),
    ParamBool('LabelExpression', False),
    ParamInt('IconSize', 0, on_change=True),
    ParamInt('FontSize', 0, on_change=True),
    ParamInt('ItemSpacing', 0, on_change=True),
    ParamHex('ItemBackground', 0, on_change=True, title='Item background color', proxy=ParamColor(),
        doc = "Tree view item background. Only effective in overlay."),
    ParamInt('ItemBackgroundPadding', 10, on_change=True, title="Item background padding", proxy=ParamSpinBox(0, 100, 1),
        doc = "Tree view item background padding."),
    ParamBool('HideColumn', True, on_change=True, title="Hide extra column",
        doc = "Hide extra tree view column for item description."),
    ParamBool('HideScrollBar', True, title="Hide scroll bar",
        doc = "Hide tree view scroll bar in dock overlay."),
    ParamBool('HideHeaderView', True, title="Hide header",
        doc = "Hide tree view header view in dock overlay."),
    ParamBool('ResizableColumn', False, on_change=True, title="Resizable columns",
        doc = "Allow tree view columns to be manually resized."),
    ParamInt('ColumnSize1', 0),
    ParamInt('ColumnSize2', 0),
    ParamBool('TreeToolTipIcon', False, title='Show icon in tool tip'),
]

def declare_begin():
    params_utils.declare_begin(sys.modules[__name__])

def declare_end():
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])

params_utils.init_params(Params, NameSpace, ClassName, ParamPath)
