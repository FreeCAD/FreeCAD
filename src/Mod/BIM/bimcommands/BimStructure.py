#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************
#Modified 2016-01-03 JAndersM

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
import ArchProfile
from draftutils import params
from draftutils import gui_utils

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import ArchPrecast
    import draftguitools.gui_trackers as DraftTrackers
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchStructure
#  \ingroup ARCH
#  \brief The Structure object and tools
#
#  This module provides tools to build Structure objects.
#  Structure elements are beams, columns, slabs, and other
#  elements that have a structural function, that is, that
#  support other parts of the building.

__title__= "FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"


#Reads preset profiles and categorizes them
Categories=[]
Presets=ArchProfile.readPresets()
for pre in Presets:
    if pre[1] not in Categories:
        Categories.append(pre[1])




class CommandStructuresFromSelection:
    """ The Arch Structures from selection command definition. """

    def __init__(self):
        pass

    def GetResources(self):
        return {'Pixmap': 'Arch_MultipleStructures',
                'MenuText': QT_TRANSLATE_NOOP("Arch_StructuresFromSelection", "Multiple Structures"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_StructuresFromSelection", "Create multiple BIM Structures from a selected base, using each selected edge as an extrusion path")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        selex = FreeCADGui.Selection.getSelectionEx()
        if len(selex) >= 2:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Create Structures From Selection"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.addModule("Draft")
            base = selex[0].Object # The first selected object is the base for the Structure objects
            for selexi in selex[1:]: # All the edges from the other objects are used as a Tool (extrusion paths)
                if len(selexi.SubElementNames) == 0:
                    subelement_names = ["Edge" + str(i) for i in range(1, len(selexi.Object.Shape.Edges) + 1)]
                else:
                    subelement_names = [sub for sub in selexi.SubElementNames if sub.startswith("Edge")]
                for sub in subelement_names:
                    FreeCADGui.doCommand("structure = Arch.makeStructure(FreeCAD.ActiveDocument." + base.Name + ")")
                    FreeCADGui.doCommand("structure.Tool = (FreeCAD.ActiveDocument." + selexi.Object.Name + ", '" + sub + "')")
                    FreeCADGui.doCommand("structure.BasePerpendicularToTool = True")
                    FreeCADGui.doCommand("Draft.autogroup(structure)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            FreeCAD.Console.PrintError(translate("Arch", "Please select the base object first and then the edges to use as extrusion paths") + "\n")


class CommandStructuralSystem:
    """ The Arch Structural System command definition. """

    def __init__(self):
        pass

    def GetResources(self):
        return {'Pixmap': 'Arch_StructuralSystem',
                'MenuText': QT_TRANSLATE_NOOP("Arch_StructuralSystem", "Structural System"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_StructuralSystem", "Create a structural system from a selected structure and axis")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            st = Draft.getObjectsOfType(sel, "Structure")
            ax = Draft.getObjectsOfType(sel, "Axis")
            if ax:
                FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Create Structural System"))
                FreeCADGui.addModule("Arch")
                if st:
                    FreeCADGui.doCommand("obj = Arch.makeStructuralSystem(" + ArchCommands.getStringList(st) + ", " + ArchCommands.getStringList(ax) + ")")
                else:
                    FreeCADGui.doCommand("obj = Arch.makeStructuralSystem(axes = " + ArchCommands.getStringList(ax) + ")")
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
            else:
                FreeCAD.Console.PrintError(translate("Arch", "Please select at least an axis object") + "\n")


class CommandStructure:

    "the Arch Structure command definition"

    def __init__(self):

        self.beammode = False

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Structure',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure from scratch or from a selected object (sketch, wire, face or solid)")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        self.Width = params.get_param_arch("StructureWidth")
        if self.beammode:
            self.Height = params.get_param_arch("StructureLength")
            self.Length = params.get_param_arch("StructureHeight")
        else:
            self.Length = params.get_param_arch("StructureLength")
            self.Height = params.get_param_arch("StructureHeight")
        self.Profile = None
        self.continueCmd = False
        self.bpoint = None
        self.bmode = False
        self.precastvalues = None
        self.wp = None
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            st = Draft.getObjectsOfType(sel,"Structure")
            ax = Draft.getObjectsOfType(sel,"Axis")
            if ax:
                FreeCADGui.runCommand("Arch_StructuralSystem")
                return
            elif not(ax) and not(st):
                FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Structure"))
                FreeCADGui.addModule("Arch")
                for obj in sel:
                    FreeCADGui.doCommand("obj = Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ")")
                    FreeCADGui.addModule("Draft")
                    FreeCADGui.doCommand("Draft.autogroup(obj)")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                return

        # interactive mode
        import WorkingPlane
        self.wp = WorkingPlane.get_working_plane()

        self.points = []
        self.tracker = DraftTrackers.boxTracker()
        self.tracker.width(self.Width)
        self.tracker.height(self.Height)
        self.tracker.length(self.Length)
        self.tracker.setRotation(self.wp.get_placement().Rotation)
        self.tracker.on()
        self.precast = ArchPrecast._PrecastTaskPanel()
        self.dents = ArchPrecast._DentsTaskPanel()
        self.precast.Dents = self.dents
        if self.beammode:
            title=translate("Arch","First point of the beam")+":"
        else:
            title=translate("Arch","Base point of column")+":"
        FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=[self.taskbox(),self.precast.form,self.dents.form],title=title)

    def getPoint(self,point=None,obj=None):

        "this function is called by the snapper when it has a 3D point"

        self.bmode = self.modeb.isChecked()
        if point is None:
            self.tracker.finalize()
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            return
        if self.bmode and (self.bpoint is None):
            self.bpoint = point
            FreeCADGui.Snapper.getPoint(last=point,callback=self.getPoint,movecallback=self.update,extradlg=[self.taskbox(),self.precast.form,self.dents.form],title=translate("Arch","Next point")+":",mode="line")
            return
        self.tracker.off()
        FreeCAD.activeDraftCommand = None
        FreeCADGui.Snapper.off()
        horiz = True # determines the type of rotation to apply to the final object
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Structure"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("WorkingPlane")
        if self.bmode:
            self.Length = point.sub(self.bpoint).Length
            params.set_param_arch("StructureHeight",self.Length)
        if self.Profile is not None:
            try: # try to update latest precast values - fails if dialog has been destroyed already
                self.precastvalues = self.precast.getValues()
            except Exception:
                pass
            if ("Precast" in self.Profile) and self.precastvalues:
                # precast concrete
                self.precastvalues["PrecastType"] = self.Profile.split("_")[1]
                self.precastvalues["Length"] = self.Length
                self.precastvalues["Width"] = self.Width
                self.precastvalues["Height"] = self.Height
                argstring = ""
                # fix for precast placement, since their (0,0) point is the lower left corner
                if self.bmode:
                    delta = FreeCAD.Vector(0,0-self.Width/2,0)
                else:
                    delta = FreeCAD.Vector(-self.Length/2,-self.Width/2,0)
                delta = self.wp.get_global_coords(delta,as_vector=True)
                point = point.add(delta)
                if self.bpoint:
                    self.bpoint = self.bpoint.add(delta)
                # build the string definition
                for pair in self.precastvalues.items():
                    argstring += pair[0].lower() + "="
                    if isinstance(pair[1],str):
                        argstring += '"' + pair[1] + '",'
                    else:
                        argstring += str(pair[1]) + ","
                FreeCADGui.addModule("ArchPrecast")
                FreeCADGui.doCommand("s = ArchPrecast.makePrecast("+argstring+")")
            else:
                # metal profile
                FreeCADGui.doCommand('p = Arch.makeProfile('+str(self.Profile)+')')
                if (abs(self.Length - self.Profile[4]) >= 0.1) or self.bmode: # forgive rounding errors
                    # horizontal
                    FreeCADGui.doCommand('s = Arch.makeStructure(p,length='+str(self.Length)+')')
                    horiz = False
                else:
                    # vertical
                    FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Height)+')')
                    #if not self.bmode:
                    #    FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
                FreeCADGui.doCommand('s.Profile = "'+self.Profile[2]+'"')
        else :
            FreeCADGui.doCommand('s = Arch.makeStructure(length='+str(self.Length)+',width='+str(self.Width)+',height='+str(self.Height)+')')

        # calculate rotation
        if self.bmode and self.bpoint:
            FreeCADGui.doCommand('s.Placement = Arch.placeAlongEdge('+DraftVecUtils.toString(self.bpoint)+","+DraftVecUtils.toString(point)+","+str(horiz)+")")
        else:
            FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
            FreeCADGui.doCommand('wp = WorkingPlane.get_working_plane()')
            FreeCADGui.doCommand('s.Placement.Rotation = s.Placement.Rotation.multiply(wp.get_placement().Rotation)')

        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(s)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        # gui_utils.end_all_events()  # Causes a crash on Linux.
        self.tracker.finalize()
        if self.continueCmd:
            self.Activated()

    def _createItemlist(self, baselist):

        "create nice labels for presets in the task panel"

        ilist=[]
        for p in baselist:
            f = FreeCAD.Units.Quantity(p[4],FreeCAD.Units.Length).getUserPreferred()
            d = params.get_param("Decimals",path="Units")
            s1 = str(round(p[4]/f[1],d))
            s2 = str(round(p[5]/f[1],d))
            s3 = str(f[2])
            ilist.append(p[2]+" ("+s1+"x"+s2+s3+")")
        return ilist

    def taskbox(self):

        "sets up a taskbox widget"

        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Structure options"))
        grid = QtGui.QGridLayout(w)

        # mode box
        labelmode = QtGui.QLabel(translate("Arch","Parameters of the structure")+":")
        self.modeb = QtGui.QRadioButton(translate("Arch","Beam"))
        self.modec = QtGui.QRadioButton(translate("Arch","Column"))
        if self.bpoint or self.beammode:
            self.modeb.setChecked(True)
        else:
            self.modec.setChecked(True)
        grid.addWidget(labelmode,0,0,1,2)
        grid.addWidget(self.modeb,1,0,1,1)
        grid.addWidget(self.modec,1,1,1,1)

        # categories box
        labelc = QtGui.QLabel(translate("Arch","Category"))
        self.valuec = QtGui.QComboBox()
        self.valuec.addItems([" ","Precast concrete"]+Categories)
        grid.addWidget(labelc,2,0,1,1)
        grid.addWidget(self.valuec,2,1,1,1)

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset"))
        self.vPresets = QtGui.QComboBox()
        self.pSelect = [None]
        fpresets = [" "]
        self.vPresets.addItems(fpresets)
        grid.addWidget(labelp,3,0,1,1)
        grid.addWidget(self.vPresets,3,1,1,1)

        # length
        label1 = QtGui.QLabel(translate("Arch","Length"))
        self.vLength = ui.createWidget("Gui::InputField")
        if self.modeb.isChecked():
            self.vLength.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
        else:
            self.vLength.setText(FreeCAD.Units.Quantity(self.Length,FreeCAD.Units.Length).UserString)
        grid.addWidget(label1,4,0,1,1)
        grid.addWidget(self.vLength,4,1,1,1)

        # width
        label2 = QtGui.QLabel(translate("Arch","Width"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(label2,5,0,1,1)
        grid.addWidget(self.vWidth,5,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Height"))
        self.vHeight = ui.createWidget("Gui::InputField")
        if self.modeb.isChecked():
            self.vHeight.setText(FreeCAD.Units.Quantity(self.Length,FreeCAD.Units.Length).UserString)
        else:
            self.vHeight.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
        grid.addWidget(label3,6,0,1,1)
        grid.addWidget(self.vHeight,6,1,1,1)

        # horizontal button
        value5 = QtGui.QPushButton(translate("Arch","Switch Length/Height"))
        grid.addWidget(value5,7,0,1,1)
        value6 = QtGui.QPushButton(translate("Arch","Switch Length/Width"))
        grid.addWidget(value6,7,1,1,1)

        # continue button
        label4 = QtGui.QLabel(translate("Arch","Con&tinue"))
        value4 = QtGui.QCheckBox()
        value4.setObjectName("ContinueCmd")
        value4.setLayoutDirection(QtCore.Qt.RightToLeft)
        label4.setBuddy(value4)
        self.continueCmd = params.get_param("ContinueMode")
        value4.setChecked(self.continueCmd)
        grid.addWidget(label4,8,0,1,1)
        grid.addWidget(value4,8,1,1,1)

        # connect slots
        QtCore.QObject.connect(self.valuec,QtCore.SIGNAL("currentIndexChanged(int)"),self.setCategory)
        QtCore.QObject.connect(self.vPresets,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        QtCore.QObject.connect(self.vLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.vWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.vHeight,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotateLH)
        QtCore.QObject.connect(value6,QtCore.SIGNAL("pressed()"),self.rotateLW)
        QtCore.QObject.connect(self.modeb,QtCore.SIGNAL("toggled(bool)"),self.switchLH)

        # restore preset
        stored = params.get_param_arch("StructurePreset")
        if stored:
            if stored.lower().startswith("precast_"):
                self.valuec.setCurrentIndex(1)
                tp = stored.split("_")[1]
                if tp and (tp in self.precast.PrecastTypes):
                    self.vPresets.setCurrentIndex(self.precast.PrecastTypes.index(tp))
            elif ";" in stored:
                stored = stored.split(";")
                if len(stored) >= 3:
                    if stored[1] in Categories:
                        self.valuec.setCurrentIndex(2+Categories.index(stored[1]))
                        ps = [p[2] for p in self.pSelect]
                        if stored[2] in ps:
                            self.vPresets.setCurrentIndex(ps.index(stored[2]))
        return w

    def update(self,point,info):

        "this function is called by the Snapper when the mouse is moved"

        if FreeCADGui.Control.activeDialog():
            try: # try to update latest precast values - fails if dialog has been destroyed already
                self.precastvalues = self.precast.getValues()
            except Exception:
                pass
            if self.Height >= self.Length:
                delta = Vector(0,0,self.Height/2)
            else:
                delta = Vector(self.Length/2,0,0)
            delta = self.wp.get_global_coords(delta,as_vector=True)
            if self.modec.isChecked():
                self.tracker.pos(point.add(delta))
                self.tracker.on()
            else:
                if self.bpoint:
                    delta = Vector(0,0,-self.Height/2)
                    delta = self.wp.get_global_coords(delta,as_vector=True)
                    self.tracker.update([self.bpoint.add(delta),point.add(delta)])
                    self.tracker.on()
                    l = (point.sub(self.bpoint)).Length
                    self.vLength.setText(FreeCAD.Units.Quantity(l,FreeCAD.Units.Length).UserString)
                else:
                    self.tracker.off()

    def setWidth(self,d):

        self.Width = d
        self.tracker.width(d)
        params.set_param_arch("StructureWidth",d)

    def setHeight(self,d):

        self.Height = d
        self.tracker.height(d)
        if self.modeb.isChecked():
            params.set_param_arch("StructureLength",d)
        else:
            params.set_param_arch("StructureHeight",d)

    def setLength(self,d):

        self.Length = d
        self.tracker.length(d)
        if self.modeb.isChecked():
            params.set_param_arch("StructureHeight",d)
        else:
            params.set_param_arch("StructureLength",d)

    def setContinue(self,i):

        self.continueCmd = bool(i)
        params.set_param("ContinueMode", bool(i))

    def setCategory(self,i):

        self.vPresets.clear()
        if i > 1:
            self.precast.form.hide()
            self.pSelect = [p for p in Presets if p[1] == Categories[i-2]]
            fpresets = self._createItemlist(self.pSelect)
            self.vPresets.addItems(fpresets)
            self.setPreset(0)
        elif i == 1:
            self.precast.form.show()
            self.pSelect = self.precast.PrecastTypes
            fpresets = self.precast.PrecastTypes
            self.vPresets.addItems(fpresets)
            self.setPreset(0)
        else:
            self.precast.form.hide()
            self.pSelect = [None]
            fpresets = [" "]
            self.vPresets.addItems(fpresets)
            params.set_param_arch("StructurePreset","")

    def setPreset(self,i):

        self.Profile = None
        elt = self.pSelect[i]
        if elt:
            if elt in self.precast.PrecastTypes:
                self.precast.setPreset(elt)
                self.Profile = "Precast_" + elt
                if elt in ["Pillar","Beam"]:
                    self.dents.form.show()
                else:
                    self.dents.form.hide()
                params.set_param_arch("StructurePreset",self.Profile)
            else:
                p=elt[0]-1 # Presets indexes are 1-based
                self.vLength.setText(FreeCAD.Units.Quantity(float(Presets[p][4]),FreeCAD.Units.Length).UserString)
                self.vWidth.setText(FreeCAD.Units.Quantity(float(Presets[p][5]),FreeCAD.Units.Length).UserString)
                self.Profile = Presets[p]
                params.set_param_arch("StructurePreset",";".join([str(i) for i in self.Profile]))

    def switchLH(self,bmode):

        if bmode:
            self.bmode = True
            if self.Height > self.Length:
                self.rotateLH()
        else:
            self.bmode = False
            if self.Length > self.Height:
                self.rotateLH()
                self.tracker.setRotation(FreeCAD.Rotation())

    def rotateLH(self):

        l = self.vLength.text()
        h = self.vHeight.text()
        self.vLength.setText(h)
        self.vHeight.setText(l)

    def rotateLW(self):

        l = self.vLength.text()
        w = self.vWidth.text()
        self.vLength.setText(w)
        self.vWidth.setText(l)


FreeCADGui.addCommand("Arch_Structure", CommandStructure())
FreeCADGui.addCommand("Arch_StructuralSystem", CommandStructuralSystem())
FreeCADGui.addCommand("Arch_StructuresFromSelection", CommandStructuresFromSelection())

class ArchStructureGroupCommand:

    def GetCommands(self):
        return ("Arch_Structure", "Arch_StructuralSystem", "Arch_StructuresFromSelection")
    def GetResources(self):
        return { "MenuText": QT_TRANSLATE_NOOP("Arch_StructureTools", "Structure tools"),
                 "ToolTip": QT_TRANSLATE_NOOP("Arch_StructureTools", "Structure tools")
               }
    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

FreeCADGui.addCommand("Arch_StructureTools", ArchStructureGroupCommand())
