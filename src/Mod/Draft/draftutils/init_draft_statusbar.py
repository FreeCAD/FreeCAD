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
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

from draftutils.init_tools import get_draft_snap_commands

#----------------------------------------------------------------------------
# SCALE WIDGET FUNCTIONS
#----------------------------------------------------------------------------

draft_scales_metrics =  ["1:1000", "1:500", "1:250", "1:200", "1:100",
                         "1:50", "1:25","1:20", "1:10", "1:5","1:2",
                         "1:1",
                         "2:1", "5:1", "10:1", "20:1",
                         QT_TRANSLATE_NOOP("draft","custom"),
                        ]

draft_scales_arch_imperial =  ["1/16in=1ft", "3/32in=1ft", "1/8in=1ft",
                               "3/16in=1ft", "1/4in=1ft","3/8in=1ft",
                               "1/2in=1ft", "3/4in=1ft", "1in=1ft",
                               "1.5in=1ft", "3in=1ft",
                               QT_TRANSLATE_NOOP("draft","custom"),
                              ]

draft_scales_eng_imperial =  ["1in=10ft", "1in=20ft", "1in=30ft",
                              "1in=40ft", "1in=50ft", "1in=60ft",
                              "1in=70ft", "1in=80ft", "1in=90ft",
                              "1in=100ft",
                              QT_TRANSLATE_NOOP("draft","custom"),
                             ]

