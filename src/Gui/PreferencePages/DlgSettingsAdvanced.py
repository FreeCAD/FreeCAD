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
'''Auto code generator for preference page of Display/UI
'''
import cog, sys
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(
    path.dirname(path.dirname(path.abspath(__file__)))), 'Tools'))
import params_utils
from params_utils import auto_comment

sys.path.append(path.join(path.dirname(
    path.dirname(path.dirname(path.abspath(__file__)))), 'Gui'))
import OverlayParams, TreeParams

Title = 'Advanced'
NameSpace = 'Gui'
ClassName = 'DlgSettingsAdvanced'
ClassDoc = 'Preference dialog for various advanced UI settings'
UserInit = 'init();'

_OverlayParams = { param.name : param for param in OverlayParams.Params }
_TreeParams = { param.name : param for param in TreeParams.Params }

ParamGroup = (
    ('Tree view', [_TreeParams[name] for name in (
        'ItemBackgroundPadding',
        'FontSize',
    )]),

    ('Overlay', [_OverlayParams[name] for name in (
        'DockOverlayWheelDelay',
        'DockOverlayAlphaRadius',
        'DockOverlayCheckNaviCube',
        'DockOverlayHintTriggerSize',
        'DockOverlayHintSize',
        'DockOverlayHintLeftOffset',
        'DockOverlayHintLeftLength',
        'DockOverlayHintRightOffset',
        'DockOverlayHintRightLength',
        'DockOverlayHintTopOffset',
        'DockOverlayHintTopLength',
        'DockOverlayHintBottomOffset',
        'DockOverlayHintBottomLength',
        'DockOverlayHintDelay',
        'DockOverlaySplitterHandleTimeout',
        'DockOverlayActivateOnHover',
        'DockOverlayDelay',
        'DockOverlayAnimationDuration',
        'DockOverlayAnimationCurve',
    )]),
)

def declare_begin():
    params_utils.preference_dialog_declare_begin(sys.modules[__name__])

def declare_end():
    params_utils.preference_dialog_declare_end(sys.modules[__name__])

def define():
    params_utils.preference_dialog_define(sys.modules[__name__])
