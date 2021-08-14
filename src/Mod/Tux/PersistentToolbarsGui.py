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

from collections import defaultdict
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
_mw_observer = None
_tb_areas = {"Top": QtCore.Qt.ToolBarArea.TopToolBarArea,
             "Bottom": QtCore.Qt.ToolBarArea.BottomToolBarArea,
             "Left": QtCore.Qt.ToolBarArea.LeftToolBarArea,
             "Right": QtCore.Qt.ToolBarArea.RightToolBarArea}
_area_names = {QtCore.Qt.ToolBarArea.TopToolBarArea:'Top',
               QtCore.Qt.ToolBarArea.BottomToolBarArea:'Bottom',
               QtCore.Qt.ToolBarArea.LeftToolBarArea:'Left',
               QtCore.Qt.ToolBarArea.RightToolBarArea:'Right'}

class Observer:
    def __init__(self):
        self.pUser = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
        self.pUser.AttachManager(self)
        self.timer = QtCore.QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(onWorkbenchActivated)

    def slotParamChanged(self, _param, tp, name, value):
        if not _saving_params and tp and name and value:
            App.Logger('tux').log('pending restore {}', name)
            timer.start(100)

class MainWindowStateObserver:
    def __init__(self):
        self.param = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
        self.param.AttachManager(self)
        self.timer = QtCore.QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(onWorkbenchActivated)

    def slotParamChanged(self, _param, _tp, name, _value):
        if name == "WindowStateRestored" or name == "DefaultToolBarArea":
            App.Logger('tux').log('pending restore {}', name)
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
        globaltb.append(pGlobal.GetGroup(group).GetString("Name"))
    return frozenset(globaltb)

def getToolbars(restore):
    """Return toolbars arranged according to the main window layout."""

    # Qt toolbar layout in main window is completely hidden from user code. The
    # order of toolbars is unknownable, because invisible toolbars can occupy
    # the same position with a different underlying layout position. Therefore,
    # We include only visible toolbar position to guess the layout.
    #
    # During restore, we shall only move toolbars that belongs to the current
    # active workbench, that is, we shall exclude those with hidden
    # toolbar.toggleViewAction(), and also exclude toolbars from global
    # workbench (which will be managed by QMainwindow.restoreState()).

    layout = defaultdict(list)
    sorting = {}
    toolbars = {}
    for tb in mw.findChildren(QtGui.QToolBar):
        if restore:
            isConnected(tb)

        name = tb.objectName()
        if not name or tb.isFloating():
            continue

        toolbars[name] = tb

        area = mw.toolBarArea(tb)
        if not tb.isVisible():
            continue

        x = tb.x()
        y = tb.y()

        l = sorting.setdefault(area, [])
        if area == QtCore.Qt.ToolBarArea.TopToolBarArea:
            l.append((y,x,tb))
        elif area == QtCore.Qt.ToolBarArea.LeftToolBarArea:
            l.append((x,y,tb))
        elif area == QtCore.Qt.ToolBarArea.RightToolBarArea:
            l.append((-x,y,tb))
        elif area == QtCore.Qt.ToolBarArea.BottomToolBarArea:
            l.append((-y,x,tb))

    logger = App.Logger('tux')
    for area, tbs in sorting.items():
        tbs.sort()
        lines = []
        r = None
        for t in tbs:
            if r is None or r != t[0]:
                lines.append([])
            r = t[0]
            lines[-1].append(t[2])

        logger.log('{} {}', 'restore' if restore else 'save',
                [[t.objectName() for t in line] for line in lines])
        layout[area] = lines

    return (toolbars, layout)


