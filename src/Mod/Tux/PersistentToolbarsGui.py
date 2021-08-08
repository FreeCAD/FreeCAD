# Persistent toolbars for FreeCAD
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

"""Persistent toolbars for FreeCAD."""

import operator
import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore
from PySide import QtGui

conectedToolbars = []
timer = QtCore.QTimer()
mw = Gui.getMainWindow()
_saving_params = False
_observer = None

class Observer:
    def __init__(self):
        pUser = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
        pUser.AttachManager(self)
        timer.setSingleShot(True)
        timer.timeout.disconnect()
        timer.timeout.connect(onWorkbenchActivated)

    def slotParamChanged(self, _param, tp, name, value):
        if not _saving_params and tp and name and value:
            timer.start(100)


def pythonToolbars():
    """Manage Python based toolbars in Arch and Draft workbench."""

    active = Gui.activeWorkbench().__class__.__name__

    if active == "DraftWorkbench" or active == "ArchWorkbench" or active == "BIMWorkbench":
        if hasattr(Gui, "draftToolBar"):
            try:
                Gui.draftToolBar.Activated()
            except Exception:
                m = "Persistent toolbars: draftToolBar toolbar not managed.\n"
                App.Console.PrintMessage(m)
        if hasattr(Gui, "Snapper"):
            try:
                Gui.Snapper.show()
            except Exception:
                m = "Persistent toolbars: Snapper toolbar not managed.\n"
                App.Console.PrintMessage(m)
    else:
        pass


def isConnected(i):
    """Connect toolbar to onSave function."""

    if i not in conectedToolbars:
        conectedToolbars.append(i)
        i.topLevelChanged.connect(onSave)
    else:
        pass

def getGlobalToolbars():
    pGlobal = App.ParamGet("User parameter:BaseApp/Workbench/Global/Toolbar")
    globaltb = ["File", "Workbench", "Macro", "View", "Structure"]
    for group in pGlobal.GetGroups():
        globaltb.append(group.GetString("Name"))
    return globaltb

def onRestore(active):
    """Restore current workbench toolbars position."""

    toolbars = {}
    tb = mw.findChildren(QtGui.QToolBar)
    pUser = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
    pSystem = App.ParamGet("User parameter:Tux/PersistentToolbars/System")
    globaltb = getGlobalToolbars()

    for i in tb:

        isConnected(i)

        if i.objectName() and i.toggleViewAction().isVisible() and not i.isFloating():
            toolbars[i.objectName()] = i
        else:
            pass

    if pUser.GetGroup(active).GetBool("Saved"):
        group = pUser.GetGroup(active)
    elif pSystem.GetGroup(active).GetBool("Saved"):
        group = pSystem.GetGroup(active)
    else:
        group = None

    if group:
        topRestore = [ t for t in group.GetString("Top").split(",") if t not in globaltb ]
        leftRestore = [ t for t in group.GetString("Left").split(",") if t not in globaltb ]
        rightRestore = [ t for t in group.GetString("Right").split(",") if t not in globaltb ]
        bottomRestore = [ t for t in group.GetString("Bottom").split(",") if t not in globaltb ]

        # Reduce flickering.
        for i in toolbars:
            if (i not in topRestore and
                    i not in leftRestore and
                    i not in rightRestore and
                    i not in bottomRestore):

                area = mw.toolBarArea(toolbars[i])

                if area == QtCore.Qt.ToolBarArea.LeftToolBarArea:
                    leftRestore.append(i)
                elif area == QtCore.Qt.ToolBarArea.RightToolBarArea:
                    rightRestore.append(i)
                elif area == QtCore.Qt.ToolBarArea.BottomToolBarArea:
                    bottomRestore.append(i)
                else:
                    topRestore.append(i)
            else:
                pass

        for i in topRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.TopToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.TopToolBarArea, toolbars[i])
            else:
                pass

        for i in leftRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.LeftToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.LeftToolBarArea, toolbars[i])
            else:
                pass

        for i in rightRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.RightToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.RightToolBarArea, toolbars[i])
            else:
                pass

        for i in bottomRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.BottomToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.BottomToolBarArea, toolbars[i])
            else:
                pass
    else:
        pass


def onSave():
    """Save current workbench toolbars position."""

    tb = mw.findChildren(QtGui.QToolBar)
    active = Gui.activeWorkbench().__class__.__name__
    p = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
    group = p.GetGroup(active)
    globaltb = getGlobalToolbars()

    top = []
    left = []
    right = []
    bottom = []

    _saving_params = True

    for i in tb:
        if not i.isFloating():

            area = mw.toolBarArea(i)

            x = i.geometry().x()
            y = i.geometry().y()

            if area == QtCore.Qt.ToolBarArea.TopToolBarArea:
                top.append([x, y, i])
            elif area == QtCore.Qt.ToolBarArea.LeftToolBarArea:
                left.append([x, y, i])
            elif area == QtCore.Qt.ToolBarArea.RightToolBarArea:
                right.append([-x, y, i])
            elif area == QtCore.Qt.ToolBarArea.BottomToolBarArea:
                bottom.append([x, -y, i])
            else:
                pass
        else:
            pass

    top = sorted(top, key=operator.itemgetter(1, 0))
    left = sorted(left, key=operator.itemgetter(0, 1))
    right = sorted(right, key=operator.itemgetter(0, 1))
    bottom = sorted(bottom, key=operator.itemgetter(1, 0))

    for info, name in [(top,"Top"), (bottom,"Bottom"), (left,"Left"), (right,"Right")]:
        saved = []
        for _,_,tb in info:
            if mw.toolBarBreak(tb):
                saved.append("Break")

            if not tb.toggleViewAction().isVisible():
                # Toolbar with its toggle view action hidden means it does not
                # belong to the active workbench. But we shall still save the
                # otherwise invisible toolbar (hence below).
                continue

            name = tb.objectName()
            if name:
                saved.append(name)

        group.SetString(name, ",".join(saved))

    group.SetBool("Saved", 1)
    _saving_params = False


def onWorkbenchActivated():
    """When workbench gets activated restore toolbar position."""

    active = Gui.activeWorkbench().__class__.__name__

    if active:
        pythonToolbars()
        onRestore(active)
    else:
        pass


def onStart():
    """Start persistent toolbars."""
    if mw.property("eventLoop"):
        start = False
        try:
            mw.mainWindowClosed
            mw.workbenchActivated
            start = True
        except AttributeError:
            pass
        if start:
            timer.stop()
            onWorkbenchActivated()
            mw.mainWindowClosed.connect(onClose)
            mw.workbenchActivated.connect(onWorkbenchActivated)
            global _observer
            if not _observer:
                _observer = Observer()


def onClose():
    """Remove system provided presets on FreeCAD close."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars")
    p.RemGroup("System")


timer.timeout.connect(onStart)
timer.start(500)
