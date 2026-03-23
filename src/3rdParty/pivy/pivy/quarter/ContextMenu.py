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

from pivy.qt import QtCore
from pivy.qt.QtCore import QObject
from pivy.qt.QtGui import QMouseEvent
from pivy.qt.QtWidgets import QMenu

try:
    from pivy.qt.QtGui import QActionGroup, QAction
except ImportError:
    from pivy.qt.QtWidgets import QActionGroup, QAction

from pivy import coin
from pivy.coin import SoEventManager, SoScXMLStateMachine, SoRenderManager, SoGLRenderAction

from .eventhandlers import EventManager


class ContextMenu(QObject):
    def __init__(self, quarterwidget):
        QObject.__init__(self, quarterwidget)
        #QObject.__init__(quarterwidget)

        self._quarterwidget = quarterwidget
        self._rendermanager = self._quarterwidget.getSoRenderManager()

        self.contextmenu = QMenu()
        self.functionsmenu = QMenu("Functions")
        self.rendermenu = QMenu("Render Mode")
        self.stereomenu = QMenu("Stereo Mode")
        self.transparencymenu = QMenu("Transparency Type")

        self.functionsgroup = QActionGroup(self)
        self.stereomodegroup = QActionGroup(self)
        self.rendermodegroup = QActionGroup(self)
        self.transparencytypegroup = QActionGroup(self)

        self.rendermodes = []
        self.rendermodes.append((SoRenderManager.AS_IS, "as is"))
        self.rendermodes.append((SoRenderManager.WIREFRAME, "wireframe"))
        self.rendermodes.append((SoRenderManager.WIREFRAME_OVERLAY, "wireframe overlay"))
        self.rendermodes.append((SoRenderManager.POINTS, "points"))
        self.rendermodes.append((SoRenderManager.HIDDEN_LINE, "hidden line"))
        self.rendermodes.append((SoRenderManager.BOUNDING_BOX, "bounding box"))

        self.stereomodes = []
        self.stereomodes.append((SoRenderManager.MONO, "mono"))
        self.stereomodes.append((SoRenderManager.ANAGLYPH, "anaglyph"))
        self.stereomodes.append((SoRenderManager.QUAD_BUFFER, "quad buffer"))
        self.stereomodes.append((SoRenderManager.INTERLEAVED_ROWS, "interleaved rows"))
        self.stereomodes.append((SoRenderManager.INTERLEAVED_COLUMNS, "interleaved columns"))

        self.transparencytypes = []
        self.transparencytypes.append((SoGLRenderAction.NONE, "none"))
        self.transparencytypes.append((SoGLRenderAction.SCREEN_DOOR, "screen door"))
        self.transparencytypes.append((SoGLRenderAction.ADD, "add"))
        self.transparencytypes.append((SoGLRenderAction.DELAYED_ADD, "delayed add"))
        self.transparencytypes.append((SoGLRenderAction.SORTED_OBJECT_ADD, "sorted object add"))
        self.transparencytypes.append((SoGLRenderAction.BLEND, "blend"))
        self.transparencytypes.append((SoGLRenderAction.DELAYED_BLEND, "delayed blend"))
        self.transparencytypes.append((SoGLRenderAction.SORTED_OBJECT_BLEND, "sorted object blend"))
        self.transparencytypes.append((SoGLRenderAction.SORTED_OBJECT_SORTED_TRIANGLE_ADD, "sorted object sorted triangle add"))
        self.transparencytypes.append((SoGLRenderAction.SORTED_OBJECT_SORTED_TRIANGLE_BLEND, "sorted object sorted triangle blend"))
        self.transparencytypes.append((SoGLRenderAction.SORTED_LAYERS_BLEND, "sorted layers blend"))

        self.rendermodeactions = []
        for first, second in self.rendermodes:
            action = QAction(second, self)
            action.setCheckable(True)
            action.setChecked(self._rendermanager.getRenderMode() == first)
            action.setData(first)
            self.rendermodeactions.append(action)
            self.rendermodegroup.addAction(action)
            self.rendermenu.addAction(action)

        self.stereomodeactions = []
        for first, second in self.stereomodes:
            action = QAction(second, self)
            action.setCheckable(True)
            action.setChecked(self._rendermanager.getStereoMode() == first)
            action.setData(first)
            self.stereomodeactions.append(action)
            self.stereomodegroup.addAction(action)
            self.stereomenu.addAction(action)

        self.transparencytypeactions = []
        for first, second in self.transparencytypes:
            action = QAction(second, self)
            action.setCheckable(True)
            action.setChecked(self._rendermanager.getGLRenderAction().getTransparencyType() == first)
            action.setData(first)
            self.transparencytypeactions.append(action)
            self.transparencytypegroup.addAction(action)
            self.transparencymenu.addAction(action)

        viewall = QAction("View All", self)
        seek = QAction("Seek", self)
        self.functionsmenu.addAction(viewall)
        self.functionsmenu.addAction(seek)

        self.connect(seek, QtCore.SIGNAL("triggered(bool)"), self.seek)

        self.connect(viewall, QtCore.SIGNAL("triggered(bool)"), self.viewAll)

        self.connect(self.rendermodegroup, QtCore.SIGNAL("triggered(QAction *)"), self.changeRenderMode)

        self.connect(self.stereomodegroup, QtCore.SIGNAL("triggered(QAction *)"), self.changeStereoMode)

        self.connect(self.transparencytypegroup, QtCore.SIGNAL("triggered(QAction *)"), self.changeTransparencyType)

        self.contextmenu.addMenu(self.functionsmenu)
        self.contextmenu.addMenu(self.rendermenu)
        self.contextmenu.addMenu(self.stereomenu)
        self.contextmenu.addMenu(self.transparencymenu)

    def __del__(self):
        del self.functionsmenu
        del self.rendermenu
        del self.stereomenu
        del self.transparencymenu
        del self.contextmenu

    def getMenu(self):
        return self.contextmenu

    def exec_(self, pos):
        self._processEvent("sim.coin3d.coin.PopupMenuOpen")
        self.contextmenu.exec_(pos)

    def seek(self, checked):
        self._processEvent("sim.coin3d.coin.navigation.Seek")

    def viewAll(self, checked):
        self._processEvent("sim.coin3d.coin.navigation.ViewAll")

    def _processEvent(self, eventname):
        eventmanager = self._quarterwidget.getSoEventManager()
        for c in range(eventmanager.getNumSoScXMLStateMachines()):
            sostatemachine = eventmanager.getSoScXMLStateMachine(c)
            sostatemachine.queueEvent(coin.SbName(eventname))
            sostatemachine.processEventQueue()

    def changeRenderMode(self, action):
        try:
            self._rendermanager.setRenderMode(action.data().toInt()[0])
        except AttributeError:
            self._rendermanager.setRenderMode(action.data())

    def changeStereoMode(self, action):
        try:
            self._rendermanager.setStereoMode(action.data().toInt()[0])
        except AttributeError:
            self._rendermanager.setStereoMode(action.data())

    def changeTransparencyType(self, action):
        try:
            self._quarterwidget.setTransparencyType(action.data().toInt()[0])
        except AttributeError:
            self._quarterwidget.setTransparencyType(action.data())
