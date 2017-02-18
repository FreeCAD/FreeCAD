# Navigation indicator for FreeCAD
# Copyright (C) 2016, 2017  triplus @ FreeCAD
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

t0 = "Navigation style not recognized."

t1 = str("""<p align='center'><b>OpenInventor</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
 </tr>
 <tr>
  <td align='center'><img src=':/icons/NavigationOpenInventor_Select.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenInventor_Zoom.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenInventor_ZoomAlt.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenInventor_Rotate.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenInventor_Pan.svg'></td>
 </tr>
</table>""")

t2 = str("""<p align='center'><b>CAD</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
 </tr>
 <tr>
  <td align='center'><img src=':/icons/NavigationCAD_Select.svg'></td>
  <td align='center'><img src=':/icons/NavigationCAD_Zoom.svg'></td>
  <td align='center'><img src=':/icons/NavigationCAD_Rotate.svg'></td>
  <td align='center'><img src=':/icons/NavigationCAD_RotateAlt.svg'></td>
  <td align='center'><img src=':/icons/NavigationCAD_Pan.svg'></td>
 </tr>
</table>""")

t3 = str("""<p align='center'><b>Blender</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
  <th><small>Pan</small></th>
 </tr>
 <tr>
  <td align='center'><img src=':/icons/NavigationBlender_Select.svg'></td>
  <td align='center'><img src=':/icons/NavigationBlender_Zoom.svg'></td>
  <td align='center'><img src=':/icons/NavigationBlender_Rotate.svg'></td>
  <td align='center'><img src=':/icons/NavigationBlender_Pan.svg'></td>
  <td align='center'><img src=':/icons/NavigationBlender_PanAlt.svg'></td>
 </tr>
</table>""")

t4 = str("""<p align='center'><b>MayaGesture</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
  <th><small>Tilt</small></th>
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
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
  <th><small>Pan</small></th>
  <th><small>Tilt</small></th>
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
<p><small><b>Zoom:</b> Page Up or Page Down key.<br>
<b>Rotation focus:</b> Middle mouse button or key H.</small></p>""")

t5 = str("""<p align='center'><b>Touchpad</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
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
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
 </tr>
 <tr>
  <td align='center'><img src=':/icons/NavigationTouchpad_SelectTouch.svg'></td>
  <td align='center'><img src=':/icons/NavigationTouchpad_ZoomTouch.svg'></td>
  <td align='center'><img src=':/icons/NavigationTouchpad_RotateTouch.svg'></td>
  <td align='center'><img src=':/icons/NavigationTouchpad_RotateTouchAlt.svg'></td>
  <td align='center'><img src=':/icons/NavigationTouchpad_PanTouch.svg'></td>
 </tr>
</table>
<p><small><b>Zoom:</b> Page Up or Page Down key.</small></p>""")

t6 = str("""<p align='center'><b>Gesture</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
  <th><small>Tilt</small></th>
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
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
  <th><small>Pan</small></th>
  <th><small>Tilt</small></th>
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
<p><small><b>Zoom:</b> Page Up or Page Down key.<br>
<b>Rotation focus:</b> Middle mouse button or key H.</small></p>""")

t7 = str("""<p align='center'><b>OpenCascade</b> navigation style</p>
<table>
 <tr>
  <th><small>Select</small></th>
  <th><small>Zoom</small></th>
  <th><small>Zoom</small></th>
  <th><small>Rotate</small></th>
  <th><small>Pan</small></th>
  <th><small>Pan</small></th>
 </tr>
 <tr>
  <td align='center'><img src=':/icons/NavigationOpenCascade_Select.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenCascade_Zoom.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenCascade_ZoomAlt.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenCascade_Rotate.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenCascade_Pan.svg'></td>
  <td align='center'><img src=':/icons/NavigationOpenCascade_PanAlt.svg'></td>
 </tr>
</table>""")

indicator = QtGui.QToolButton(statusBar)
indicator.setAutoRaise(True)
indicator.setObjectName("Std_NavigationIndicator")
indicator.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
indicator.setPopupMode(QtGui.QToolButton
                       .ToolButtonPopupMode
                       .InstantPopup)

menu = QtGui.QMenu(indicator)
indicator.setMenu(menu)

menuSettings = QtGui.QMenu("Settings", menu)
menuOrbit = QtGui.QMenu("Orbit style", menu)

