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

import FreeCAD
import FreeCADGui
import PathScripts.PathLog as PathLog
# from pivy import coin
# from itertools import cycle
import json
import Mesh
import time
import camotics
import PathScripts.PathPost  as PathPost
import io
import PathScripts
import queue
from threading import Thread, Lock
import subprocess
import PySide

from PySide import QtCore, QtGui

__title__ = "Camotics Simulator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Task panel for Camotics Simulation"

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class CAMoticsUI:
    def __init__(self, simulation):
        # this will create a Qt widget from our ui file
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/TaskPathCamoticsSim.ui")
        self.simulation = simulation
        self.opModel = PySide.QtGui.QStandardItemModel(0, 0)#len(self.columnNames()))
        self.initializeUI()
        self.lock = False

    def columnNames(self):
        return ['Operation']

    def loadData(self):
        self.opModel.clear()
        self.opModel.setHorizontalHeaderLabels(self.columnNames())
        ops = self.simulation.finalpostlist
        for i, op in enumerate(ops):
            libItem = PySide.QtGui.QStandardItem(op.Label)
            libItem.setToolTip('op')
            libItem.setData(op)
            #libItem.setIcon(PySide.QtGui.QPixmap(':/icons/Path_ToolTable.svg'))
            self.opModel.appendRow(libItem)


    def initializeUI(self):
        self.form.progressBar.reset()
        self.updateEstimate("00:00:00")
        self.form.btnRun.setText("Run")
        self.form.btnLaunchCamotics.clicked.connect(self.launchCamotics)
        self.simulation.progressUpdate.connect(self.calculating)
        self.simulation.estimateChanged.connect(self.updateEstimate)
        self.form.btnRun.clicked.connect(self.runButton)
        self.form.lstPathObjects.setModel(self.opModel)
        self.form.lstPathObjects.selectionModel().selectionChanged.connect(self.reSelect)
        self.loadData()
        self.selectAll()

    def selectAll(self):
        selmodel = self.form.lstPathObjects.selectionModel()
        index0 = self.opModel.index(0, 0)
        index1 = self.opModel.index(self.opModel.rowCount()-1,0)
        itemSelection = QtCore.QItemSelection(index0, index1)
        selmodel.blockSignals(True)
        selmodel.select(itemSelection, QtCore.QItemSelectionModel.Rows | QtCore.QItemSelectionModel.Select)
        selmodel.blockSignals(False)


    def reSelect(self):
        selmodel = self.form.lstPathObjects.selectionModel()
        item = selmodel.selection().indexes()[0]

        index0 = self.opModel.index(0, 0)
        itemSelection = QtCore.QItemSelection(index0, item)

        selmodel.blockSignals(True)
        selmodel.select(itemSelection,
                        QtCore.QItemSelectionModel.Rows | QtCore.QItemSelectionModel.Select)
        selmodel.blockSignals(False)

        selectedObjs = [self.opModel.itemFromIndex(i).data() for i in selmodel.selection().indexes()]

        self.simulation.setSimulationPath(selectedObjs)

    def runButton(self):
        if self.form.btnRun.text() == 'Run':
            self.simulation.run()
        else:
            self.simulation.stopSim()


    def updateEstimate(self, timestring):
        self.form.txtRunEstimate.setText(timestring)

    def launchCamotics(self):
        projfile = self.simulation.buildproject()
        subprocess.Popen(["camotics", projfile])

    def accept(self):
        self.simulation.accept()
        FreeCADGui.Control.closeDialog()

    def reject(self):
        self.simulation.cancel()
        FreeCADGui.Control.closeDialog()

    def calculating(self, progress=0.0):
        if progress < 1.0:
            self.form.btnRun.setText('Stop')
        else:
            self.form.btnRun.setText('Run')
        self.form.progressBar.setValue(int(progress*100))


