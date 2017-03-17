# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
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

from __future__ import print_function
import FreeCAD
import xml.sax
import FreeCADGui
import Path
import os
from PySide import QtCore, QtGui
import PathScripts
import PathUtils

import PathScripts.PathLog as PathLog

LOG_MODULE = 'PathToolLibraryManager'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
#PathLog.trackModule('PathToolLibraryManager')

try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)


# Tooltable XML readers
class FreeCADTooltableHandler(xml.sax.ContentHandler):
    # http://www.tutorialspoint.com/python/python_xml_processing.htm

    def __init__(self):
        self.tooltable = None
        self.tool = None
        self.number = None

    # Call when an element is found
    def startElement(self, tag, attributes):
        if tag == "Tooltable":
            self.tooltable = Path.Tooltable()
        elif tag == "Toolslot":
            self.number = int(attributes["number"])
        elif tag == "Tool":
            self.tool = Path.Tool()
            self.tool.Name = str(attributes["name"])
            self.tool.ToolType = str(attributes["type"])
            self.tool.Material = str(attributes["mat"])
            # for some reason without the following line I get an error
            #print attributes["diameter"]
            self.tool.Diameter = float(attributes["diameter"])
            self.tool.LengthOffset = float(attributes["length"])
            self.tool.FlatRadius = float(attributes["flat"])
            self.tool.CornerRadius = float(attributes["corner"])
            self.tool.CuttingEdgeAngle = float(attributes["angle"])
            self.tool.CuttingEdgeHeight = float(attributes["height"])

    # Call when an elements ends
    def endElement(self, tag):
        if tag == "Toolslot":
            if self.tooltable and self.tool and self.number:
                self.tooltable.setTool(self.number, self.tool)
                self.number = None
                self.tool = None


class HeeksTooltableHandler(xml.sax.ContentHandler):

    def __init__(self):
        self.tooltable = Path.Tooltable()
        self.tool = None
        self.number = None

    # Call when an element is found
    def startElement(self, tag, attributes):
        if tag == "Tool":
            self.tool = Path.Tool()
            self.number = int(attributes["tool_number"])
            self.tool.Name = str(attributes["title"])
        elif tag == "params":
            t = str(attributes["type"])
            if t == "drill":
                self.tool.ToolType = "Drill"
            elif t == "center_drill_bit":
                self.tool.ToolType = "CenterDrill"
            elif t == "end_mill":
                self.tool.ToolType = "EndMill"
            elif t == "slot_cutter":
                self.tool.ToolType = "SlotCutter"
            elif t == "ball_end_mill":
                self.tool.ToolType = "BallEndMill"
            elif t == "chamfer":
                self.tool.ToolType = "Chamfer"
            elif t == "engraving_bit":
                self.tool.ToolType = "Engraver"
            m = str(attributes["material"])
            if m == "0":
                self.tool.Material = "HighSpeedSteel"
            elif m == "1":
                self.tool.Material = "Carbide"
            # for some reason without the following line I get an error
            #print attributes["diameter"]
            self.tool.Diameter = float(attributes["diameter"])
            self.tool.LengthOffset = float(attributes["tool_length_offset"])
            self.tool.FlatRadius = float(attributes["flat_radius"])
            self.tool.CornerRadius = float(attributes["corner_radius"])
            self.tool.CuttingEdgeAngle = float(
                attributes["cutting_edge_angle"])
            self.tool.CuttingEdgeHeight = float(
                attributes["cutting_edge_height"])

    # Call when an elements ends
    def endElement(self, tag):
        if tag == "Tool":
            if self.tooltable and self.tool and self.number:
                self.tooltable.setTool(self.number, self.tool)
                self.number = None
                self.tool = None


