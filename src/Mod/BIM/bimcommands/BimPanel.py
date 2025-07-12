# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM Panel-related Arch_"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


#           Description                 l    w    t

Presets = [None,
           ["Plywood 12mm, 1220 x 2440",1220,2440,12],
           ["Plywood 15mm, 1220 x 2440",1220,2440,15],
           ["Plywood 18mm, 1220 x 2440",1220,2440,18],
           ["Plywood 25mm, 1220 x 2440",1220,2440,25],
           ["MDF 3mm, 900 x 600",       900, 600, 3],
           ["MDF 6mm, 900 x 600",       900, 600, 6],
           ["OSB 18mm, 1220 x 2440",    1220,2440,18],
           ]


class Arch_Panel:

    "the Arch Panel Arch_ definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Panel',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Panel","Panel"),
                'Accel': "P, A",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Panel","Creates a panel object from scratch or from a selected object (sketch, wire, face or solid)")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        import WorkingPlane
        import Draft
        import draftguitools.gui_trackers as DraftTrackers
        from draftutils import params

        self.doc = FreeCAD.ActiveDocument
        self.Length = params.get_param_arch("PanelLength")
        self.Width = params.get_param_arch("PanelWidth")
        self.Thickness = params.get_param_arch("PanelThickness")
        self.Profile = None
        self.featureName = "Panel"
        self.rotated = False
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            if len(sel) == 1:
                if Draft.getType(sel[0]) == "Panel":
                    return
            self.doc.openTransaction(str(translate("Arch","Create Panel")))
            FreeCADGui.addModule("Arch")
            FreeCADGui.addModule("Draft")
            for obj in sel:
                FreeCADGui.doCommand("obj = Arch.makePanel(FreeCAD.ActiveDocument." + obj.Name + ",thickness=" + str(self.Thickness) + ")")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
            self.doc.commitTransaction()
            self.doc.recompute()
            return

        # interactive mode

        FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
        WorkingPlane.get_working_plane()
        self.points = []
        self.tracker = DraftTrackers.boxTracker()
        self.tracker.width(self.Width)
        self.tracker.height(self.Thickness)
        self.tracker.length(self.Length)
        self.tracker.on()
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
        FreeCADGui.draftToolBar.continueCmd.show()

    def getPoint(self,point=None,obj=None):

        "this function is called by the snapper when it has a 3D point"

        import DraftVecUtils

        FreeCAD.activeDraftCommand = None
        FreeCADGui.Snapper.off()
        self.tracker.finalize()
        if point is None:
            return
        self.doc.openTransaction(translate("Arch","Create Panel"))
        FreeCADGui.addModule("Arch")
        if self.Profile:
            pr = Presets[self.Profile]
            FreeCADGui.doCommand('p = Arch.makeProfile('+str(pr[2])+','+str(pr[3])+','+str(pr[4])+','+str(pr[5])+')')
            FreeCADGui.doCommand('s = Arch.makePanel(p,thickness='+str(self.Thickness)+')')
            #FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
        else:
            FreeCADGui.doCommand('s = Arch.makePanel(length='+str(self.Length)+',width='+str(self.Width)+',thickness='+str(self.Thickness)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        if self.rotated:
            FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1.00,0.00,0.00),90.00)')
        self.doc.commitTransaction()
        self.doc.recompute()
        from PySide import QtCore
        QtCore.QTimer.singleShot(100, self.check_continueMode)


    def check_continueMode(self):

        "checks if continueMode is true and restarts Panel"

        if FreeCADGui.draftToolBar.continueMode:
            self.Activated()

    def taskbox(self):

        "sets up a taskbox widget"

        from draftutils import params
        from PySide import QtCore, QtGui
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Panel options"))
        grid = QtGui.QGridLayout(w)

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset"))
        valuep = QtGui.QComboBox()
        fpresets = [" "]
        for p in Presets[1:]:
            fpresets.append(translate("Arch",p[0]))
        valuep.addItems(fpresets)
        grid.addWidget(labelp,0,0,1,1)
        grid.addWidget(valuep,0,1,1,1)

        # length
        label1 = QtGui.QLabel(translate("Arch","Length"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(FreeCAD.Units.Quantity(self.Length,FreeCAD.Units.Length).UserString)
        grid.addWidget(label1,1,0,1,1)
        grid.addWidget(self.vLength,1,1,1,1)

        # width
        label2 = QtGui.QLabel(translate("Arch","Width"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(label2,2,0,1,1)
        grid.addWidget(self.vWidth,2,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Thickness"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(FreeCAD.Units.Quantity(self.Thickness,FreeCAD.Units.Length).UserString)
        grid.addWidget(label3,3,0,1,1)
        grid.addWidget(self.vHeight,3,1,1,1)

        # horizontal button
        value4= QtGui.QPushButton(translate("Arch","Rotate"))
        grid.addWidget(value4,4,0,1,2)

        valuep.currentIndexChanged.connect(self.setPreset)
        self.vLength.valueChanged.connect(self.setLength)
        self.vWidth.valueChanged.connect(self.setWidth)
        self.vHeight.valueChanged.connect(self.setThickness)
        value4.pressed.connect(self.rotate)
        return w

    def update(self,point,info):

        "this function is called by the Snapper when the mouse is moved"

        if FreeCADGui.Control.activeDialog():
            self.tracker.pos(point)
            if self.rotated:
                self.tracker.width(self.Thickness)
                self.tracker.height(self.Width)
                self.tracker.length(self.Length)
            else:
                self.tracker.width(self.Width)
                self.tracker.height(self.Thickness)
                self.tracker.length(self.Length)

    def setWidth(self,d):

        from draftutils import params
        self.Width = d.Value
        params.set_param_arch("PanelWidth",d)

    def setThickness(self,d):

        from draftutils import params
        self.Thickness = d.Value
        params.set_param_arch("PanelThickness",d)

    def setLength(self,d):

        from draftutils import params
        self.Length = d.Value
        params.set_param_arch("PanelLength",d)

    def setPreset(self,i):

        if i > 0:
            self.vLength.setText(FreeCAD.Units.Quantity(float(Presets[i][1]),FreeCAD.Units.Length).UserString)
            self.vWidth.setText(FreeCAD.Units.Quantity(float(Presets[i][2]),FreeCAD.Units.Length).UserString)
            self.vHeight.setText(FreeCAD.Units.Quantity(float(Presets[i][3]),FreeCAD.Units.Length).UserString)

    def rotate(self):

        self.rotated = not self.rotated


class Arch_PanelCut:

    "the Arch Panel Cut command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Panel_Cut',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Panel_Cut","Panel Cut"),
                'Accel': "P, C",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Panel_Cut","Creates 2D views of selected panels")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        import Draft
        if FreeCADGui.Selection.getSelection():
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Panel Cut")))
            FreeCADGui.addModule("Arch")
            for obj in FreeCADGui.Selection.getSelection():
                if Draft.getType(obj) == "Panel":
                    FreeCADGui.doCommand("Arch.makePanelCut(FreeCAD.ActiveDocument."+obj.Name+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class Arch_PanelSheet:

    "the Arch Panel Sheet definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Panel_Sheet',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Panel_Sheet","Panel Sheet"),
                'Accel': "P, S",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Panel_Sheet","Creates a 2D sheet which can contain panel cuts")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Panel Sheet")))
        FreeCADGui.addModule("Arch")
        if FreeCADGui.Selection.getSelection():
            l = "["
            for obj in FreeCADGui.Selection.getSelection():
                l += "FreeCAD.ActiveDocument."+obj.Name+","
            l += "]"
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            FreeCADGui.doCommand("__objs__ = "+l)
            FreeCADGui.doCommand("Arch.makePanelSheet(__objs__)")
            FreeCADGui.doCommand("del __objs__")
        else:
            FreeCADGui.doCommand("Arch.makePanelSheet()")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Arch_Nest:

    "the Arch Panel definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Nest',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Nest","Nest"),
                'Accel': "N, E",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Nest","Nests a series of selected shapes in a container")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(NestTaskPanel())