class CamoticsSimulation(QtCore.QObject):

    SIM = camotics.Simulation()
    q = queue.Queue()
    progressUpdate      = QtCore.Signal(object)
    estimateChanged     = QtCore.Signal(str)

    SHAPEMAP = {'ballend': 'Ballnose',
                'endmill': 'Cylindrical',
                'v-bit'  : 'Conical',
                'chamfer': 'Snubnose'}

    cutMaterial = None

    def worker(self, lock):
        while True:
            item = self.q.get()
            with lock:
                if item['TYPE'] == 'STATUS':
                    if item['VALUE'] == 'DONE':
                        self.SIM.wait()
                        surface = self.SIM.get_surface('binary')
                        #surface = self.SIM.get_surface('python')
                        self.SIM.wait()
                        self.addMesh(surface)
                        #self.makeCoinMesh(surface)
                elif item['TYPE'] == 'PROGRESS':
                    #self.taskForm.calculating(item['VALUE'])
                    msg = item['VALUE']
                    self.progressUpdate.emit(msg)
            self.q.task_done()



    def __init__(self):
        super().__init__()  # needed for QT signals
        lock = Lock()
        Thread(target=self.worker, daemon=True, args=(lock,)).start()

    def callback(self, status, progress):
        self.q.put({'TYPE': 'PROGRESS', 'VALUE': progress})
        self.q.put({'TYPE': 'STATUS'  , 'VALUE': status  })

    def isDone(self, success):
        self.q.put({'TYPE': 'STATUS'  , 'VALUE': 'DONE'})

    def stopSim(self):
        if self.SIM.is_running():
            self.SIM.interrupt()
            self.SIM.wait()
        return True


    def addMesh(self, surface):
        '''takes a binary stl and adds a Mesh to the current docuemnt'''

        if self.cutMaterial is None:
            self.cutMaterial = FreeCAD.ActiveDocument.addObject("Mesh::Feature", "SimulationOutput")
        buffer=io.BytesIO()
        buffer.write(surface)
        buffer.seek(0)
        mesh=Mesh.Mesh()
        mesh.read(buffer, "STL")
        self.cutMaterial.Mesh = mesh
        # Mesh.show(mesh)

    def setSimulationPath(self, postlist):
        gcode, fname = PathPost.CommandPathPost().getGcodeSilently(postlist, self.job)
        self.SIM.compute_path(gcode)
        self.SIM.wait()

        tot = sum([step['time'] for step in self.SIM.get_path()])
        self.estimateChanged.emit(time.strftime("%H:%M:%S", time.gmtime(tot)))

    def Activate(self):
        self.job = FreeCADGui.Selection.getSelectionEx()[0].Object
        postlist = PathPost.buildPostList(self.job)
        self.finalpostlist = [item for slist in postlist for item in slist]

        self.taskForm = CAMoticsUI(self)
        FreeCADGui.Control.showDialog(self.taskForm)

        self.SIM.set_metric()
        self.SIM.set_resolution('high')

        bb = self.job.Stock.Shape.BoundBox
        self.SIM.set_workpiece(min = (bb.XMin, bb.YMin, bb.ZMin), max = (bb.XMax, bb.YMax, bb.ZMax))

        for t in self.job.Tools.Group:
            self.SIM.set_tool(t.ToolNumber,
                    metric = True,
                    shape = self.SHAPEMAP.get(t.Tool.ShapeName, 'Cylindrical'),
                    length = t.Tool.Length.Value,
                    diameter = t.Tool.Diameter.Value)


        self.setSimulationPath(self.finalpostlist)

    def run(self):
        self.SIM.start(self.callback, done=self.isDone)

    #def makeCoinMesh(self, surface):
    #    # this doesn't work yet
    #    #sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph();
    #    color = coin.SoBaseColor()
    #    color.rgb = (1, 0, 1)
    #    coords = coin.SoTransform()
    #    node = coin.SoSeparator()
    #    node.addChild(color)
    #    node.addChild(coords)

    #    end = [-1]
    #    vertices = list(zip(*[iter(surface['vertices'])] * 3))
    #    #polygons = list(zip(*[iter(vertices)] * 3, cycle(end)))
    #    polygons = list(zip(*[iter(range(len(vertices)))] * 3, cycle(end)))

    #    print('verts {}'.format(len(vertices)))
    #    print('polygons {}'.format(len(polygons)))

    #    data=coin.SoCoordinate3()
    #    face=coin.SoIndexedFaceSet()
    #    node.addChild(data)
    #    node.addChild(face)

    #    # i = 0
    #    # for i, v in enumerate(vertices):
    #    #     print('i: {} v: {}'.format(i, v))
    #        #data.point.set1Value(i, v[0], v[1], v[2])
    #    #     i += 1
    #    # i = 0
    #    # for p in polygons:
    #    #     try:
    #    #         face.coordIndex.set1Value(i, p)
    #    #         i += 1
    #    #     except Exception as e:
    #    #         print(e)
    #    #         print(i)
    #    #         print(p)

    #    # sg.addChild(node)


    def RemoveMaterial(self):
        if self.cutMaterial is not None:
            FreeCAD.ActiveDocument.removeObject(self.cutMaterial.Name)
            self.cutMaterial = None

    def accept(self):
        pass

    def cancel(self):
        self.RemoveMaterial()

    def buildproject(self):

        job = self.job
        gcode, fname = PathPost.CommandPathPost().getGcodeSilently(self.finalpostlist, self.job, temp=False)

        tooltemplate = {
            "units": "metric",
            "shape": "cylindrical",
            "length": 10,
            "diameter": 3.125,
            "description": ""
        }

        workpiecetemplate = {
            "automatic": False,
            "margin": 0,
            "bounds": {
                "min": [0, 0, 0],
                "max": [0, 0, 0]}
        }

        camoticstemplate = {
            "units": "metric",
            "resolution-mode": "high",
            "resolution": 1,
            "tools": {},
            "workpiece": {},
            "files": []
        }

        unitstring = "imperial" if FreeCAD.Units.getSchema() in [2,3,5,7] else "metric"

        camoticstemplate["units"] = unitstring
        camoticstemplate["resolution-mode"] = "medium"
        camoticstemplate["resolution"] = 1

        toollist = {}
        for t in job.Tools.Group:
            tooltemplate["units"] = unitstring
            if hasattr(t.Tool, 'Camotics'):
                tooltemplate["shape"] = t.Tool.Camotics
            else:
                tooltemplate["shape"] = self.SHAPEMAP.get(t.Tool.ShapeName, 'Cylindrical')

            tooltemplate["length"] = t.Tool.Length.Value
            tooltemplate["diameter"] = t.Tool.Diameter.Value
            tooltemplate["description"] = t.Label
            toollist[t.ToolNumber] = tooltemplate

        camoticstemplate['tools'] = toollist

        bb = job.Stock.Shape.BoundBox

        workpiecetemplate['bounds']['min'] = [bb.XMin, bb.YMin, bb.ZMin]
        workpiecetemplate['bounds']['max'] = [bb.XMax, bb.YMax, bb.ZMax]
        camoticstemplate['workpiece'] = workpiecetemplate

        camoticstemplate['files'] = [fname]

        foo = QtGui.QFileDialog.getSaveFileName(QtGui.QApplication.activeWindow(), "Camotics Project File")
        if foo:
            tfile = foo[0]
        else:
            return None

        with open(tfile, 'w') as t:
            proj=json.dumps(camoticstemplate, indent=2)
            t.write(proj)

        return tfile #json.dumps(camoticstemplate, indent=2)


class CommandCamoticsSimulate:
    def GetResources(self):
        return {'Pixmap': 'Path_Camotics',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Camotics", "Camotics"),
                'Accel': "P, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Camotics", "Simulate using Camotics"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            job = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(job.Proxy, PathScripts.PathJob.ObjectJob)
        except:
            return False

    def Activated(self):
        pathSimulation.Activate()


pathSimulation = CamoticsSimulation()

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_Camotics', CommandCamoticsSimulate())


FreeCAD.Console.PrintLog("Loading PathCamoticsSimulateGui ... done\n")