class ToolLibraryManager():
    '''
    The Tool Library is a list of individual tool tables.  Each
    Tool Table can contain n tools.  The tool library will be persisted to user
    preferences and all or part of the library can be exported to other formats
    '''

    def __init__(self):
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return

    def saveMainLibrary(self, tooltable):
        '''Persists the permanent library to FreeCAD user preferences'''
        tmpstring = tooltable.Content
        self.prefs.SetString("ToolLibrary", tmpstring)
        return True

    def getLists(self):
        '''Builds the list of all Tool Table lists'''
        tablelist = []
        toollist = "<Main>"
        tablelist.append(toollist)

        # Get ToolTables from any open CNC jobs
        for o in FreeCAD.ActiveDocument.Objects:
            if hasattr(o, "Proxy"):
                if isinstance(o.Proxy, PathScripts.PathJob.ObjectPathJob):
                    tablelist.append(o.Label)
        return tablelist

    def _findList(self, listname):
        tt = None
        if listname == "<Main>":
            tmpstring = self.prefs.GetString("ToolLibrary", "")
            if tmpstring != "":
                Handler = FreeCADTooltableHandler()
                xml.sax.parseString(tmpstring, Handler)
                tt = Handler.tooltable
            else:
                tt = Path.Tooltable()
        else:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Label == listname:
                    tt = o.Tooltable
        return tt

    def getTool(self, listname, toolnum):
        tt = self._findList(listname)
        return tt.getTool(toolnum)

    def getTools(self, tablename):
        '''returns the tool data for a given table'''
        tooldata = []
        tt = self._findList(tablename)
        headers = ["","Tool Num.","Name","Tool Type","Material","Diameter","Length Offset","Flat Radius","Corner Radius","Cutting Edge Angle","Cutting Edge Height"]
        model = QtGui.QStandardItemModel()
        model.setHorizontalHeaderLabels(headers)

        def unitconv(ivalue):
            parms = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
            digits = parms.GetContents()[1][2] #get user's number of digits of precision
            if parms.GetContents()[0][2]==0:
                suffix = 'mm'
                conversion = 1.0
            elif parms.GetContents()[0][2]==3:
                suffix = 'in'
                conversion = 25.4
            else:
                suffix = ''
            val = FreeCAD.Units.parseQuantity(str(round(ivalue/conversion,digits))+suffix)
            displayed_val = val.UserString #just the displayed value-not the internal one

            return displayed_val

        if tt:
            if len(tt.Tools) == 0:
                tooldata.append([])
            for number, t in tt.Tools.iteritems():

                itemcheck = QtGui.QStandardItem()
                itemcheck.setCheckable(True)
                itemNumber =  QtGui.QStandardItem(str(number))
                itemName =  QtGui.QStandardItem(t.Name)
                itemToolType =  QtGui.QStandardItem(t.ToolType)
                itemMaterial =  QtGui.QStandardItem(t.Material)
                itemDiameter =  QtGui.QStandardItem(unitconv(t.Diameter))
                itemLengthOffset =  QtGui.QStandardItem(unitconv(t.LengthOffset))
                itemFlatRadius =  QtGui.QStandardItem(unitconv(t.FlatRadius))
                itmCornerRadius =  QtGui.QStandardItem(unitconv(t.CornerRadius))
                itemCuttingEdgeAngle =  QtGui.QStandardItem(str(t.CuttingEdgeAngle))
                itemCuttingEdgeHeight =  QtGui.QStandardItem(unitconv(t.CuttingEdgeHeight))

                row = [itemcheck, itemNumber, itemName, itemToolType, itemMaterial, itemDiameter, itemLengthOffset, itemFlatRadius, itmCornerRadius, itemCuttingEdgeAngle, itemCuttingEdgeHeight]
                model.appendRow(row)

        return model

    # methods for importing and exporting
    def read(self, filename, listname):
        "imports a tooltable from a file"
        parser = xml.sax.make_parser()
        parser.setFeature(xml.sax.handler.feature_namespaces, 0)
        if os.path.splitext(filename[0])[1].lower() == ".tooltable":
            Handler = HeeksTooltableHandler()
        else:
            Handler = FreeCADTooltableHandler()

        try:
            parser.setContentHandler(Handler)
            parser.parse(unicode(filename[0]))
            if not Handler.tooltable:
                return None

            ht = Handler.tooltable
            tt = self._findList(listname)
            for t in ht.Tools:
                newt = ht.getTool(t).copy()
                tt.addTools(newt)
            if listname == "<Main>":
                self.saveMainLibrary(tt)
            return True
        except Exception as e:
            print("could not parse file", e)

    def write(self, filename, listname):
        "exports the tooltable to a file"
        tt = self._findList(listname)
        if tt:
            try:
                file = open(unicode(filename[0]), "wb")

                if filename[1] == 'LinuxCNC tooltable (*.tbl)':
                    for key in tt.Tools:
                        t = tt.Tools[key]
                        file.write("T{} P{} Y{} Z{} A{} B{} C{} U{} V{} W{} D{} I{} J{} Q{} ;{}\n".format(key,key,0,t.LengthOffset,0,0,0,0,0,0,t.Diameter,0,0,0,t.Name))

                else:
                    file.write('<?xml version="1.0" encoding="UTF-8"?>\n')
                    file.write(tt.Content)

                file.close()
                print("Written ", unicode(filename[0]))

            except Exception as e:
                print("Could not write file:", e)

    def addnew(self, listname, tool, position = None):
        "adds a new tool at the end of the table"
        print(listname, tool, position)
        tt = self._findList(listname)
        if position is None:
            tt.addTools(tool)
            newID = tt.Tools.keys()[-1]
        else:
            tt.setTool(position, tool)
            newID = position

        if listname == "<Main>":
            return self.saveMainLibrary(tt)
        return newID

    def updateTool(self, listname, toolnum, tool):
        '''updates tool data'''
        tt = self._findList(listname)
        tt.deleteTool(toolnum)
        tt.setTool(toolnum, tool)
        if listname == "<Main>":
            return self.saveMainLibrary(tt)
        return True

    def moveup(self, number, listname):
        "moves a tool to a lower number, if possible"
        if number < 2:
            return False
        target = number - 1
        tt = self._findList(listname)

        t1 = tt.getTool(number).copy()
        tt.deleteTool(number)
        if target in tt.Tools.keys():
            t2 = tt.getTool(target).copy()
            tt.deleteTool(target)
            tt.setTool(number, t2)
        tt.setTool(target, t1)
        if listname == "<Main>":
            self.saveMainLibrary(tt)
        return True

    def movedown(self, number, listname):
        "moves a tool to a higher number, if possible"
        tt = self._findList(listname)
        target = number + 1
        t1 = tt.getTool(number).copy()
        tt.deleteTool(number)
        if target in tt.Tools.keys():
            t2 = tt.getTool(target).copy()
            tt.deleteTool(target)
            tt.setTool(number, t2)
        tt.setTool(target, t1)
        if listname == "<Main>":
            self.saveMainLibrary(tt)
        return True

    def delete(self, number, listname):
        '''deletes a tool from the current list'''
        tt = self._findList(listname)
        tt.deleteTool(number)
        if listname == "<Main>":
            self.saveMainLibrary(tt)
        return True


