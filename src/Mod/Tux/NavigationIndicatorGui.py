# SPDX-License-Identifier: LGPL-2.1-or-later

# Navigation indicator for FreeCAD
# Copyright (C) 2016, 2017, 2018 triplus @ FreeCAD
#
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

"""Navigation indicator for FreeCAD."""

import Tux_rc
import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui
from PySide import QtCore

mw = Gui.getMainWindow()
statusBar = mw.statusBar()
p = App.ParamGet("User parameter:Tux/NavigationIndicator")
pView = App.ParamGet("User parameter:BaseApp/Preferences/View")
pMWin = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")


def translate(context, text):
    "convenience function for Qt 5/6 translator"
    return QtGui.QApplication.translate(context, text, None)


class IndicatorButton(QtGui.QPushButton):
    """Detect language change events."""

    def __init__(self, parent=None):
        super(IndicatorButton, self).__init__()

    def changeEvent(self, event):
        """Change events."""
        if event.type() == QtCore.QEvent.LanguageChange:
            retranslateUi()
            onTooltip()
            self.adjustSize()
        return super(IndicatorButton, self).changeEvent(event)

    def onChange(self, paramGrp, param):
        if param == "NavigationStyle":
            setCurrent()

    def mousePressEvent(self, event):
        RePopulateIcons()
        return super(IndicatorButton, self).mousePressEvent(event)


def RePopulateIcons():
    curStyleSheet = pMWin.GetString("StyleSheet")
    if "dark" in curStyleSheet.lower():
        StyleSheetType = "light"
    else:
        StyleSheetType = "dark"

    a1.setIcon(QtGui.QIcon(":/icons/NavigationBlender_" + StyleSheetType + ".svg"))
    a2.setIcon(QtGui.QIcon(":/icons/NavigationCAD_" + StyleSheetType + ".svg"))
    a3.setIcon(QtGui.QIcon(":/icons/NavigationGesture_" + StyleSheetType + ".svg"))
    a4.setIcon(QtGui.QIcon(":/icons/NavigationMayaGesture_" + StyleSheetType + ".svg"))
    a5.setIcon(QtGui.QIcon(":/icons/NavigationOpenCascade_" + StyleSheetType + ".svg"))
    a6.setIcon(QtGui.QIcon(":/icons/NavigationOpenInventor_" + StyleSheetType + ".svg"))
    a7.setIcon(QtGui.QIcon(":/icons/NavigationOpenSCAD_" + StyleSheetType + ".svg"))
    a8.setIcon(QtGui.QIcon(":/icons/NavigationRevit_" + StyleSheetType + ".svg"))
    a9.setIcon(QtGui.QIcon(":/icons/NavigationSiemensNX_" + StyleSheetType + ".svg"))
    a10.setIcon(QtGui.QIcon(":/icons/NavigationSolidWorks_" + StyleSheetType + ".svg"))
    a11.setIcon(QtGui.QIcon(":/icons/NavigationTinkerCAD_" + StyleSheetType + ".svg"))
    a12.setIcon(QtGui.QIcon(":/icons/NavigationTouchpad_" + StyleSheetType + ".svg"))


