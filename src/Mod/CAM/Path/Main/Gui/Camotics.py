# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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


from PySide.QtCore import QT_TRANSLATE_NOOP
from threading import Thread, Lock
import FreeCAD
import FreeCADGui
import Mesh
import Path
import PathScripts
import Path.Post.Command as PathPost
import camotics
import io
import json
import queue
import subprocess

from PySide import QtCore, QtGui

__title__ = "Camotics Simulator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Task panel for Camotics Simulation"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class CAMoticsUI:
    def __init__(self, simulation):
        # this will create a Qt widget from our ui file
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/TaskPathCamoticsSim.ui")
        self.simulation = simulation
        self.initializeUI()
        self.lock = False

    def initializeUI(self):
        self.form.timeSlider.sliderReleased.connect(
            lambda: self.simulation.execute(self.form.timeSlider.value())
        )
        self.form.progressBar.reset()
        self.form.timeSlider.setEnabled = False
        self.form.btnLaunchCamotics.clicked.connect(self.launchCamotics)
        self.form.btnMakeFile.clicked.connect(self.makeCamoticsFile)
        self.simulation.progressUpdate.connect(self.calculating)
        self.simulation.statusChange.connect(self.updateStatus)
        self.form.txtStatus.setText(translate("Path", "Drag Slider to Simulate"))

    def launchCamotics(self):
        filename = self.makeCamoticsFile()
        subprocess.Popen(["camotics", filename])

    def makeCamoticsFile(self):
        Path.Log.track()
        filename = QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("Path", "Save Project As"),
            "",
            translate("Path", "Camotics Project (*.camotics)"),
        )[0]
        if filename:
            if not filename.endswith(".camotics"):
                filename += ".camotics"

            text = self.simulation.buildproject()
            try:
                with open(filename, "w") as outputfile:
                    outputfile.write(text)
            except IOError:
                QtGui.QMessageBox.information(
                    self, translate("Path", "Unable to open file: {}".format(filename))
                )

        return filename

    def accept(self):
        self.simulation.accept()
        FreeCADGui.Control.closeDialog()

    def reject(self):
        self.simulation.cancel()
        if self.simulation.simmesh is not None:
            FreeCAD.ActiveDocument.removeObject(self.simulation.simmesh.Name)
        FreeCADGui.Control.closeDialog()

    def setRunTime(self, duration):
        self.form.timeSlider.setMinimum(0)
        self.form.timeSlider.setMaximum(duration)

    def calculating(self, progress=0.0):
        self.form.timeSlider.setEnabled = progress == 1.0
        self.form.progressBar.setValue(int(progress * 100))

    def updateStatus(self, status):
        self.form.txtStatus.setText(status)