class EditorPanel():
    def __init__(self):
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ToolLibraryEditor.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolLibraryEditor.ui")
        #self.editform = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ToolEdit.ui")
        self.editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")
        self.TLM = ToolLibraryManager()

        self.loadTable()
        self.form.ToolsList.resizeColumnsToContents()

    def accept(self):
        pass

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        pass

    def getType(self, tooltype):
        "gets a combobox index number for a given type or viceversa"
        toolslist = ["Drill", "CenterDrill", "CounterSink", "CounterBore",
                     "Reamer", "Tap", "EndMill", "SlotCutter", "BallEndMill",
                     "ChamferMill", "CornerRound", "Engraver"]
        if isinstance(tooltype, str):
            if tooltype in toolslist:
                return toolslist.index(tooltype)
            else:
                return 0
        else:
            return toolslist[tooltype]

    def getMaterial(self, material):
        "gets a combobox index number for a given material or viceversa"
        matslist = ["HighSpeedSteel", "HighCarbonToolSteel", "CastAlloy",
                    "Carbide", "Ceramics", "Diamond", "Sialon"]
        if isinstance(material, str):
            if material in matslist:
                return matslist.index(material)
            else:
                return 0
        else:
            return matslist[material]

    def addTool(self):
        t = Path.Tool()
        editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")

        r = editform.exec_()
        if r:
            if editform.NameField.text():
                t.Name = str(editform.NameField.text()) #FIXME: not unicode safe!
            t.ToolType = self.getType(editform.TypeField.currentIndex())
            t.Material = self.getMaterial(editform.MaterialField.currentIndex())
            t.Diameter = FreeCAD.Units.parseQuantity(editform.DiameterField.text())
            t.LengthOffset = FreeCAD.Units.parseQuantity(editform.LengthOffsetField.text())
            t.FlatRadius = FreeCAD.Units.parseQuantity(editform.FlatRadiusField.text())
            t.CornerRadius = FreeCAD.Units.parseQuantity(editform.CornerRadiusField.text())
            t.CuttingEdgeAngle = FreeCAD.Units.Quantity(editform.CuttingEdgeAngleField.text())
            t.CuttingEdgeHeight = FreeCAD.Units.parseQuantity(editform.CuttingEdgeHeightField.text())

            listname = "<Main>"

            if self.TLM.addnew(listname, t) is True:
                self.loadTable()

    def setFields(self):
        pass

    def open(self):
        pass

    def loadTable(self):
        #tooldata = self.TLM.getTools(curr.data())
        tooldata = self.TLM.getTools("<Main>")
        self.form.ToolsList.setModel(tooldata)

    def moveUp(self):
        "moves a tool to a lower number, if possible"
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = "<Main>"
            #listname = self.form.listView.selectedIndexes()[0].data()
            if self.TLM.moveup(number, listname) is True:
                self.loadTable()

    def moveDown(self):
        "moves a tool to a higher number, if possible"
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = "<Main>"
            #listname = self.form.listView.selectedIndexes()[0].data()
            if self.TLM.movedown(number, listname) is True:
                self.loadTable()

    def delete(self):
        '''deletes a tool'''
        #listname  =  self.form.listView.selectedIndexes()[0].data()
        listname = "<Main>"
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                t = model.index(i, 1)
                self.TLM.delete(int(t.data()) ,listname)
        self.loadTable()

    def editTool(self, currItem):

        row = currItem.row()
        value = currItem.sibling(row, 1).data()
        #listname = self.form.listView.selectedIndexes()[0].data()
        listname = "<Main>"
        toolnum = int(value)
        tool = self.TLM.getTool(listname, toolnum)
        editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")

        editform.NameField.setText(tool.Name)
        editform.TypeField.setCurrentIndex(self.getType(tool.ToolType))
        editform.MaterialField.setCurrentIndex(self.getMaterial(tool.Material))
        editform.DiameterField.setText(FreeCAD.Units.Quantity(tool.Diameter, FreeCAD.Units.Length).UserString)
        editform.LengthOffsetField.setText(FreeCAD.Units.Quantity(tool.LengthOffset, FreeCAD.Units.Length).UserString)
        editform.FlatRadiusField.setText(FreeCAD.Units.Quantity(tool.FlatRadius, FreeCAD.Units.Length).UserString)
        editform.CornerRadiusField.setText(FreeCAD.Units.Quantity(tool.CornerRadius, FreeCAD.Units.Length).UserString)
        editform.CuttingEdgeAngleField.setText(FreeCAD.Units.Quantity(tool.CuttingEdgeAngle, FreeCAD.Units.Angle).UserString)
        editform.CuttingEdgeHeightField.setText(FreeCAD.Units.Quantity(tool.CuttingEdgeHeight, FreeCAD.Units.Length).UserString)

        r = editform.exec_()
        if r:
            if editform.NameField.text():
                tool.Name = str(editform.NameField.text()) #FIXME: not unicode safe!
            tool.ToolType = self.getType(editform.TypeField.currentIndex())
            tool.Material = self.getMaterial(editform.MaterialField.currentIndex())
            tool.Diameter = FreeCAD.Units.parseQuantity(editform.DiameterField.text())
            tool.LengthOffset = FreeCAD.Units.parseQuantity(editform.LengthOffsetField.text())
            tool.FlatRadius = FreeCAD.Units.parseQuantity(editform.FlatRadiusField.text())
            tool.CornerRadius = FreeCAD.Units.parseQuantity(editform.CornerRadiusField.text())
            tool.CuttingEdgeAngle = FreeCAD.Units.Quantity(editform.CuttingEdgeAngleField.text())
            tool.CuttingEdgeHeight = FreeCAD.Units.parseQuantity(editform.CuttingEdgeHeightField.text())

            if self.TLM.updateTool(listname, toolnum, tool) is True:
                self.loadTable()

    def importFile(self):
        "imports a tooltable from a file"
        filename = QtGui.QFileDialog.getOpenFileName(self.form, _translate( "TooltableEditor", "Open tooltable", None), None, _translate("TooltableEditor", "Tooltable XML (*.xml);;HeeksCAD tooltable (*.tooltable)", None))
        if filename[0]:
            listname = self.form.listView.selectedIndexes()[0].data()
            if self.TLM.read(filename, listname):
                self.loadTable(self.form.listView.selectedIndexes()[0])


    def exportFile(self):
        "imports a tooltable from a file"
        filename = QtGui.QFileDialog.getSaveFileName(self.form, _translate("TooltableEditor", "Save tooltable", None), None, _translate("TooltableEditor", "Tooltable XML (*.xml);;LinuxCNC tooltable (*.tbl)", None))

        if filename[0]:
            #listname = self.form.listView.selectedIndexes()[0].data()
            listname = '<Main>'
            self.TLM.write(filename, listname)

    def checkCopy(self):
        self.form.btnCopyTools.setEnabled(False)
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                self.form.btnCopyTools.setEnabled(True)

        if len(PathUtils.GetJobs()) == 0:
            self.form.btnCopyTools.setEnabled(False)

    def copyTools(self):
        tools = []
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                item = model.index(i, 1)
                tools.append(item.data())
        if len(tools) == 0:
            return

        targets = self.TLM.getLists()
        currList = "<Main>"

        for target in targets:
            if target == currList:
                targets.remove(target)

        if len(targets) == 0:
            FreeCAD.Console.PrintWarning("no place to go")
            return
        elif len(targets) == 1:
            targetlist = targets[0]
        else:
            form = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolCopy.ui")
            form.cboTarget.addItems(targets)
            r = form.exec_()
            if r is False:
                return None
            else:
                targetlist = form.cboTarget.currentText()

        for toolnum in tools:
            tool = self.TLM.getTool(currList, int(toolnum))
            PathLog.debug('tool: {}, toolnum: {}'.format(tool, toolnum))
            for i in FreeCAD.ActiveDocument.findObjects("Path::Feature"):
                if isinstance(i.Proxy, PathScripts.PathJob.ObjectPathJob) and i.Label == targetlist:

                    label = "T{}: {}".format(toolnum, tool.Name)
                    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython",label)
                    PathScripts.PathLoadTool.LoadTool(obj)
                    PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)
                    PathUtils.addToJob(obj, targetlist)
                    FreeCAD.activeDocument().recompute()
                    newtool = tool.copy()
                    obj.Tooltable.setTool(int(toolnum), newtool)
                    obj.ToolNumber = int(toolnum)
                    #obj.recompute()
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        # Connect Signals and Slots
        self.form.ButtonNewTool.clicked.connect(self.addTool)
        self.form.ButtonImport.clicked.connect(self.importFile)
        self.form.ButtonExport.clicked.connect(self.exportFile)
        self.form.ButtonDown.clicked.connect(self.moveDown)
        self.form.ButtonUp.clicked.connect(self.moveUp)
        self.form.ButtonDelete.clicked.connect(self.delete)
        self.form.ToolsList.doubleClicked.connect(self.editTool)
        self.form.ToolsList.clicked.connect(self.checkCopy)
        self.form.btnCopyTools.clicked.connect(self.copyTools)

        self.form.btnCopyTools.setEnabled(False)

        self.setFields()

class CommandToolLibraryEdit():
    def edit(self):
        editor = EditorPanel()
        editor.setupUi()

        r = editor.form.exec_()
        if r:
            pass

    def GetResources(self):
        return {'Pixmap'  : 'Path-ToolTable',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolTable","Tool Manager"),
                'Accel': "P, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolTable","Tool Manager")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        self.edit()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolLibraryEdit',CommandToolLibraryEdit())