def get_scales(unit_system = 0):
    """
    returns the list of preset scales accordin to unit system.

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
    f = 1/scale
    f = round(f,2)
    f = f.as_integer_ratio()
    if f[1] == 1 or f[0] == 1:
        label = str(f[1]) + ":" + str(f[0])
        return label
    else:
        return str(scale)

def label_to_scale(label):
    """
    transform a scale string into scale factor as float
    """
    try :
        scale = float(label)
        return scale
    except :
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
            except:
                err = QT_TRANSLATE_NOOP("draft",
                                        "Unable to convert input into a "
                                        "scale factor")
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
    scale_widget = sb.findChild(QtGui.QToolBar,"draft_status_scale_widget")
    if action.text() == QT_TRANSLATE_NOOP("draft","custom"):
        dialog_text = QT_TRANSLATE_NOOP("draft",
                                        "Set custom annotation scale in "
                                        "format x:x, x=x"
                                       )
        custom_scale = QtGui.QInputDialog.getText(None, "Set custom scale",
                                                  dialog_text)
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
    if mw:
        sb = mw.statusBar()
        if sb is None:
            return
    else:
        return

    scale_widget = QtGui.QToolBar()
    scale_widget.setObjectName("draft_status_scale_widget")

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
    tooltip = "Set the scale used by draft annotation tools"
    scaleLabel.setToolTip(QT_TRANSLATE_NOOP("draft",tooltip))
    scale_widget.addWidget(scaleLabel)
    scale_widget.scaleLabel = scaleLabel

    # add scale widget to the statusbar
    sb.insertPermanentWidget(3, scale_widget)
    scale_widget.show()


def init_draft_statusbar_snap():
    """
    this function initializes draft statusbar snap widget
    """
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

    mw = Gui.getMainWindow()
    if mw:
        sb = mw.statusBar()
        if sb is None:
            return
    else:
        return

    # SNAP WIDGET - init ----------------------------------------------------

    snap_widget = QtGui.QToolBar()
    snap_widget.setObjectName("draft_snap_widget")
    snap_widget.setIconSize(QtCore.QSize(16,16))

    # GRID BUTTON - init
    gridbutton = QtGui.QPushButton(snap_widget)
    gridbutton.setIcon(QtGui.QIcon.fromTheme("Draft",
                                             QtGui.QIcon(":/icons/"
                                                         "Draft_Grid.svg")))
    gridbutton.setToolTip(QT_TRANSLATE_NOOP("Draft",
                                            "Toggles Grid On/Off"))
    gridbutton.setObjectName("Grid_Statusbutton")
    gridbutton.setWhatsThis("Draft_ToggleGrid")
    gridbutton.setFlat(True)
    QtCore.QObject.connect(gridbutton,QtCore.SIGNAL("clicked()"),
                           lambda f=Gui.doCommand,
                           arg='Gui.runCommand("Draft_ToggleGrid")':f(arg))
    snap_widget.addWidget(gridbutton)

    # SNAP BUTTON - init
    snappref = param.GetString("snapModes","111111111101111")[0]
    snapbutton = QtGui.QPushButton(snap_widget)
    snapbutton.setIcon(QtGui.QIcon.fromTheme("Draft",
                                              QtGui.QIcon(":/icons/"
                                                          "Snap_Lock.svg")))
    snapbutton.setObjectName("Snap_Statusbutton")
    snapbutton.setWhatsThis("Draft_ToggleLockSnap")
    snapbutton.setToolTip(QT_TRANSLATE_NOOP("Draft",
                                          "Object snapping"))
    snapbutton.setCheckable(True)
    snapbutton.setChecked(bool(int(snappref)))
    snapbutton.setFlat(True)

    snaps_menu = QtGui.QMenu(snapbutton)
    snaps_menu.setObjectName("draft_statusbar_snap_toolbar")

    snap_gui_commands = get_draft_snap_commands()
    if 'Draft_Snap_Ortho' in snap_gui_commands:
        snap_gui_commands.remove('Draft_Snap_Ortho')
    if 'Draft_Snap_WorkingPlane' in snap_gui_commands:
        snap_gui_commands.remove('Draft_Snap_WorkingPlane')
    if 'Draft_Snap_Dimensions' in snap_gui_commands:
        snap_gui_commands.remove('Draft_Snap_Dimensions')
    if 'Draft_ToggleGrid' in snap_gui_commands:
        snap_gui_commands.remove('Draft_ToggleGrid')

    Gui.Snapper.init_draft_snap_buttons(snap_gui_commands,snaps_menu, "_Statusbutton")
    Gui.Snapper.restore_snap_buttons_state(snaps_menu, "_Statusbutton")

    snapbutton.setMenu(snaps_menu)
    snap_widget.addWidget(snapbutton)


    # DIMENSION BUTTON - init
    dimpref = param.GetString("snapModes","111111111101111")[13]
    dimbutton = QtGui.QPushButton(snap_widget)
    dimbutton.setIcon(QtGui.QIcon.fromTheme("Draft",
                                            QtGui.QIcon(":/icons/"
                                                        "Snap_Dimensions.svg")))
    dimbutton.setToolTip(QT_TRANSLATE_NOOP("Draft",
                                           "Toggles Visual Aid Dimensions On/Off"))
    dimbutton.setObjectName("Draft_Snap_Dimensions_Statusbutton")
    dimbutton.setWhatsThis("Draft_ToggleDimensions")
    dimbutton.setFlat(True)
    dimbutton.setCheckable(True)
    dimbutton.setChecked(bool(int(dimpref)))
    QtCore.QObject.connect(dimbutton,QtCore.SIGNAL("clicked()"),
                           lambda f=Gui.doCommand,
                           arg='Gui.runCommand("Draft_Snap_Dimensions")':f(arg))
    snap_widget.addWidget(dimbutton)

    # ORTHO BUTTON - init
    ortopref = param.GetString("snapModes","111111111101111")[10]
    orthobutton = QtGui.QPushButton(snap_widget)
    orthobutton.setIcon(QtGui.QIcon.fromTheme("Draft",
                                              QtGui.QIcon(":/icons/"
                                                          "Snap_Ortho.svg")))
    orthobutton.setObjectName("Draft_Snap_Ortho"+"_Statusbutton")
    orthobutton.setWhatsThis("Draft_ToggleOrtho")
    orthobutton.setToolTip(QT_TRANSLATE_NOOP("Draft",
                                             "Toggles Ortho On/Off"))
    orthobutton.setFlat(True)
    orthobutton.setCheckable(True)
    orthobutton.setChecked(bool(int(ortopref)))
    QtCore.QObject.connect(orthobutton,QtCore.SIGNAL("clicked()"),
                           lambda f=Gui.doCommand,
                           arg='Gui.runCommand("Draft_Snap_Ortho")':f(arg))
    snap_widget.addWidget(orthobutton)

    # WORKINGPLANE BUTTON - init
    wppref = param.GetString("snapModes","111111111101111")[14]
    wpbutton = QtGui.QPushButton(snap_widget)
    wpbutton.setIcon(QtGui.QIcon.fromTheme("Draft",
                                              QtGui.QIcon(":/icons/"
                                                          "Snap_WorkingPlane.svg")))
    wpbutton.setObjectName("Draft_Snap_WorkingPlane_Statusbutton")
    wpbutton.setWhatsThis("Draft_ToggleWorkingPlaneSnap")
    wpbutton.setToolTip(QT_TRANSLATE_NOOP("Draft",
                                          "Toggles Constrain to Working Plane On/Off"))
    wpbutton.setFlat(True)
    wpbutton.setCheckable(True)
    wpbutton.setChecked(bool(int(wppref)))
    QtCore.QObject.connect(wpbutton,QtCore.SIGNAL("clicked()"),
                           lambda f=Gui.doCommand,
                           arg='Gui.runCommand("Draft_Snap_WorkingPlane")':f(arg))
    snap_widget.addWidget(wpbutton)

    # add snap widget to the statusbar
    sb.insertPermanentWidget(2, snap_widget)
    snap_widget.show()


def show_draft_statusbar():
    """
    shows draft statusbar if present or initializes it
    """
    params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    display_statusbar = params.GetBool("DisplayStatusbar", True)

    if not display_statusbar:
        return

    mw = Gui.getMainWindow()
    if mw:
        sb = mw.statusBar()

        scale_widget = sb.findChild(QtGui.QToolBar,
                                    "draft_status_scale_widget")
        if scale_widget:
            scale_widget.show()
        else:
            scale_widget = mw.findChild(QtGui.QToolBar,
                            "draft_status_scale_widget")
            if scale_widget:
                sb.insertPermanentWidget(3, scale_widget)
                scale_widget.show()
            elif params.GetBool("DisplayStatusbarScaleWidget", True):
                t = QtCore.QTimer()
                t.singleShot(500, init_draft_statusbar_scale)

        snap_widget = sb.findChild(QtGui.QToolBar,"draft_snap_widget")
        if snap_widget:
            snap_widget.show()
        else:
            snap_widget = mw.findChild(QtGui.QToolBar,"draft_snap_widget")
            if snap_widget:
                sb.insertPermanentWidget(2, snap_widget)
                snap_widget.show()
            elif params.GetBool("DisplayStatusbarSnapWidget", True):
                t = QtCore.QTimer()
                t.singleShot(500, init_draft_statusbar_snap)


def hide_draft_statusbar():
    """
    hides draft statusbar if present
    """
    mw = Gui.getMainWindow()
    if not mw:
        return
    sb = mw.statusBar()

    # hide scale widget
    scale_widget = sb.findChild(QtGui.QToolBar,
                                "draft_status_scale_widget")
    if scale_widget:
        scale_widget.hide()
    else:
        # when switching workbenches, the toolbar sometimes "jumps"
        # out of the status bar to any other dock area...
        scale_widget = mw.findChild(QtGui.QToolBar,
                                    "draft_status_scale_widget")
        if scale_widget:
            scale_widget.hide()

    # hide snap widget
    snap_widget = sb.findChild(QtGui.QToolBar,"draft_snap_widget")
    if snap_widget:
        snap_widget.hide()
    else:
        # when switching workbenches, the toolbar sometimes "jumps"
        # out of the status bar to any other dock area...
        snap_widget = mw.findChild(QtGui.QToolBar,"draft_snap_widget")
        if snap_widget:
            snap_widget.hide()

## @}
