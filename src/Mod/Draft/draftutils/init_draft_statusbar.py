# ***************************************************************************
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the initialization code for the workbench's status bar.

The status bar is activated by `InitGui.py` when the workbench is started,
and is populated by various widgets, buttons and menus.
"""
## @package init_draft_statusbar
# \ingroup draftutils
# \brief Provides the initialization code for the workbench's status bar.

## \addtogroup draftutils
# @{
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui

from draftutils.init_tools import get_draft_snap_commands
from draftutils.translate import translate

#----------------------------------------------------------------------------
# SCALE WIDGET FUNCTIONS
#----------------------------------------------------------------------------

draft_scales_metrics =  ["1:1000", "1:500", "1:250", "1:200", "1:100",
                         "1:50", "1:25","1:20", "1:10", "1:5","1:2",
                         "1:1",
                         "2:1", "5:1", "10:1", "20:1",
                         translate("draft", "Custom"),
                        ]

draft_scales_arch_imperial =  ["1/16in=1ft", "3/32in=1ft", "1/8in=1ft",
                               "3/16in=1ft", "1/4in=1ft","3/8in=1ft",
                               "1/2in=1ft", "3/4in=1ft", "1in=1ft",
                               "1.5in=1ft", "3in=1ft",
                               translate("draft", "Custom"),
                              ]

draft_scales_eng_imperial =  ["1in=10ft", "1in=20ft", "1in=30ft",
                              "1in=40ft", "1in=50ft", "1in=60ft",
                              "1in=70ft", "1in=80ft", "1in=90ft",
                              "1in=100ft",
                              translate("draft", "Custom"),
                             ]

def get_scales(unit_system = 0):
    """
    returns the list of preset scales according to unit system.

    Parameters:
    unit_system =   0 : default from user preferences
                    1 : metrics
                    2 : imperial architectural
                    3 : imperial engineering
    """

    if unit_system == 0:
        param = App.ParamGet("User parameter:BaseApp/Preferences/Units")
        scale_units_system = param.GetInt("UserSchema", 0)
        if scale_units_system in [0, 1, 4, 6]:
            return draft_scales_metrics
        elif scale_units_system in [2, 3, 5]:
            return draft_scales_arch_imperial
        elif scale_units_system in [7]:
            return draft_scales_eng_imperial
    elif unit_system == 1:
        return draft_scales_metrics
    elif unit_system == 2:
        return draft_scales_arch_imperial
    elif unit_system == 3:
        return draft_scales_eng_imperial


def scale_to_label(scale):
    """
    transform a float number into a 1:X or X:1 scale and return it as label
    """
    f = round(scale, 2)
    if f == 1.0:
        return "1:1"
    elif f > 1.0:
        f = f.as_integer_ratio()
        if f[1] == 1:
            return str(f[0]) + ":1"
        else:
            return str(scale)
    else:
        f = round(1/scale, 2)
        f = f.as_integer_ratio()
        if f[1] == 1:
            return "1:" + str(f[0])
        else:
            return str(scale)

def label_to_scale(label):
    """
    transform a scale string into scale factor as float
    """
    try :
        scale = float(label)
        return scale
    except Exception:
        if ":" in label:
            f = label.split(":")
        elif "=" in label:
            f = label.split("=")
        else:
            return
        if len(f) == 2:
            try:
                num = App.Units.Quantity(f[0]).Value
                den = App.Units.Quantity(f[1]).Value
                scale = num/den
                return scale
            except Exception:
                err = translate("draft",
                                "Unable to convert input into a scale factor")
                App.Console.PrintWarning(err)
                return None

def _set_scale(action):
    """
    triggered by scale pushbutton, set DraftAnnotationScale in preferences
    """
    # set the label of the scale button
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

    mw = Gui.getMainWindow()
    sb = mw.statusBar()

    scale_widget = sb.findChild(QtGui.QToolBar,"draft_scale_widget")

    if action.text() == translate("draft", "Custom"):
        title_text = translate("draft", "Set custom scale")
        dialog_text = translate("draft",
                                "Set custom annotation scale in format x:x, x=x")
        custom_scale = QtGui.QInputDialog.getText(None, title_text, dialog_text)
        if custom_scale[1]:
            print(custom_scale[0])
            scale = label_to_scale(custom_scale[0])
            if scale is None:
                return
            param.SetFloat("DraftAnnotationScale", scale)
            cs = scale_to_label(scale)
            scale_widget.scaleLabel.setText(cs)
    else:
        text_scale = action.text()
        scale_widget.scaleLabel.setText(text_scale)
        scale = label_to_scale(text_scale)
        param.SetFloat("DraftAnnotationScale", scale)

#----------------------------------------------------------------------------
# MAIN DRAFT STATUSBAR FUNCTIONS
#----------------------------------------------------------------------------

def init_draft_statusbar_scale():
    """
    this function initializes draft statusbar scale widget
    """
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

    mw = Gui.getMainWindow()
    sb = mw.statusBar()

    scale_widget = QtGui.QToolBar()
    scale_widget.setObjectName("draft_scale_widget")

    # get scales list according to system units
    draft_scales = get_scales()

    # get draft annotation scale
    draft_annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)

    # initializes scale widget
    scale_widget.draft_scales = draft_scales
    scaleLabel = QtGui.QPushButton("Scale")
    scaleLabel.setObjectName("ScaleLabel")
    scaleLabel.setFlat(True)
    menu = QtGui.QMenu(scaleLabel)
    gUnits = QtGui.QActionGroup(menu)
    for u in draft_scales:
        a = QtGui.QAction(gUnits)
        a.setText(u)
        menu.addAction(a)
    scaleLabel.setMenu(menu)
    gUnits.triggered.connect(_set_scale)
    scale_label = scale_to_label(draft_annotation_scale)
    scaleLabel.setText(scale_label)
    scaleLabel.setToolTip(translate("draft",
                                    "Set the scale used by draft annotation tools"))
    scale_widget.addWidget(scaleLabel)
    scale_widget.scaleLabel = scaleLabel

    # add scale widget to the statusbar
    sb.insertPermanentWidget(3, scale_widget)
    scale_widget.show()


def init_draft_statusbar_snap():
    """
    this function initializes draft statusbar snap widget
    """

    def _spacer():
        """
        empty label instead of snap_widget.addSeparator()
        """
        label = QtGui.QLabel()
        label.setFixedWidth(2)
        return label

    mw = Gui.getMainWindow()
    sb = mw.statusBar()

    # check if the toolbar is available, without it the required actions
    # may be missing:
    if mw.findChild(QtGui.QToolBar, "Draft snap") is None:
        return

    # snap widget:
    snap_widget = QtGui.QToolBar()
    snap_widget.setObjectName("draft_snap_widget")
    snap_widget.setOrientation(QtCore.Qt.Orientation.Horizontal)
    snap_widget.setIconSize(QtCore.QSize(16, 16))
    sb.insertPermanentWidget(2, snap_widget)

    # grid button:
    snap_widget.addAction(Gui.Command.get("Draft_ToggleGrid").getAction()[0])

    snap_widget.addWidget(_spacer())

    # lock button:
    snap_widget.addAction(Gui.Command.get("Draft_Snap_Lock").getAction()[0])
    snap_action = snap_widget.children()[-1]
    snap_action.setFixedWidth(40) # Widen the button.

    snap_widget.addWidget(_spacer())

    # dimension button:
    snap_widget.addAction(Gui.Command.get("Draft_Snap_Dimensions").getAction()[0])

    snap_widget.addWidget(_spacer())

    # ortho button:
    snap_widget.addAction(Gui.Command.get("Draft_Snap_Ortho").getAction()[0])

    snap_widget.addWidget(_spacer())

    # working plane button:
    snap_widget.addAction(Gui.Command.get("Draft_Snap_WorkingPlane").getAction()[0])

    # menu for lock button:
    for cmd in get_draft_snap_commands():
        if cmd not in ["Separator",
                       "Draft_ToggleGrid",
                       "Draft_Snap_Lock", # Is automatically added to the menu anyway.
                       "Draft_Snap_Dimensions",
                       "Draft_Snap_Ortho",
                       "Draft_Snap_WorkingPlane"]:
            snap_action.addAction(Gui.Command.get(cmd).getAction()[0])


def show_draft_statusbar():
    """
    shows draft statusbar if present or initializes it
    """
    params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

    mw = Gui.getMainWindow()
    sb = mw.statusBar()

    if params.GetBool("DisplayStatusbarScaleWidget", True):
        scale_widget = sb.findChild(QtGui.QToolBar, "draft_scale_widget")
        if scale_widget:
            scale_widget.show()
        else:
            scale_widget = mw.findChild(QtGui.QToolBar, "draft_scale_widget")
            if scale_widget:
                sb.insertPermanentWidget(3, scale_widget)
                scale_widget.show()
            else:
                t = QtCore.QTimer()
                t.singleShot(500, init_draft_statusbar_scale)

    if params.GetBool("DisplayStatusbarSnapWidget", True):
        snap_widget = sb.findChild(QtGui.QToolBar, "draft_snap_widget")
        if snap_widget:
            snap_widget.setOrientation(QtCore.Qt.Orientation.Horizontal)
            snap_widget.show()
        else:
            snap_widget = mw.findChild(QtGui.QToolBar, "draft_snap_widget")
            if snap_widget:
                sb.insertPermanentWidget(2, snap_widget)
                snap_widget.setOrientation(QtCore.Qt.Orientation.Horizontal)
                snap_widget.show()
            else:
                t = QtCore.QTimer()
                t.singleShot(500, init_draft_statusbar_snap)


def hide_draft_statusbar():
    """
    hides draft statusbar if present
    """
    mw = Gui.getMainWindow()
    sb = mw.statusBar()

    # hide scale widget
    scale_widget = sb.findChild(QtGui.QToolBar, "draft_scale_widget")
    if scale_widget is None:
        # when switching workbenches, the toolbar sometimes "jumps"
        # out of the status bar to any other dock area...
        scale_widget = mw.findChild(QtGui.QToolBar, "draft_scale_widget")
    if scale_widget:
        scale_widget.hide()
        # prevent the widget from showing up as a blank item in the toolbar
        # area context menu after switching to a different workbench:
        scale_widget.toggleViewAction().setVisible(False)

    # hide snap widget
    snap_widget = sb.findChild(QtGui.QToolBar,"draft_snap_widget")
    if snap_widget is None:
        # when switching workbenches, the toolbar sometimes "jumps"
        # out of the status bar to any other dock area...
        snap_widget = mw.findChild(QtGui.QToolBar,"draft_snap_widget")
    if snap_widget:
        snap_widget.hide()
        # prevent the widget from showing up as a blank item in the toolbar
        # area context menu after switching to a different workbench:
        snap_widget.toggleViewAction().setVisible(False)

## @}
