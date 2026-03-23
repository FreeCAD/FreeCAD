#!/usr/bin/env python

###
# Copyright (c) 2002-2008 Kongsberg SIM
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

import os
import sys

from pivy.qt import QtCore, QtGui
from pivy.qt.QtGui import QMainWindow, QWorkspace, QAction, QFileDialog, QApplication

from pivy.coin import SoInput, SoDB
from pivy.quarter import QuarterWidget


class MdiQuarterWidget(QuarterWidget):
    def __init__(self, parent, sharewidget):
        QuarterWidget.__init__(self, parent=parent, sharewidget=sharewidget)

    def loadFile(self, filename):
        in_ = SoInput()
        if (in_.openFile(str(filename.toLatin1()))):
            root = SoDB.readAll(in_)
        if (root):
            self.setSceneGraph(root)
            self.currentfile = filename
            self.setWindowTitle(filename)
            return True
        return False

    def currentFile(self):
        return self.currentfile

    def minimumSizeHint(self):
        return QtCore.QSize(640, 480)


class MdiMainWindow(QMainWindow):
    def __init__(self, qApp):
        QMainWindow.__init__(self)
        self._firstwidget = None
        self._workspace = QWorkspace()
        self.setCentralWidget(self._workspace)
        self.setAcceptDrops(True)
        self.setWindowTitle("Pivy Quarter MDI example")

        filemenu = self.menuBar().addMenu("&File")
        windowmenu = self.menuBar().addMenu("&Windows")

        fileopenaction = QAction("&Open", self)
        fileexitaction = QAction("E&xit", self)
        tileaction = QAction("Tile", self)
        cascadeaction = QAction("Cascade", self)

        filemenu.addAction(fileopenaction)
        filemenu.addAction(fileexitaction)
        windowmenu.addAction(tileaction)
        windowmenu.addAction(cascadeaction)

        self.connect(fileopenaction, QtCore.SIGNAL("triggered()"), self.open)
        self.connect(fileexitaction, QtCore.SIGNAL("triggered()"), QtGui.qApp.closeAllWindows)
        self.connect(tileaction, QtCore.SIGNAL("triggered()"), self._workspace.tile)
        self.connect(cascadeaction, QtCore.SIGNAL("triggered()"), self._workspace.cascade)

        windowmapper = QtCore.QSignalMapper(self)
        self.connect(windowmapper, QtCore.SIGNAL("mapped(QWidget *)"), self._workspace.setActiveWindow)

        self.dirname = os.curdir        

    def dragEnterEvent(self, event):
        # just accept anything...
        event.acceptProposedAction()

    def dropEvent(self, event):
        mimedata = event.mimeData()
        if mimedata.hasUrls():
            path = mimedata.urls().takeFirst().path()
            self.open_path(path)

    def closeEvent(self, event):
        self._workspace.closeAllSubWindows()

    def open(self):
        self.open_path(QFileDialog.getOpenFileName(self, "", self.dirname))

    def open_path(self, filename):
        self.dirname = os.path.dirname(str(filename.toLatin1()))
        if not filename.isEmpty():
            existing = self.findMdiChild(filename)
            if existing:
                self._workspace.setActiveWindow(existing)
                return
        child = self.createMdiChild()
        if (child.loadFile(filename)):
            self.statusBar().showMessage("File loaded", 2000)
            child.show()
        else:
            child.close()

    def findMdiChild(self, filename):
        canonicalpath = QtCore.QFileInfo(filename).canonicalFilePath()
        for window in self._workspace.windowList():
            mdiwidget = window
            if mdiwidget.currentFile() == canonicalpath:
                return mdiwidget
        return 0;

    def createMdiChild(self):
        widget = MdiQuarterWidget(None, self._firstwidget)
        self._workspace.addSubWindow(widget)
        if not self._firstwidget:
            self._firstwidget = widget
        return widget


def main():
    app = QApplication(sys.argv)

    mdi = MdiMainWindow(app)    
    mdi.show()
    if len(sys.argv)==2:
        mdi.open_path(QtCore.QString(sys.argv[1]))
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
