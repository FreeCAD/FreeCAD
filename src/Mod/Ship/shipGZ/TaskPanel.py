#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import math
import FreeCAD as App
import FreeCADGui as Gui
import Units
from PySide import QtGui, QtCore
import PlotAux
from shipUtils import Paths
import shipUtils.Units as USys
import shipUtils.Locale as Locale


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipGZ/TaskPanel.ui"

    def accept(self):
        return True

    def reject(self):
        self.preview.clean()
        return True

    def clicked(self, index):
        pass

    def open(self):
        pass

    def needsFullSpace(self):
        return True

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return False

    def helpRequested(self):
        pass

    def setupUi(self):
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")

        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NPoints")
        form.var_draft = self.widget(QtGui.QCheckBox, "VariableDraft")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()

    def getMainWindow(self):
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise RuntimeError("No main window found")

    def widget(self, class_id, name):
        """Return the selected widget.

        Keyword arguments:
        class_id -- Class identifier
        name -- Name of the widget
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        return form.findChild(class_id, name)

    def initValues(self):
        """ Set initial values for fields
        """
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.setWindowTitle(QtGui.QApplication.translate(
            "ship_gz",
            "Plot the GZ curve",
            None,
            QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "AngleLabel").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Angle",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "NPointsLabel").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Number of points",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableDraft").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Variable draft",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableDraft").setTooltip(
            QtGui.QApplication.translate(
                "ship_gz",
                "The ship will be moved to the equilibrium draft for each" + \
                " roll angle. It will significantly increase the required" + \
                " computing time",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableTrim").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Variable trim",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableTrim").setTooltip(
            QtGui.QApplication.translate(
                "ship_gz",
                "The ship will be rotated to the equilibrium trim angle for" + \
                " each roll angle. It will significantly increase the" + \
                " required computing time",
                None,
                QtGui.QApplication.UnicodeUTF8))



def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel