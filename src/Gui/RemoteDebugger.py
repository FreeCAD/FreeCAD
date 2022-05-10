# -*- coding: utf-8 -*-
#/******************************************************************************
# *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>        *
# *                                                                            *
# *   This file is part of the FreeCAD CAx development system.                 *
# *                                                                            *
# *   This library is free software; you can redistribute it and/or            *
# *   modify it under the terms of the GNU Library General Public              *
# *   License as published by the Free Software Foundation; either             *
# *   version 2 of the License, or (at your option) any later version.         *
# *                                                                            *
# *   This library  is distributed in the hope that it will be useful,         *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Library General Public License for more details.                     *
# *                                                                            *
# *   You should have received a copy of the GNU Library General Public        *
# *   License along with this library; see the file COPYING.LIB. If not,       *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
# *   Suite 330, Boston, MA  02111-1307, USA                                   *
# *                                                                            *
# ******************************************************************************/


import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui

class RemoteDebugger():
    def __init__(self, parent=None):
        ui = App.getHomePath() + "Ext/freecad/gui/RemoteDebugger.ui"
        self.dialog = Gui.PySideUic.loadUi(ui)
        self.dialog.buttonBox.accepted.connect(self.accept)
        self.dialog.buttonBox.rejected.connect(self.reject)

    def accept(self):
        try:
            index = self.dialog.tabWidget.currentIndex()

            if index == 0: # winpdb
                passwd = self.dialog.lineEditPassword.text()

                import rpdb2
                rpdb2.start_embedded_debugger(passwd, timeout = 30)

            elif index == 1: # VS code
                address = self.dialog.lineEditAddress.text()
                port = self.dialog.spinBoxPort.value()
                redirect = self.dialog.checkRedirectOutput.isChecked()

                import ptvsd
                ptvsd.enable_attach(address=(address, port), redirect_output=redirect)
                ptvsd.wait_for_attach()
        except Exception as e:
            QtGui.QMessageBox.warning(self.dialog, "Failed to attach", str(e))

        self.dialog.accept()

    def reject(self):
        self.dialog.reject()

    def exec_(self):
        self.dialog.exec_()


def attachToRemoteDebugger():
    dlg = RemoteDebugger(Gui.getMainWindow())
    dlg.exec_()
