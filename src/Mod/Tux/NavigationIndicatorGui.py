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


try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text):
        "convenience function for Qt 4 translator"
        return QtGui.QApplication.translate(context, text, None, _encoding)

except AttributeError:

    def translate(context, text):
        "convenience function for Qt 5 translator"
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
      <td align='center'><img src=':/icons/NavigationBlender_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_PanAlt.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationCAD_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_RotateAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_Pan.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationGesture_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_RotateAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_Tilt.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationGesture_SelectTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_ZoomTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_RotateTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_PanTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_PanTouchAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationGesture_TiltTouch.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationMayaGesture_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_ZoomAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_Tilt.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationMayaGesture_SelectTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_ZoomTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_RotateTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_PanTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_PanTouchAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationMayaGesture_TiltTouch.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationOpenCascade_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenCascade_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenCascade_ZoomAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenCascade_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenCascade_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenCascade_PanAlt.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationOpenInventor_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_ZoomAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Pan.svg'></td>
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
        + text04
        + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/NavigationOpenSCAD_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenSCAD_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenSCAD_ZoomAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenSCAD_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenSCAD_Pan.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationBlender_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationRevit_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationRevit_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_PanAlt.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationTinkerCAD_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationTinkerCAD_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationTinkerCAD_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationTinkerCAD_Pan.svg'></td>
     </tr>
    </table>"""
    )

    global t10
    t10 = (
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
      <td align='center'><img src=':/icons/NavigationTouchpad_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_ZoomAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_RotateAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_Pan.svg'></td>
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
      <td align='center'><img src=':/icons/NavigationTouchpad_SelectTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_ZoomTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_RotateTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_RotateTouchAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_PanTouch.svg'></td>
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
    a0.setText(translate("NavigationIndicator", "Undefined"))


indicator = IndicatorButton(statusBar)
indicator.setFlat(True)
indicator.adjustSize()
indicator.setObjectName("NavigationIndicator")

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

menuOrbit.addAction(aTurntable)
menuOrbit.addAction(aTrackball)
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
a1.setIcon(QtGui.QIcon(":/icons/NavigationBlender_dark.svg"))
a1.setText("Blender  ")
a1.setData("Gui::BlenderNavigationStyle")
a1.setObjectName("Indicator_NavigationBlender")

a2 = QtGui.QAction(gStyle)
a2.setIcon(QtGui.QIcon(":/icons/NavigationCAD_dark.svg"))
a2.setText("CAD  ")
a2.setData("Gui::CADNavigationStyle")
a2.setObjectName("Indicator_NavigationCAD")

a3 = QtGui.QAction(gStyle)
a3.setIcon(QtGui.QIcon(":/icons/NavigationGesture_dark.svg"))
a3.setText("Gesture  ")
a3.setData("Gui::GestureNavigationStyle")
a3.setObjectName("Indicator_NavigationGesture")

a4 = QtGui.QAction(gStyle)
a4.setIcon(QtGui.QIcon(":/icons/NavigationMayaGesture_dark.svg"))
a4.setText("MayaGesture  ")
a4.setData("Gui::MayaGestureNavigationStyle")
a4.setObjectName("Indicator_NavigationMayaGesture")

a5 = QtGui.QAction(gStyle)
a5.setIcon(QtGui.QIcon(":/icons/NavigationOpenCascade_dark.svg"))
a5.setText("OpenCascade  ")
a5.setData("Gui::OpenCascadeNavigationStyle")
a5.setObjectName("Indicator_NavigationOpenCascade")

a6 = QtGui.QAction(gStyle)
a6.setIcon(QtGui.QIcon(":/icons/NavigationOpenInventor_dark.svg"))
a6.setText("OpenInventor  ")
a6.setData("Gui::InventorNavigationStyle")
a6.setObjectName("Indicator_NavigationOpenInventor")

a7 = QtGui.QAction(gStyle)
a7.setIcon(QtGui.QIcon(":/icons/NavigationOpenSCAD_dark.svg"))
a7.setText("OpenSCAD  ")
a7.setData("Gui::OpenSCADNavigationStyle")
a7.setObjectName("Indicator_NavigationOpenSCAD")

a8 = QtGui.QAction(gStyle)
a8.setIcon(QtGui.QIcon(":/icons/NavigationRevit_dark.svg"))
a8.setText("Revit  ")
a8.setData("Gui::RevitNavigationStyle")
a8.setObjectName("Indicator_NavigationRevit")

a9 = QtGui.QAction(gStyle)
a9.setIcon(QtGui.QIcon(":/icons/NavigationTinkerCAD_dark.svg"))
a9.setText("TinkerCAD  ")
a9.setData("Gui::TinkerCADNavigationStyle")
a9.setObjectName("Indicator_NavigationTinkerCAD")

a10 = QtGui.QAction(gStyle)
a10.setIcon(QtGui.QIcon(":/icons/NavigationTouchpad_dark.svg"))
a10.setText("Touchpad  ")
a10.setData("Gui::TouchpadNavigationStyle")
a10.setObjectName("Indicator_NavigationTouchpad")

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


def onOrbitShow():
    """Set turntable or trackball orbit style."""

    OrbitStyle = pView.GetInt("OrbitStyle", 0)
    gOrbit.blockSignals(True)
    if OrbitStyle == 0:
        aTurntable.setChecked(True)
    elif OrbitStyle == 1:
        aTrackball.setChecked(True)
    elif OrbitStyle == 2:
        aFreeTurntable.setChecked(True)
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
