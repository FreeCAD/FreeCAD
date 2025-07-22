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
'''Auto code generator for overlay widgets related parameters in Preferences/View
'''
import cog
import inspect, sys
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.abspath(__file__))), 'Tools'))
import params_utils

from params_utils import ParamBool, ParamInt, ParamString, ParamUInt, ParamHex, \
                         ParamFloat, ParamProxy, ParamLinePattern, ParamFile, \
                         ParamComboBox, ParamColor, ParamSpinBox, auto_comment

NameSpace = 'Gui'
ClassName = 'OverlayParams'
ParamPath = 'User parameter:BaseApp/Preferences/View'
ClassDoc = 'Convenient class to obtain overlay widgets related parameters'

AnimationCurveTypes = (
    "Linear",
    "InQuad",
    "OutQuad",
    "InOutQuad",
    "OutInQuad",
    "InCubic",
    "OutCubic",
    "InOutCubic",
    "OutInCubic",
    "InQuart",
    "OutQuart",
    "InOutQuart",
    "OutInQuart",
    "InQuint",
    "OutQuint",
    "InOutQuint",
    "OutInQuint",
    "InSine",
    "OutSine",
    "InOutSine",
    "OutInSine",
    "InExpo",
    "OutExpo",
    "InOutExpo",
    "OutInExpo",
    "InCirc",
    "OutCirc",
    "InOutCirc",
    "OutInCirc",
    "InElastic",
    "OutElastic",
    "InOutElastic",
    "OutInElastic",
    "InBack",
    "OutBack",
    "InOutBack",
    "OutInBack",
    "InBounce",
    "OutBounce",
    "InOutBounce",
    "OutInBounce",
)

class ParamAnimationCurve(ParamProxy):
    WidgetType = 'Gui::PrefComboBox'

    def widget_setter(self, _param):
        return None

    def init_widget(self, param, row, group_name):
        super().init_widget(param, row, group_name)
        cog.out(f'''
    {auto_comment()}
    for (const auto &item : OverlayParams::AnimationCurveTypes)
        {param.widget_name}->addItem(item);''')
        cog.out(f'''
    {param.widget_name}->setCurrentIndex({param.namespace}::{param.class_name}::default{param.name}());''')

Params = [
    ParamBool('DockOverlayAutoView', True, on_change=True, title="Auto hide in non 3D view"),
    ParamInt('DockOverlayDelay', 200, "Overlay dock (re),layout delay.", title="Layout delay", proxy=ParamSpinBox(0, 5000, 100, suffix=" ms")),
    ParamInt('DockOverlayRevealDelay', 2000),
    ParamInt('DockOverlaySplitterHandleTimeout', 0, title="Splitter auto hide delay", proxy=ParamSpinBox(0, 99999, 100, suffix=" ms"),
         doc="Overlay splitter handle auto hide delay. Set zero to disable auto hiding."),
    ParamBool('DockOverlayActivateOnHover', True, title="Activate on hover",
         doc="Show auto hidden dock overlay on mouse over.\n"
             "If disabled, then show on mouse click."),
    ParamBool('DockOverlayAutoMouseThrough', True,
         "Auto mouse click through transparent part of dock overlay.", title="Auto mouse pass through"),
    ParamBool('DockOverlayWheelPassThrough', True,
         "Auto pass through mouse wheel event on transparent dock overlay.", title="Auto mouse wheel pass through"),
    ParamInt('DockOverlayWheelDelay', 1000, title="Delay mouse wheel pass through", proxy=ParamSpinBox(0, 99999, 1, suffix=" ms"),
         doc="Delay capturing mouse wheel event for passing through if it is\n"
              "previously handled by other widget."),
    ParamInt('DockOverlayAlphaRadius', 2, title="Alpha test radius", proxy=ParamSpinBox(1, 100, 1, suffix=" px"), doc=\
         "If auto mouse click through is enabled, then this radius\n"
         "defines a region of alpha test under the mouse cursor.\n"
         "Auto click through is only activated if all pixels within\n"
         "the region are non-opaque."),
    ParamBool('DockOverlayCheckNaviCube', True, on_change=True, title="Check Navigation Cube",
         doc="Leave space for Navigation Cube in dock overlay"),
    ParamInt('DockOverlayHintTriggerSize', 16, title="Hint trigger size", proxy=ParamSpinBox(1, 100, 1, suffix=" px"),
         doc="Auto hide hint visual display triggering width"),
    ParamInt('DockOverlayHintSize', 8, title="Hint width", proxy=ParamSpinBox(1, 100, 1, suffix=" px"),
         doc="Auto hide hint visual display width"),
    ParamInt('DockOverlayHintLeftLength', 100, title='Left panel hint length', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display length for left panel. Set to zero to fill the space."),
    ParamInt('DockOverlayHintRightLength', 100, title='Right panel hint length', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display length for right panel. Set to zero to fill the space."),
    ParamInt('DockOverlayHintTopLength', 100, title='Top panel hint length', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display length for top panel. Set to zero to fill the space."),
    ParamInt('DockOverlayHintBottomLength', 100, title='Bottom panel hint length', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display length for bottom panel. Set to zero to fill the space."),
    ParamInt('DockOverlayHintLeftOffset', 0, title='Left panel hint offset', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display offset for left panel"),
    ParamInt('DockOverlayHintRightOffset', 0, title='Right panel hint offset', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display offset for right panel"),
    ParamInt('DockOverlayHintTopOffset', 0, title='Top panel hint offset', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display offset for top panel"),
    ParamInt('DockOverlayHintBottomOffset', 0, title='Bottom panel hint offset', proxy=ParamSpinBox(0, 10000, 10, suffix=" px"),
         doc="Auto hide hint visual display offset for bottom panel"),
    ParamBool('DockOverlayHintTabBar', False, "Show tab bar on mouse over when auto hide", title="Hint show tab bar"),
    ParamBool('DockOverlayHideTabBar', True, on_change=True, doc="Hide tab bar in dock overlay", title='Hide tab bar'),
    ParamInt('DockOverlayHintDelay', 200, "Delay before show hint visual", title="Hint delay", proxy=ParamSpinBox(0, 1000, 100, suffix=" ms")),
    ParamInt('DockOverlayAnimationDuration', 200, "Auto hide animation duration, 0 to disable",
         title="Animation duration", proxy=ParamSpinBox(0, 5000, 100, suffix=" ms")),
    ParamInt('DockOverlayAnimationCurve', 7, "Auto hide animation curve type", title="Animation curve type", proxy=ParamAnimationCurve()),
    ParamBool('DockOverlayHidePropertyViewScrollBar', False, "Hide property view scroll bar in dock overlay", title="Hide property view scroll bar"),
    ParamInt('DockOverlayMinimumSize', 30, on_change=True,
            doc="Minimum overlay dock widget width/height",
            title="Minimum dock widget size"),
]

def declare():
    cog.out(f'''
{auto_comment()}
#include <QString>
''')

    params_utils.declare_begin(sys.modules[__name__])
    cog.out(f'''
    {auto_comment()}
    static const std::vector<QString> AnimationCurveTypes;
''')
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])
    cog.out(f'''
{auto_comment()}
const std::vector<QString> OverlayParams::AnimationCurveTypes = {{''')
    for item in AnimationCurveTypes:
        cog.out(f'''
    QStringLiteral("{item}"),''')
    cog.out(f'''
}};
''')


params_utils.init_params(Params, NameSpace, ClassName, ParamPath)
