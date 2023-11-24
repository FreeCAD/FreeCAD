# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Contains a parameter observer class and several related functions."""

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui

from draftutils import init_draft_statusbar
from draftutils import utils
from draftutils.translate import translate
from draftviewproviders import view_base


class ParamObserver:

    def slotParamChanged(self, param, tp, name, value):
        if name in ("gridBorder", "gridShowHuman", "coloredGridAxes", "gridEvery",
                    "gridSpacing", "gridSize", "gridTransparency", "gridColor"):
            _param_observer_callback_grid()
            return
        if name == "SnapBarShowOnlyDuringCommands":
            _param_observer_callback_snapbar(value)
            return
        if name == "DisplayStatusbarSnapWidget":
            _param_observer_callback_snapwidget()
            return
        if name == "DisplayStatusbarScaleWidget":
            _param_observer_callback_scalewidget()
            return
        if name == "snapStyle":
            _param_observer_callback_snapstyle()
            return
        if name == "snapcolor":
            _param_observer_callback_snapcolor()
            return
        if name == "patternFile":
            _param_observer_callback_svg_pattern()
            return


def _param_observer_callback_grid():
    if hasattr(App, "draft_working_planes") and hasattr(Gui, "Snapper"):
        try:
            trackers = Gui.Snapper.trackers
            for wp in App.draft_working_planes[1]:
                view = wp._view
                if view in trackers[0]:
                    i = trackers[0].index(view)
                    grid = trackers[1][i]
                    grid.pts = []
                    grid.reset()
                    grid.displayHumanFigure(wp)
                    grid.setAxesColor(wp)
        except Exception:
            pass


def _param_observer_callback_snapbar(value):
    # value is a string: "0" or "1"
    if Gui.activeWorkbench().name() not in ("DraftWorkbench", "ArchWorkbench", "BIMWorkbench"):
        return
    if hasattr(Gui, "Snapper"):
        toolbar = Gui.Snapper.get_snap_toolbar()
        if toolbar is not None:
            toolbar.setVisible(value == "0")


def _param_observer_callback_snapwidget():
    if Gui.activeWorkbench().name() == "DraftWorkbench":
        init_draft_statusbar.hide_draft_statusbar()
        init_draft_statusbar.show_draft_statusbar()


def _param_observer_callback_scalewidget():
    if Gui.activeWorkbench().name() == "DraftWorkbench":
        init_draft_statusbar.hide_draft_statusbar()
        init_draft_statusbar.show_draft_statusbar()


def _param_observer_callback_snapstyle():
    if hasattr(Gui, "Snapper"):
        Gui.Snapper.set_snap_style()


def _param_observer_callback_snapcolor():
    if hasattr(Gui, "Snapper"):
        for snap_track in Gui.Snapper.trackers[2]:
            snap_track.setColor()


def _param_observer_callback_svg_pattern():
    utils.load_svg_patterns()
    if App.ActiveDocument is None:
        return

    pats = list(utils.svg_patterns())
    pats.sort()
    pats = ["None"] + pats

    data = []
    for doc in App.listDocuments().values():
        vobjs = []
        for obj in doc.Objects:
            if hasattr(obj, "ViewObject"):
                vobj = obj.ViewObject
                if hasattr(vobj, "Pattern") \
                        and hasattr(vobj, "Proxy") \
                        and isinstance(vobj.Proxy, view_base.ViewProviderDraft) \
                        and vobj.getEnumerationsOfProperty("Pattern") != pats:
                    vobjs.append(vobj)
        if vobjs:
            data.append([doc, vobjs])
    if not data:
        return

    msg = translate("draft",
"""Do you want to update the SVG pattern options
of existing objects in all opened documents?""")
    res = QtGui.QMessageBox.question(None, "Update SVG patterns", msg,
                                     QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                     QtGui.QMessageBox.No)
    if res == QtGui.QMessageBox.No:
        return

    for doc, vobjs in data:
        doc.openTransaction("SVG pattern update")
        for vobj in vobjs:
            old = vobj.Pattern
            if old in pats:
                vobj.Pattern = pats
            else:
                tmp_pats = [old] + pats[1:]
                tmp_pats.sort()
                vobj.Pattern = ["None"] + tmp_pats
            vobj.Pattern = old
        doc.commitTransaction()


def _param_observer_start(param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")):
    param.AttachManager(ParamObserver())