class NestTaskPanel:


    '''The TaskPanel for Arch Nest'''

    def __init__(self,obj=None):

        import ArchNesting
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchNest.ui")
        self.form.progressBar.hide()
        self.form.ButtonPreview.setEnabled(False)
        self.form.ButtonStop.setEnabled(False)
        self.form.ButtonContainer.pressed.connect(self.getContainer)
        self.form.ButtonShapes.pressed.connect(self.getShapes)
        self.form.ButtonRemove.pressed.connect(self.removeShapes)
        self.form.ButtonStart.pressed.connect(self.start)
        self.form.ButtonStop.pressed.connect(self.stop)
        self.form.ButtonPreview.pressed.connect(self.preview)
        self.shapes = []
        self.container = None
        self.nester = None
        self.temps = []

    def reject(self):

        self.stop()
        self.clearTemps()
        return True

    def accept(self):

        self.stop()
        self.clearTemps()
        if self.nester:
            FreeCAD.ActiveDocument.openTransaction("Nesting")
            self.nester.apply()
            FreeCAD.ActiveDocument.commitTransaction()
        return True

    def clearTemps(self):

        for t in self.temps:
            if FreeCAD.ActiveDocument.getObject(t.Name):
                FreeCAD.ActiveDocument.removeObject(t.Name)
        self.temps = []

    def getContainer(self):

        import Draft
        s = FreeCADGui.Selection.getSelection()
        if len(s) == 1:
            if hasattr(s[0],'Shape'):
                if len(s[0].Shape.Faces) == 1:
                    if not (s[0] in self.shapes):
                        self.form.Container.clear()
                        self.addObject(s[0],self.form.Container)
                        self.container = s[0]
                else:
                    FreeCAD.Console.PrintError(translate("Arch","This object has no face"))
                if Draft.getType(s[0]) == "PanelSheet":
                    if hasattr(s[0],"Rotations"):
                        if s[0].Rotations:
                            self.form.Rotations.setText(str(s[0].Rotations))

    def getShapes(self):

        s = FreeCADGui.Selection.getSelection()
        for o in s:
            if hasattr(o,'Shape'):
                if not o in self.shapes:
                    if o != self.container:
                        self.addObject(o,self.form.Shapes)
                        self.shapes.append(o)

    def addObject(self,obj,form):

        from PySide import QtGui
        i = QtGui.QListWidgetItem()
        i.setText(obj.Label)
        i.setToolTip(obj.Name)
        if hasattr(obj.ViewObject,"Proxy"):
            if hasattr(obj.ViewObject.Proxy,"getIcon"):
                i.setIcon(QtGui.QIcon(obj.ViewObject.Proxy.getIcon()))
        elif hasattr(obj.ViewObject, "Icon"):
            i.setIcon(QtGui.QIcon(obj.ViewObject.Icon))
        else:
            i.setIcon(QtGui.QIcon(":/icons/Part_3D_object.svg"))
        form.addItem(i)

    def removeShapes(self):

        for i in self.form.Shapes.selectedItems():
            o = FreeCAD.ActiveDocument.getObject(i.toolTip())
            if o:
                if o in self.shapes:
                    self.shapes.remove(o)
                    self.form.Shapes.takeItem(self.form.Shapes.row(i))

    def setCounter(self,value):

        self.form.progressBar.setValue(value)

    def start(self):

        from PySide import QtGui
        import ArchNesting
        self.clearTemps()
        self.form.progressBar.setFormat("pass 1: %p%")
        self.form.progressBar.setValue(0)
        self.form.progressBar.show()
        tolerance = self.form.Tolerance.value()
        discretize = self.form.Subdivisions.value()
        rotations = [float(x) for x in self.form.Rotations.text().split(",")]
        ArchNesting.TOLERANCE = tolerance
        ArchNesting.DISCRETIZE = discretize
        ArchNesting.ROTATIONS = rotations
        self.nester = ArchNesting.Nester()
        self.nester.addContainer(self.container)
        self.nester.addObjects(self.shapes)
        self.nester.setCounter = self.setCounter
        self.form.ButtonStop.setEnabled(True)
        self.form.ButtonStart.setEnabled(False)
        self.form.ButtonPreview.setEnabled(False)
        QtGui.QApplication.processEvents()
        result = self.nester.run()
        self.form.progressBar.hide()
        self.form.ButtonStart.setEnabled(True)
        self.form.ButtonStop.setEnabled(False)
        if result:
            self.form.ButtonPreview.setEnabled(True)

    def stop(self):

        if self.nester:
            self.nester.stop()
        self.form.ButtonStart.setEnabled(True)
        self.form.ButtonStop.setEnabled(False)
        self.form.ButtonPreview.setEnabled(False)
        self.form.progressBar.hide()

    def preview(self):

        self.clearTemps()
        if self.nester:
            t = self.nester.show()
            if t:
                self.temps.extend(t)


class Arch_PanelGroup:

    def GetCommands(self):
        return tuple(['Arch_Panel','Arch_Panel_Cut','Arch_Panel_Sheet','Arch_Nest'])

    def GetResources(self):
        return { 'MenuText': QT_TRANSLATE_NOOP("Arch_PanelTools",'Panel Tools'),
                 'ToolTip': QT_TRANSLATE_NOOP("Arch_PanelTools",'Panel tools')
               }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v


FreeCADGui.addCommand('Arch_Panel',Arch_Panel())
FreeCADGui.addCommand('Arch_Panel_Cut',Arch_PanelCut())
FreeCADGui.addCommand('Arch_Panel_Sheet',Arch_PanelSheet())
FreeCADGui.addCommand('Arch_Nest',Arch_Nest())
FreeCADGui.addCommand('Arch_PanelTools', Arch_PanelGroup())
