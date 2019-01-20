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

timer = QtCore.QTimer()
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
        return super(IndicatorButton, self).changeEvent(event)


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
    t1 = "<p align='center'><b>OpenInventor</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_ZoomAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationOpenInventor_Pan.svg'></td>
     </tr>
    </table>
    <b>""" + text08 + ":</b> " + text10 + "</small></p>"

    global t2
    t2 = "<p align='center'><b>CAD</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/NavigationCAD_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_RotateAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationCAD_Pan.svg'></td>
     </tr>
    </table>
    <b>""" + text08 + ":</b> " + text10 + "</small></p>"

    global t3
    t3 = "<p align='center'><b>Revit</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text04 + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/NavigationBlender_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationRevit_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationRevit_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_PanAlt.svg'></td>
     </tr>
    </table>
    <b>""" + text08 + ":</b> " + text10 + "</small></p>"

    global t4
    t4 = "<p align='center'><b>Blender</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text04 + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/NavigationBlender_Select.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Zoom.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Rotate.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_Pan.svg'></td>
      <td align='center'><img src=':/icons/NavigationBlender_PanAlt.svg'></td>
     </tr>
    </table>
    <b>""" + text08 + ":</b> " + text10 + "</small></p>"

    global t5
    t5 = "<p align='center'><b>MayaGesture</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text05 + """</small></th>
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
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text05 + """</small></th>
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
    <p><small><b>""" + text02 + ":</b> " + text07 + """<br>
    <b>""" + text08 + ":</b> " + text09 + "</small></p>"

    global t6
    t6 = "<p align='center'><b>Touchpad</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
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
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
     </tr>
     <tr>
      <td align='center'><img src=':/icons/NavigationTouchpad_SelectTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_ZoomTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_RotateTouch.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_RotateTouchAlt.svg'></td>
      <td align='center'><img src=':/icons/NavigationTouchpad_PanTouch.svg'></td>
     </tr>
    </table>
    <p><small><b>""" + text02 + ":</b> " + text07 + "</p>"

    global t7
    t7 = "<p align='center'><b>Gesture</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text05 + """</small></th>
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
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text05 + """</small></th>
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
    <p><small><b>""" + text02 + ":</b> " + text07 + """<br>
    <b>""" + text08 + ":</b> " + text09 + "</small></p>"

    global t8
    t8 = "<p align='center'><b>OpenCascade</b> " + text06 + """</p>
    <table>
     <tr>
      <th><small>""" + text01 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text02 + """</small></th>
      <th><small>""" + text03 + """</small></th>
      <th><small>""" + text04 + """</small></th>
      <th><small>""" + text04 + """</small></th>
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

    menuSettings.setTitle(translate("NavigationIndicator", "Settings"))
    menuOrbit.setTitle(translate("NavigationIndicator", "Orbit style"))
    aCompact.setText(translate("NavigationIndicator", "Compact"))
    aTooltip.setText(translate("NavigationIndicator", "Tooltip"))
    aTurntable.setText(translate("NavigationIndicator", "Turntable"))
    aTrackball.setText(translate("NavigationIndicator", "Trackball"))
    a0.setText(translate("NavigationIndicator", "Undefined"))


indicator = IndicatorButton(statusBar)
indicator.setFlat(True)
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

menuOrbit.addAction(aTurntable)
menuOrbit.addAction(aTrackball)

menuSettings.addMenu(menuOrbit)
menuSettings.addSeparator()
menuSettings.addAction(aCompact)
menuSettings.addAction(aTooltip)

gStyle = QtGui.QActionGroup(menu)

a0 = QtGui.QAction(gStyle)
a0.setIcon(QtGui.QIcon(":/icons/NavigationUndefined.svg"))
a0.setData("Undefined")
a0.setObjectName("Indicator_NavigationUndefined")

a1 = QtGui.QAction(gStyle)
a1.setIcon(QtGui.QIcon(":/icons/NavigationOpenInventor.svg"))
a1.setText("OpenInventor")
a1.setData("Gui::InventorNavigationStyle")
a1.setObjectName("Indicator_NavigationOpenInventor")

a2 = QtGui.QAction(gStyle)
a2.setIcon(QtGui.QIcon(':/icons/NavigationCAD.svg'))
a2.setText("CAD")
a2.setData("Gui::CADNavigationStyle")
a2.setObjectName("Indicator_NavigationCAD")

a3 = QtGui.QAction(gStyle)
a3.setIcon(QtGui.QIcon(":/icons/NavigationRevit.svg"))
a3.setText("Revit")
a3.setData("Gui::RevitNavigationStyle")
a3.setObjectName("Indicator_NavigationRevit")

a4 = QtGui.QAction(gStyle)
a4.setIcon(QtGui.QIcon(":/icons/NavigationBlender.svg"))
a4.setText("Blender")
a4.setData("Gui::BlenderNavigationStyle")
a4.setObjectName("Indicator_NavigationBlender")

a5 = QtGui.QAction(gStyle)
a5.setIcon(QtGui.QIcon(":/icons/NavigationMayaGesture.svg"))
a5.setText("MayaGesture")
a5.setData("Gui::MayaGestureNavigationStyle")
a5.setObjectName("Indicator_NavigationMayaGesture")

a6 = QtGui.QAction(gStyle)
a6.setIcon(QtGui.QIcon(":/icons/NavigationTouchpad.svg"))
a6.setText("Touchpad")
a6.setData("Gui::TouchpadNavigationStyle")
a6.setObjectName("Indicator_NavigationTouchpad")

a7 = QtGui.QAction(gStyle)
a7.setIcon(QtGui.QIcon(":/icons/NavigationGesture.svg"))
a7.setText("Gesture")
a7.setData("Gui::GestureNavigationStyle")
a7.setObjectName("Indicator_NavigationGesture")

a8 = QtGui.QAction(gStyle)
a8.setIcon(QtGui.QIcon(":/icons/NavigationOpenCascade.svg"))
a8.setText("OpenCascade")
a8.setData("Gui::OpenCascadeNavigationStyle")
a8.setObjectName("Indicator_NavigationOpenCascade")

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
    else:
        pView.SetInt("OrbitStyle", 1)


def onOrbitShow():
    """Set turntable or trackball orbit style."""

    gOrbit.blockSignals(True)
    if pView.GetInt("OrbitStyle", 1):
        aTrackball.setChecked(True)
    else:
        aTurntable.setChecked(True)
    gOrbit.blockSignals(False)


def onMenu(action):
    """Set navigation style on selection."""

    s = False

    if action and action.data() != "Undefined":
        s = True
        setCompact(action)
        menu.setDefaultAction(action)
        indicator.setIcon(action.icon())
        indicator.setToolTip(action.toolTip())
        pView.SetString("NavigationStyle", action.data())
    else:
        pass

    if s:
        a0.setVisible(False)
    else:
        a0.setVisible(True)
        a0.setEnabled(True)
        setCompact(a0)
        menu.setDefaultAction(a0)
        indicator.setIcon(a0.icon())
        indicator.setToolTip(a0.toolTip())


def setCurrent():
    """Set navigation style on start and on interval."""
    gStyle.blockSignals(True)

    s = False
    actions = gStyle.actions()
    current = pView.GetString("NavigationStyle")

    if current:
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
        setCompact(a2)
        menu.setDefaultAction(a2)
        indicator.setIcon(a2.icon())
        indicator.setToolTip(a2.toolTip())
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
onOrbitShow()

statusBar.addPermanentWidget(indicator)
statusBar.addPermanentWidget(statusBar.children()[2])

setCurrent()

gStyle.triggered.connect(onMenu)
gOrbit.triggered.connect(onOrbit)
aCompact.triggered.connect(onCompact)
aTooltip.triggered.connect(onTooltip)
menuOrbit.aboutToShow.connect(onOrbitShow)
menu.aboutToHide.connect(indicator.clearFocus)

timer.timeout.connect(setCurrent)
timer.start(10000)