def retranslateUi():
    """Retranslate navigation indicator on language change"""

    text01 = translate("NavigationIndicator", "Select")
    text02 = translate("NavigationIndicator", "Zoom")
    text03 = translate("NavigationIndicator", "Rotate")
    text04 = translate("NavigationIndicator", "Pan")
    text05 = translate("NavigationIndicator", "Tilt")
    text06 = translate("NavigationIndicator", "Navigation style")
    text07 = translate("NavigationIndicator", "Page Up or Page Down key.")
    text08 = translate("NavigationIndicator", "Rotation focus")
    text09 = translate("NavigationIndicator", "Middle mouse button or H key.")
    text10 = translate("NavigationIndicator", "Middle mouse button.")

    global t0
    t0 = translate("NavigationIndicator", "Navigation style not recognized.")

    global t1
    t1 = (
        "<p align='center'><b>Blender</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftMiddle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_LeftRight.svg'></td>
     </tr>
    </table>
    <b>"""
        + text08
        + ":</b> "
        + text10
        + "</small></p>"
    )

    global t2
    t2 = (
        "<p align='center'><b>CAD</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_MiddleLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_MiddleRight.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
     </tr>
    </table>
    <b>"""
        + text08
        + ":</b> "
        + text10
        + "</small></p>"
    )

    global t3
    t3 = (
        "<p align='center'><b>Gesture</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text05
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_LeftMove.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_AltLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Right.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_LeftRight.svg'></td>
     </tr>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text05
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Gesture_SelectTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_ZoomTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_RotateTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_PanTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_PanTouchAlt.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_TiltTouch.svg'></td>
     </tr>
    </table>
    <p><small><b>"""
        + text02
        + ":</b> "
        + text07
        + """<br>
    <b>"""
        + text08
        + ":</b> "
        + text09
        + "</small></p>"
    )

    global t4
    t4 = (
        "<p align='center'><b>MayaGesture</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text05
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_AltRight.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_AltLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_AltMiddle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_AltLeftRight.svg'></td>
     </tr>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text05
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Gesture_SelectTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_ZoomTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_RotateTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_PanTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_PanTouchAlt.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Gesture_TiltTouch.svg'></td>
     </tr>
    </table>
    <p><small><b>"""
        + text02
        + ":</b> "
        + text07
        + """<br>
    <b>"""
        + text08
        + ":</b> "
        + text09
        + "</small></p>"
    )

    global t5
    t5 = (
        "<p align='center'><b>OpenCascade</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_CtrlLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_CtrlRight.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_CtrlMiddle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
     </tr>
    </table>"""
    )

    global t6
    t6 = (
        "<p align='center'><b>OpenInventor</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_MiddleLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
     </tr>
    </table>
    <b>"""
        + text08
        + ":</b> "
        + text10
        + "</small></p>"
    )

    global t7
    t7 = (
        "<p align='center'><b>OpenSCAD</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_MiddleRight.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Right.svg'></td>
     </tr>
    </table>"""
    )

    global t8
    t8 = (
        "<p align='center'><b>Revit</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftMiddle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_LeftRight.svg'></td>
     </tr>
    </table>
    <b>"""
        + text08
        + ":</b> "
        + text10
        + "</small></p>"
    )

    global t9
    t9 = (
        "<p align='center'><b>Siemens NX</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
        <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_MiddleLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_MiddleRight.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftMiddle.svg'></td>
     </tr>
    </table>
    <b>"""
        + text08
        + ":</b> "
        + text10
        + "</small></p>"
    )

    global t10
    t10 = (
        "<p align='center'><b>SolidWorks</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftMiddle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_CtrlMiddle.svg'></td>
     </tr>
    </table>
    <b>"""
        + text08
        + ":</b> "
        + text10
        + "</small></p>"
    )

    global t11
    t11 = (
        "<p align='center'><b>TinkerCAD</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Right.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Middle.svg'></td>
     </tr>
    </table>"""
    )

    global t12
    t12 = (
        "<p align='center'><b>Touchpad</b> "
        + text06
        + """</p>
    <table>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Mouse_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_Scroll.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftCtrlMove.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_AltMove.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftLeft.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Mouse_ShiftMove.svg'></td>
     </tr>
     <tr>
      <th><small>"""
        + text01
        + """</small></th>
      <th><small>"""
        + text02
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text03
        + """</small></th>
      <th><small>"""
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/Navigation_Touchpad_Left.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Touchpad_ShiftCtrlTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Touchpad_AltTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Touchpad_ShiftLeftTouch.svg'></td>
      <td align='center'><img src=':/icons/Navigation_Touchpad_ShiftTouch.svg'></td>
     </tr>
    </table>
    <p><small><b>"""
        + text02
        + ":</b> "
        + text07
        + "</p>"
    )

    menuSettings.setTitle(translate("NavigationIndicator", "Settings"))
    menuOrbit.setTitle(translate("NavigationIndicator", "Orbit style"))
    aCompact.setText(translate("NavigationIndicator", "Compact"))
    aTooltip.setText(translate("NavigationIndicator", "Tooltip"))
    aTurntable.setText(translate("NavigationIndicator", "Turntable"))
    aFreeTurntable.setText(translate("NavigationIndicator", "Free Turntable"))
    aTrackball.setText(translate("NavigationIndicator", "Trackball"))
    aTrackballClassic.setText(translate("NavigationIndicator", "Trackball Classic"))
    aRoundedArcball.setText(translate("NavigationIndicator", "Rounded Arcball"))
    a0.setText(translate("NavigationIndicator", "Undefined"))


indicator = IndicatorButton(statusBar)
indicator.setFlat(True)
indicator.adjustSize()
indicator.setObjectName("NavigationIndicator")
text = QtGui.QApplication.translate(
    "NavigationIndicator",
    "Navigation Indicator",
    "A context menu action used to show or hide the 'Navigation indicator' toolbar widget",
)
indicator.setWindowTitle(text)

