# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCADGui
import FreeCAD
import Path
import Path.Main.Gui.JobCmd as PathJobCmd
import Path.Main.Job as PathJob
import Path.Tool.Controller as PathToolController
import PathScripts.PathUtils as PathUtils
from PySide.QtGui import QInputDialog

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class PathUtilsUserInput(object):
    def selectedToolController(self):
        tc = None
        # check if a user has selected a tool controller in the tree.
        # Return the first one and remove all from selection
        for sel in FreeCADGui.Selection.getSelectionEx():
            if hasattr(sel.Object, "Proxy"):
                if isinstance(sel.Object.Proxy, PathToolController.ToolController):
                    if tc is None:
                        tc = sel.Object
                    FreeCADGui.Selection.removeSelection(sel.Object)
        return tc

    def chooseToolController(self, controllers):
        form = FreeCADGui.PySideUic.loadUi(":/panels/DlgTCChooser.ui")
        mylist = [i.Label for i in controllers]
        form.uiToolController.addItems(mylist)
        r = form.exec_()
        if not r:
            return None
        return [i for i in controllers if i.Label == form.uiToolController.currentText()][0]

    def chooseJob(self, jobs):
        selected = FreeCADGui.Selection.getSelection()

        jbs = [
            sel
            for sel in selected
            if hasattr(sel, "Proxy") and isinstance(sel.Proxy, PathJob.ObjectJob)
        ]
        if len(jbs) == 1:  # selection contains one Job object
            return jbs[0]

        if len(selected) == 1:  # selection contains only one object
            found = PathUtils.findParentJob(selected[0])
            if found:
                return found

        jbs = [j for j in jobs if all(o in j.Model.Group for o in selected)]
        if len(jbs) == 1:  # all selected objects inside Model group of one Job
            return jbs[0]

        jbs = [j for j in jobs if all(o in j.Proxy.baseObjects(j) for o in selected)]
        if len(jbs) == 1:  # all selected objects has a clone inside one Job
            return jobs[0]

        jobs = jbs or jobs
        labels = [j.Label for j in jobs]
        label, ok = QInputDialog.getItem(None, translate("Path", "Choose a CAM Job"), None, labels)
        if ok:  # return Job selected in dialog
            index = labels.index(label)
            return jobs[index]

        return None

    def createJob(self):
        return PathJobCmd.CommandJobCreate().Activated()


PathUtils.UserInput = PathUtilsUserInput()
