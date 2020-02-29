"""Draft Statusbar commands.

This module provide the code for the Draft Statusbar, activated by initGui
"""
## @package init_tools
# \ingroup DRAFT
# \brief This module provides the code for the Draft Statusbar.

# ***************************************************************************
# *                                                                         *
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

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP


#----------------------------------------------------------------------------
# SCALE WIDGET FUNCTIONS
#----------------------------------------------------------------------------

def scale_to_label(scale):
    """
    transform a float number into a 1:X or X:1 scale and return it as label
    """
    f = scale.as_integer_ratio()
    if f[0] == 1 or f[0] == 1:
        label = str(f[0]) + ":" + str(f[1])
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
        err = QT_TRANSLATE_NOOP("draft",
                                "Unable to convert input into a scale factor")
        if ":" in label:
            f = label.split(":")
            try:
                scale = float(f[0])/float(f[1])
                return scale
            except:
                App.Console.PrintWarning(err)
                return None
        if "/" in label:
            f = label.split("/")
            try:
                scale = float(f[0])/float(f[1])
                return scale
            except:
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
    statuswidget = sb.findChild(QtGui.QToolBar,"draft_status_widget")
    if action.text() == QT_TRANSLATE_NOOP("draft","custom"):
        custom_scale = QtGui.QInputDialog.getText(None, "Custom scale", "")
        if custom_scale[1]:
            print(custom_scale[0])
            scale = label_to_scale(custom_scale[0])
            if scale is None: return
            param.SetFloat("DraftAnnotationScale", scale)
            cs = scale_to_label(scale)
            statuswidget.scaleLabel.setText(cs)
    else:
        text_scale = action.text()
        statuswidget.scaleLabel.setText(text_scale)
        scale = label_to_scale(text_scale)
        param.SetFloat("DraftAnnotationScale", scale)

#----------------------------------------------------------------------------
# MAIN DRAFT STATUSBAR FUNCTIONS
#----------------------------------------------------------------------------

def init_draft_statusbar(sb):
    """
    this function initializes draft statusbar
    """
    
    draft_scales =  ["1:1000", "1:500", "1:250", "1:200", "1:100", 
                     "1:50", "1:25","1:20", "1:10", "1:5","1:2",
                     "1:1",
                     "2:1", "5:1", "10:1", "20:1",                 
                     QT_TRANSLATE_NOOP("draft","custom"),                 
                    ]
    
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    draft_annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)
    
    statuswidget = QtGui.QToolBar()
    statuswidget.setObjectName("draft_status_widget")
   
    # SCALE TOOL -------------------------------------------------------------
    statuswidget.draft_scales = draft_scales
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
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    draft_annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)
    scale_label = scale_to_label(draft_annotation_scale)
    scaleLabel.setText(scale_label)
    tooltip = "Set the scale used by draft annotation tools"
    scaleLabel.setToolTip(QT_TRANSLATE_NOOP("draft",tooltip))
    statuswidget.addWidget(scaleLabel)
    statuswidget.scaleLabel = scaleLabel
    
    # ADD TOOLS TO STATUS BAR ------------------------------------------------
    sb.addPermanentWidget(statuswidget)
    statuswidget.show()

def show_draft_statusbar():
    """
    shows draft statusbar if present or initializes it
    """
    mw = Gui.getMainWindow()
    if mw:
        sb = mw.statusBar()
        statuswidget = sb.findChild(QtGui.QToolBar,"draft_status_widget")
        if statuswidget:
            statuswidget.show()
        else:
            init_draft_statusbar(sb)

def hide_draft_statusbar():
    """
    hides draft statusbar if present
    """
    mw = Gui.getMainWindow()
    if mw:
        sb = mw.statusBar()
        statuswidget = sb.findChild(QtGui.QToolBar,"draft_status_widget")
        if statuswidget:
            statuswidget.hide()
        else:
            # when switching workbenches, the toolbar sometimes "jumps"
            # out of the status bar to any other dock area...
            statuswidget = mw.findChild(QtGui.QToolBar,"draft_status_widget")
            if statuswidget:
                statuswidget.hide()