menu = QtGui.QMenu(indicator)
indicator.setMenu(menu)

menuSettings = QtGui.QMenu(menu)
menuOrbit = QtGui.QMenu(menu)

aCompact = QtGui.QAction(menuSettings)
aCompact.setCheckable(True)
aTooltip = QtGui.QAction(menuSettings)
aTooltip.setCheckable(True)

gOrbit = QtGui.QActionGroup(menuSettings)

aTurntable = QtGui.QAction(gOrbit)
aTurntable.setObjectName("NavigationIndicator_Turntable")
aTurntable.setCheckable(True)
aTrackball = QtGui.QAction(gOrbit)
aTrackball.setObjectName("NavigationIndicator_Trackball")
aTrackball.setCheckable(True)
aFreeTurntable = QtGui.QAction(gOrbit)
aFreeTurntable.setObjectName("NavigationIndicator_FreeTurntable")
aFreeTurntable.setCheckable(True)
aTrackballClassic = QtGui.QAction(gOrbit)
aTrackballClassic.setObjectName("NavigationIndicator_TrackballClassic")
aTrackballClassic.setCheckable(True)
aRoundedArcball = QtGui.QAction(gOrbit)
aRoundedArcball.setObjectName("NavigationIndicator_RoundedArcball")
aRoundedArcball.setCheckable(True)

menuOrbit.addAction(aRoundedArcball)
menuOrbit.addAction(aTrackball)
menuOrbit.addAction(aTrackballClassic)
menuOrbit.addAction(aTurntable)
menuOrbit.addAction(aFreeTurntable)

menuSettings.addMenu(menuOrbit)
menuSettings.addSeparator()
menuSettings.addAction(aCompact)
menuSettings.addAction(aTooltip)

gStyle = QtGui.QActionGroup(menu)

a0 = QtGui.QAction(gStyle)
a0.setIcon(QtGui.QIcon(":/icons/NavigationUndefined.svg"))
a0.setData("Undefined  ")
a0.setObjectName("Indicator_NavigationUndefined")

a1 = QtGui.QAction(gStyle)
a1.setText("Blender  ")
a1.setData("Gui::BlenderNavigationStyle")
a1.setObjectName("Indicator_NavigationBlender")

a2 = QtGui.QAction(gStyle)
a2.setText("CAD  ")
a2.setData("Gui::CADNavigationStyle")
a2.setObjectName("Indicator_NavigationCAD")

a3 = QtGui.QAction(gStyle)
a3.setText("Gesture  ")
a3.setData("Gui::GestureNavigationStyle")
a3.setObjectName("Indicator_NavigationGesture")

a4 = QtGui.QAction(gStyle)
a4.setText("MayaGesture  ")
a4.setData("Gui::MayaGestureNavigationStyle")
a4.setObjectName("Indicator_NavigationMayaGesture")

a5 = QtGui.QAction(gStyle)
a5.setText("OpenCascade  ")
a5.setData("Gui::OpenCascadeNavigationStyle")
a5.setObjectName("Indicator_NavigationOpenCascade")

a6 = QtGui.QAction(gStyle)
a6.setText("OpenInventor  ")
a6.setData("Gui::InventorNavigationStyle")
a6.setObjectName("Indicator_NavigationOpenInventor")

a7 = QtGui.QAction(gStyle)
a7.setText("OpenSCAD  ")
a7.setData("Gui::OpenSCADNavigationStyle")
a7.setObjectName("Indicator_NavigationOpenSCAD")

a8 = QtGui.QAction(gStyle)
a8.setText("Revit  ")
a8.setData("Gui::RevitNavigationStyle")
a8.setObjectName("Indicator_NavigationRevit")

a9 = QtGui.QAction(gStyle)
a9.setText("Siemens NX  ")
a9.setData("Gui::SiemensNXNavigationStyle")
a9.setObjectName("Indicator_NavigationSiemensNX")

a10 = QtGui.QAction(gStyle)
a10.setText("SolidWorks  ")
a10.setData("Gui::SolidWorksNavigationStyle")
a10.setObjectName("Indicator_NavigationSolidWorks")

a11 = QtGui.QAction(gStyle)
a11.setText("TinkerCAD  ")
a11.setData("Gui::TinkerCADNavigationStyle")
a11.setObjectName("Indicator_NavigationTinkerCAD")

a12 = QtGui.QAction(gStyle)
a12.setText("Touchpad  ")
a12.setData("Gui::TouchpadNavigationStyle")
a12.setObjectName("Indicator_NavigationTouchpad")