def onRestore(active, migrating=False):
    """Restore current workbench toolbars position."""

    pMain = App.ParamGet('User parameter:BaseApp/Preferences/MainWindow')
    def_area = pMain.GetString('DefaultToolBarArea', 'Top')
    saved_area = "Top"

    pUser = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
    pSystem = App.ParamGet("User parameter:Tux/PersistentToolbars/System")

    if pUser.GetGroup(active).GetBool("Saved"):
        group = pUser.GetGroup(active)
        saved_area = group.GetString('SavedDefaultArea', 'Top')
    elif pSystem.GetGroup(active).GetBool("Saved"):
        group = pSystem.GetGroup(active)
    else:
        if not migrating:
            onSave()
        return

    globaltb = getGlobalToolbars()
    toolbars, tbs = getToolbars(True)

    logger = App.Logger('tux')
    if migrating:
        logger.log('migrating {}', active)

    if saved_area != def_area:
        restored = tbs[def_area]
        for name, area in _tb_areas.items():
            if name == def_area:
                continue
            lines = tbs[area]
            for i,line in enumerate(list(lines)):
                move = []
                for tb in line:
                    if tb not in globaltb:
                        move.append(tb)
                if not move:
                    continue
                if len(restored) <= i:
                    restored.append([])
                rline = restored[i]
                for tb in move:
                    line.remove(tb)
                    rline.append(tb)
                if not line:
                    lines.remove(line)

    for name, area in _tb_areas.items():
        lines = []
        curlines = tbs.get(area, [])
        restore = []

        # retrive the stored toolbars and break it into lines
        for n in group.GetString(name).split(","):
            if n == 'Break':
                if lines and lines[-1]:
                    lines.append([])
                continue
            tb = toolbars.get(n, None)
            if tb and tb.isVisible():
                if not lines:
                    lines.append([])
                lines[-1].append(tb)
                restore.append(tb)

        # now restore line by line
        for r, line in enumerate(lines):
            cur = curlines[r] if len(curlines) > r else []
            # append any new non-global toolbars at the end of the line
            for tb in cur:
                if tb.objectName() not in globaltb and tb not in restore:
                    line.append(tb)

            # Find a toolbar in the current line for insertion. We shall avoid
            # adding new lines as much as possible.
            tail = None
            for tb in reversed(cur):
                if tb in line:
                    # If there is a toolbar from the end in the current line
                    # that is also in the restored line, it is good for use as
                    # an insertion point.
                    tail = tb
                    break
                # otherwise, opt for the first global toolbar from the end of
                # the current line
                if tb.objectName() in globaltb:
                    if not tail:
                        tail = tb

            logger.log('current {}{} {}', name, r, [t.objectName() for t in cur])
            logger.log('restore {}{} {}', name, r, [t.objectName() for t in line])
            logger.log('tail {}{} {}', name, r, tail.objectName() if tail else None)
            revert = True
            # start actual restoring, from the end of the line
            for tb in reversed(line):
                if tb.objectName() in globaltb:
                    # Do not try to move global toolbar. But if this recorded
                    # global toolbar is actual in the current line, use it for
                    # the next insertion point.
                    if tb in cur:
                        tail = tb
                        revert = False
                    continue

                if tail is None:
                    # if no insertion point can be found, start a new line
                    logger.log('{}{}: add tail "{}"', name, r, tb.objectName())
                    mw.addToolBar(area, tb)
                    mw.insertToolBarBreak(tb)
                elif tail != tb:
                    logger.log('{}{}: add "{}" before "{}"',
                            name, r, tb.objectName(), tail.objectName())
                    # Insert the toolbar. Because QMainWindow only allow to
                    # insert before a given toolbar, we may need to immediately
                    # revert the position to get the correct order
                    mw.insertToolBar(tail, tb)
                    if revert:
                        mw.insertToolBar(tb, tail)
                        logger.log('{}{}: revert', name, r)

                # After adding/inserting the toolbar, use it for the next
                # insertion point. And we don't need to revert any more.
                revert = False
                tail = tb

    if def_area != saved_area:
        breaks = []
        darea = _area_names[def_area]
        for tb in toolbars.values():
            area = mw.toolBarArea()
            if tb in globaltb or tb.isVisible() or area == darea:
                continue
            if mw.toolBarBreak(tb):
                breaks.append(tb)
            mw.addToolBar(darea, tb)
        for tb in breaks:
            mw.insertToolBarBreak(tb)

def onSave():
    """Save current workbench toolbars position."""

    active = Gui.activeWorkbench().__class__.__name__
    p = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
    group = p.GetGroup(active)

    _, tbs = getToolbars(False)

    global _saving_params
    _saving_params = True
    for name, area in _tb_areas.items():
        saved = []
        for line in tbs[area]:
            if saved:
                saved.append("Break")
            for tb in line:
                saved.append(tb.objectName())

        group.SetString(name, ",".join(saved))
    group.SetBool("Saved", 1)
    pMain = App.ParamGet('User parameter:BaseApp/Preferences/MainWindow')
    def_area = pMain.GetString('DefaultToolBarArea', 'Top')
    group.SetString('SavedDefaultArea', def_area)
    _saving_params = False

def onWorkbenchActivated():
    """When workbench gets activated restore toolbar position."""

    active = Gui.activeWorkbench().__class__.__name__

    if active:
        pythonToolbars()
        try:
            Gui.getMainWindow().setUpdatesEnabled(False)
            onRestore(active)
        finally:
            Gui.getMainWindow().setUpdatesEnabled(True)
    else:
        pass

def migrate():
    param = App.ParamGet('User parameter:Tux/PersistentToolbars')
    d = param.GetInt('Deprecated', 1)
    if d == 0:
        return False
    if d != 1:
        return True
    param.SetInt('Deprecated', 2)

    if param.HasGroup('User'):
        param = param.GetGroup('User')
    elif param.HasGroup('System'):
        param = param.GetGroup('System')
    else:
        return True
    for name in param.GetGroups():
        onRestore(name, True);
    return True

def onStart():
    """Start persistent toolbars."""
    if mw.property("eventLoop"):
        timer.stop()

        if migrate():
            return
        try:
            mw.mainWindowClosed.connect(onClose)
            mw.workbenchActivated.connect(onWorkbenchActivated)
        except AttributeError:
            return
        onWorkbenchActivated()
        global _observer
        if not _observer:
            _observer = Observer()
        global _mw_observer
        if not _mw_observer:
            _mw_observer = MainWindowStateObserver()

def onClose():
    """Remove system provided presets on FreeCAD close."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars")
    p.RemGroup("System")


timer.timeout.connect(onStart)
timer.start(500)