aCompact = QtGui.QAction(menuSettings)
aCompact.setText("Compact")
aCompact.setCheckable(True)
aTooltip = QtGui.QAction(menuSettings)
aTooltip.setText("Tooltip")
aTooltip.setCheckable(True)

gOrbit = QtGui.QActionGroup(menuSettings)

aTurntable = QtGui.QAction(gOrbit)
aTurntable.setText("Turntable")
aTurntable.setCheckable(True)
aTrackball = QtGui.QAction(gOrbit)
aTrackball.setText("Trackball")
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
a0.setText("Undefined")
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
a3.setIcon(QtGui.QIcon(":/icons/NavigationBlender.svg"))
a3.setText("Blender")
a3.setData("Gui::BlenderNavigationStyle")
a3.setObjectName("Indicator_NavigationBlender")

a4 = QtGui.QAction(gStyle)
a4.setIcon(QtGui.QIcon(":/icons/NavigationMayaGesture.svg"))
a4.setText("MayaGesture")
a4.setData("Gui::MayaGestureNavigationStyle")
a4.setObjectName("Indicator_NavigationMayaGesture")

a5 = QtGui.QAction(gStyle)
a5.setIcon(QtGui.QIcon(":/icons/NavigationTouchpad.svg"))
a5.setText("Touchpad")
a5.setData("Gui::TouchpadNavigationStyle")
a5.setObjectName("Indicator_NavigationTouchpad")

a6 = QtGui.QAction(gStyle)
a6.setIcon(QtGui.QIcon(":/icons/NavigationGesture.svg"))
a6.setText("Gesture")
a6.setData("Gui::GestureNavigationStyle")
a6.setObjectName("Indicator_NavigationGesture")

a7 = QtGui.QAction(gStyle)
a7.setIcon(QtGui.QIcon(":/icons/NavigationOpenCascade.svg"))
a7.setText("OpenCascade")
a7.setData("Gui::OpenCascadeNavigationStyle")
a7.setObjectName("Indicator_NavigationOpenCascade")

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


def onCompact():
    """Enable or disable compact mode."""
    if aCompact.isChecked():
        indicator.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        indicator.setStyleSheet("QToolButton::menu-indicator {image: none}")
        p.SetBool("Compact", 1)
    else:
        indicator.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
        indicator.setStyleSheet("QToolButton::menu-indicator {}")
        p.SetBool("Compact", 0)


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
        p.SetBool("Tooltip", 1)
    else:
        for i in gStyle.actions():
            i.setToolTip("")
        p.SetBool("Tooltip", 0)


def onOrbit():
    """Use turntable or trackball orbit style."""
    if aTurntable.isChecked():
        pView.SetInt("OrbitStyle", 1)
    else:
        pView.SetInt("OrbitStyle", 0)


def onMenu(action):
    """Set navigation style on selection."""

    s = False

    if action.data() != "Undefined":
        s = True
        menu.setDefaultAction(action)
        indicator.setDefaultAction(action)
        pView.SetString("NavigationStyle", action.data())
    else:
        pass

    if s:
        a0.setVisible(False)
    else:
        a0.setVisible(True)
        a0.setEnabled(True)
        menu.setDefaultAction(a0)
        indicator.setDefaultAction(a0)


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
                menu.setDefaultAction(i)
                indicator.setDefaultAction(i)
            else:
                pass
    else:
        s = True
        menu.setDefaultAction(a2)
        indicator.setDefaultAction(a2)
        pView.SetString("NavigationStyle", a2.data())

    if s:
        a0.setVisible(False)
    else:
        a0.setVisible(True)
        a0.setEnabled(True)
        menu.setDefaultAction(a0)
        indicator.setDefaultAction(a0)

    gStyle.blockSignals(False)

if p.GetBool("Compact"):
    aCompact.setChecked(True)

if p.GetBool("Tooltip", 1):
    aTooltip.setChecked(True)

if pView.GetInt("OrbitStyle", 1):
    aTurntable.setChecked(True)
else:
    aTrackball.setChecked(True)

onCompact()
onTooltip()
onOrbit()

statusBar.addPermanentWidget(indicator)
statusBar.addPermanentWidget(statusBar.children()[2])

setCurrent()

gStyle.triggered.connect(onMenu)
gOrbit.triggered.connect(onOrbit)
aCompact.triggered.connect(onCompact)
aTooltip.triggered.connect(onTooltip)

timer.timeout.connect(setCurrent)
timer.start(10000)