RePopulateIcons()

menu.addMenu(menuSettings)
menu.addSeparator()
menu.addAction(a0)
menu.addAction(a1)
menu.addAction(a2)
menu.addAction(a3)
menu.addAction(a4)
menu.addAction(a5)
menu.addAction(a6)
menu.addAction(a7)
menu.addAction(a8)
menu.addAction(a9)
menu.addAction(a10)
menu.addAction(a11)
menu.addAction(a12)

pView.Attach(indicator)


def onCompact():
    """Enable or disable compact mode."""

    if aCompact.isChecked():
        p.SetBool("Compact", 1)
    else:
        p.SetBool("Compact", 0)

    setCurrent()


def setCompact(action):
    """Set compact mode."""

    if p.GetBool("Compact", 0):
        indicator.setText("")
    else:
        indicator.setText(action.text())
        indicator.adjustSize()


def onTooltip():
    """Enable or disable verbose tooltips."""

    if aTooltip.isChecked():
        a0.setToolTip(t0)
        a1.setToolTip(t1)
        a2.setToolTip(t2)
        a3.setToolTip(t3)
        a4.setToolTip(t4)
        a5.setToolTip(t5)
        a6.setToolTip(t6)
        a7.setToolTip(t7)
        a8.setToolTip(t8)
        a9.setToolTip(t9)
        a10.setToolTip(t10)
        a11.setToolTip(t11)
        a12.setToolTip(t12)
        p.SetBool("Tooltip", 1)
    else:
        for i in gStyle.actions():
            i.setToolTip("")
        p.SetBool("Tooltip", 0)

    setCurrent()


def onOrbit():
    """Use turntable or trackball orbit style."""

    if aTurntable.isChecked():
        pView.SetInt("OrbitStyle", 0)
    elif aTrackball.isChecked():
        pView.SetInt("OrbitStyle", 1)
    elif aFreeTurntable.isChecked():
        pView.SetInt("OrbitStyle", 2)
    elif aTrackballClassic.isChecked():
        pView.SetInt("OrbitStyle", 3)
    elif aRoundedArcball.isChecked():
        pView.SetInt("OrbitStyle", 4)


def onOrbitShow():
    """Set turntable or trackball orbit style."""

    OrbitStyle = pView.GetInt("OrbitStyle", 4)
    gOrbit.blockSignals(True)
    if OrbitStyle == 0:
        aTurntable.setChecked(True)
    elif OrbitStyle == 1:
        aTrackball.setChecked(True)
    elif OrbitStyle == 2:
        aFreeTurntable.setChecked(True)
    elif OrbitStyle == 3:
        aTrackballClassic.setChecked(True)
    elif OrbitStyle == 4:
        aRoundedArcball.setChecked(True)
    gOrbit.blockSignals(False)


def onMenu(action):
    """Set navigation style on selection."""
    pView.SetString("NavigationStyle", action.data())


def setCurrent():
    """Set navigation style on start and on interval."""
    gStyle.blockSignals(True)

    s = False
    actions = gStyle.actions()
    current = pView.GetString("NavigationStyle")

    if current and current != "Undefined":
        for i in actions:
            if i.data() == current:
                s = True
                setCompact(i)
                menu.setDefaultAction(i)
                indicator.setIcon(i.icon())
                indicator.setToolTip(i.toolTip())
            else:
                pass
    else:
        s = True
        pView.SetString("NavigationStyle", a2.data())

    if s:
        a0.setVisible(False)
    else:
        a0.setVisible(True)
        a0.setEnabled(True)
        setCompact(a0)
        menu.setDefaultAction(a0)
        indicator.setIcon(a0.icon())
        indicator.setToolTip(a0.toolTip())

    gStyle.blockSignals(False)


if p.GetBool("Compact", 0):
    aCompact.setChecked(True)

if p.GetBool("Tooltip", 1):
    aTooltip.setChecked(True)

retranslateUi()
onCompact()
onTooltip()

label = statusBar.children()[2]
statusBar.removeWidget(label)
statusBar.addPermanentWidget(indicator)
statusBar.addPermanentWidget(label)
label.show()

setCurrent()

gStyle.triggered.connect(onMenu)
gOrbit.triggered.connect(onOrbit)
aCompact.triggered.connect(onCompact)
aTooltip.triggered.connect(onTooltip)
menuOrbit.aboutToShow.connect(onOrbitShow)
menu.aboutToHide.connect(indicator.clearFocus)
