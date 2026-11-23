# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Zheng Lei
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

"""Auto code generator for preference page of Display/UI"""

import cog, sys
from os import path

# import Tools/params_utils.py
sys.path.append(
    path.join(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))), "Tools")
)
import params_utils
from params_utils import auto_comment

sys.path.append(path.join(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))), "Gui"))
import OverlayParams

Title = "Advanced"
NameSpace = "Gui"
ClassName = "DlgSettingsAdvanced"
ClassDoc = "Preference dialog for various advanced UI settings"
UserInit = "init();"

_OverlayParams = {param.name: param for param in OverlayParams.Params}

ParamGroup = (
    (
        "Overlay",
        [
            _OverlayParams[name]
            for name in (
                "DockOverlayWheelDelay",
                "DockOverlayAlphaRadius",
                "DockOverlayCheckNaviCube",
                "DockOverlayHintTriggerSize",
                "DockOverlayHintSize",
                "DockOverlayHintLeftOffset",
                "DockOverlayHintLeftLength",
                "DockOverlayHintRightOffset",
                "DockOverlayHintRightLength",
                "DockOverlayHintTopOffset",
                "DockOverlayHintTopLength",
                "DockOverlayHintBottomOffset",
                "DockOverlayHintBottomLength",
                "DockOverlayHintDelay",
                "DockOverlaySplitterHandleTimeout",
                "DockOverlayActivateOnHover",
                "DockOverlayDelay",
                "DockOverlayAnimationDuration",
                "DockOverlayAnimationCurve",
            )
        ],
    ),
)


def declare_begin():
    params_utils.preference_dialog_declare_begin(sys.modules[__name__])


def declare_end():
    params_utils.preference_dialog_declare_end(sys.modules[__name__])


def define():
    params_utils.preference_dialog_define(sys.modules[__name__])