class CamoticsSimulation(QtCore.QObject):

    SIM = camotics.Simulation()
    q = queue.Queue()
    progressUpdate = QtCore.Signal(object)
    statusChange = QtCore.Signal(object)
    simmesh = None
    filenames = []

    SHAPEMAP = {
        "ballend": "Ballnose",
        "endmill": "Cylindrical",
        "v-bit": "Conical",
        "chamfer": "Snubnose",
    }

    def worker(self, lock):
        while True:
            item = self.q.get()
            Path.Log.debug("worker processing: {}".format(item))
            with lock:
                if item["TYPE"] == "STATUS":
                    self.statusChange.emit(item["VALUE"])
                    if item["VALUE"] == "DONE":
                        self.SIM.wait()
                        surface = self.SIM.get_surface("binary")
                        self.SIM.wait()
                        self.addMesh(surface)
                elif item["TYPE"] == "PROGRESS":
                    self.progressUpdate.emit(item["VALUE"])
            self.q.task_done()

    def __init__(self):
        super().__init__()  # needed for QT signals
        lock = Lock()
        Thread(target=self.worker, daemon=True, args=(lock,)).start()

    def callback(self, status, progress):
        self.q.put({"TYPE": "PROGRESS", "VALUE": progress})
        self.q.put({"TYPE": "STATUS", "VALUE": status})

    def isDone(self, success):
        self.q.put({"TYPE": "STATUS", "VALUE": "DONE"})

    def addMesh(self, surface):
        """takes a binary stl and adds a Mesh to the current document"""

        if self.simmesh is None:
            self.simmesh = FreeCAD.ActiveDocument.addObject("Mesh::Feature", "Camotics")
        buffer = io.BytesIO()
        buffer.write(surface)
        buffer.seek(0)
        mesh = Mesh.Mesh()
        mesh.read(buffer, "STL")
        self.simmesh.Mesh = mesh
        # Mesh.show(mesh)

    def Activate(self):
        self.taskForm = CAMoticsUI(self)
        FreeCADGui.Control.showDialog(self.taskForm)
        self.job = FreeCADGui.Selection.getSelectionEx()[0].Object
        self.SIM.set_metric()
        self.SIM.set_resolution("high")

        bb = self.job.Stock.Shape.BoundBox
        self.SIM.set_workpiece(
            min=(bb.XMin, bb.YMin, bb.ZMin), max=(bb.XMax, bb.YMax, bb.ZMax)
        )

        for t in self.job.Tools.Group:
            self.SIM.set_tool(
                t.ToolNumber,
                metric=True,
                shape=self.SHAPEMAP.get(t.Tool.ShapeName, "Cylindrical"),
                length=t.Tool.Length.Value,
                diameter=t.Tool.Diameter.Value,
            )

        postlist = PathPost.buildPostList(self.job)
        Path.Log.track(postlist)
        # self.filenames = [PathPost.resolveFileName(self.job)]

        success = True

        finalgcode = ""
        for idx, section in enumerate(postlist):
            partname = section[0]
            sublist = section[1]

            result, gcode, name = PathPost.CommandPathPost().exportObjectsWith(
                sublist,
                partname,
                self.job,
                idx,
                extraargs="--no-show-editor",
            )
            self.filenames.append(name)
            Path.Log.track(result, gcode, name)

            if result is None:
                success = False
            else:
                finalgcode += gcode

        if not success:
            return

        self.SIM.compute_path(finalgcode)
        self.SIM.wait()

        tot = sum([step["time"] for step in self.SIM.get_path()])
        Path.Log.debug("sim time: {}".format(tot))
        self.taskForm.setRunTime(tot)

    def execute(self, timeIndex):
        Path.Log.track()
        self.SIM.start(self.callback, time=timeIndex, done=self.isDone)

    def accept(self):
        pass

    def cancel(self):
        pass

    def buildproject(self):  # , files=[]):
        Path.Log.track()

        job = self.job

        tooltemplate = {
            "units": "metric",
            "shape": "cylindrical",
            "length": 10,
            "diameter": 3.125,
            "description": "",
        }

        workpiecetemplate = {
            "automatic": False,
            "margin": 0,
            "bounds": {"min": [0, 0, 0], "max": [0, 0, 0]},
        }

        camoticstemplate = {
            "units": "metric",
            "resolution-mode": "medium",
            "resolution": 1,
            "tools": {},
            "workpiece": {},
            "files": [],
        }

        unitstring = (
            "imperial" if FreeCAD.Units.getSchema() in [2, 3, 5, 7] else "metric"
        )

        camoticstemplate["units"] = unitstring
        camoticstemplate["resolution-mode"] = "medium"
        camoticstemplate["resolution"] = 1

        toollist = {}
        for t in job.Tools.Group:
            toolitem = tooltemplate.copy()
            toolitem["units"] = unitstring
            if hasattr(t.Tool, "Camotics"):
                toolitem["shape"] = t.Tool.Camotics
            else:
                toolitem["shape"] = self.SHAPEMAP.get(t.Tool.ShapeName, "Cylindrical")

            toolitem["length"] = t.Tool.Length.Value
            toolitem["diameter"] = t.Tool.Diameter.Value
            toolitem["description"] = t.Label
            toollist[t.ToolNumber] = toolitem

        camoticstemplate["tools"] = toollist

        bb = job.Stock.Shape.BoundBox

        workpiecetemplate["bounds"]["min"] = [bb.XMin, bb.YMin, bb.ZMin]
        workpiecetemplate["bounds"]["max"] = [bb.XMax, bb.YMax, bb.ZMax]
        camoticstemplate["workpiece"] = workpiecetemplate

        camoticstemplate["files"] = self.filenames  # files

        return json.dumps(camoticstemplate, indent=2)


class CommandCamoticsSimulate:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Camotics",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Camotics", "Camotics"),
            "Accel": "P, C",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Camotics", "Simulate using Camotics"),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            job = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(job.Proxy, Path.Main.Job.ObjectJob)
        except:
            return False

    def Activated(self):
        pathSimulation = CamoticsSimulation()
        pathSimulation.Activate()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_Camotics", CommandCamoticsSimulate())


FreeCAD.Console.PrintLog("Loading PathCamoticsSimulateGui ... done\n")
