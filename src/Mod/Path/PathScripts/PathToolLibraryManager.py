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

import FreeCAD
import xml.sax
import FreeCADGui
import Path
import os
from PySide import QtCore, QtGui

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
        # self.ToolLibrary = []
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
            if "Proxy" in o.PropertiesList:
                if hasattr(o, "Tooltable"):
                    toollist = o.Label
                    tablelist.append(toollist)
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
                itemDiameter =  QtGui.QStandardItem(str(t.Diameter))
                itemLengthOffset =  QtGui.QStandardItem(str(t.LengthOffset))
                itemFlatRadius =  QtGui.QStandardItem(str(t.FlatRadius))
                itmCornerRadius =  QtGui.QStandardItem(str(t.CornerRadius))
                itemCuttingEdgeAngle =  QtGui.QStandardItem(str(t.CuttingEdgeAngle))
                itemCuttingEdgeHeight =  QtGui.QStandardItem(str(t.CuttingEdgeHeight))

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
        parser.setContentHandler(Handler)
        parser.parse(str(filename[0]))
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

    def write(self, filename, listname):
        "exports the tooltable to a file"
        tt = self._findList(listname)
        if tt:
            fil = open(str(filename[0]), "wb")
            fil.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            fil.write(tt.Content)
            fil.close()
            print "Written ", filename[0]

    def addnew(self, listname, tool, position = None):
        "adds a new tool at the end of the table"
        print listname, tool, position
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

    # def createToolController(self, job, tool):
    #     pass

    # def exportListHeeks(self, tooltable):
    #     '''exports one or more Lists as a HeeksCNC tooltable'''
    #     pass

    # def exportListLinuxCNC(self, tooltable):
    #     '''exports one or more Lists as a LinuxCNC tooltable'''
    #     pass

    # def exportListXML(self, tooltable):
    #     '''exports one or more Lists as an XML file'''
    #     pass

class EditorPanel():
    def __init__(self):
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ToolLibraryEditor.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolLibraryEditor.ui")
        #self.editform = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ToolEdit.ui")
        self.editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")
        self.TLM = ToolLibraryManager()

        data = self.TLM.getLists()
        self.listmodel = QtGui.QStandardItemModel(self.form.listView)
#        self.listmodel = QtGui.QStringListModel(data)
        for i in data:
            item = QtGui.QStandardItem(i)
            self.listmodel.appendRow(item)

        self.form.listView.setModel(self.listmodel)
        #self.form.listView.setCurrentIndex(0)
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
            #if tooltype == 0:
            #    return "Undefined"
            #else:
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
            #if material == 0:
            #    return "Undefined"
            #else:
            return matslist[material]

    def addTool(self):
        t = Path.Tool()
        print ("adding a new tool")
        editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")

        r = editform.exec_()
        if r:
            if editform.NameField.text():
                t.Name = str(editform.NameField.text())
            t.ToolType = self.getType(editform.TypeField.currentIndex())
            t.Material = self.getMaterial(editform.MaterialField.currentIndex())
            t.Diameter = editform.DiameterField.value()
            t.LengthOffset = editform.LengthOffsetField.value()
            t.FlatRadius = editform.FlatRadiusField.value()
            t.CornerRadius = editform.CornerRadiusField.value()
            t.CuttingEdgeAngle = editform.CuttingEdgeAngleField.value()
            t.CuttingEdgeHeight = editform.CuttingEdgeHeightField.value()

            listname = self.form.listView.selectedIndexes()[0].data()
            if self.TLM.addnew(listname, t) is True:
                self.loadTable(self.form.listView.selectedIndexes()[0])

    def setFields(self):
        index = self.listmodel.index(0, 0, QtCore.QModelIndex())
        self.form.listView.setFocus()
        sm = self.form.listView.selectionModel()
        sm.select(index, sm.Select)

    def open(self):
        pass

    def loadTable(self, curr):
        tooldata = self.TLM.getTools(curr.data())
        self.form.ToolsList.setModel(tooldata)

    def moveUp(self):
        "moves a tool to a lower number, if possible"
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.form.listView.selectedIndexes()[0].data()

            if self.TLM.moveup(number, listname) is True:
                self.loadTable(self.form.listView.selectedIndexes()[0])

    def moveDown(self):
        "moves a tool to a higher number, if possible"
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.form.listView.selectedIndexes()[0].data()
            if self.TLM.movedown(number, listname) is True:
                self.loadTable(self.form.listView.selectedIndexes()[0])

    def delete(self):
        '''deletes a tool'''
        listname  =  self.form.listView.selectedIndexes()[0].data()
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                t = model.index(i, 1)
                self.TLM.delete(int(t.data()) ,listname)
        self.loadTable(self.form.listView.selectedIndexes()[0])

    def editTool(self, currItem):

        row = currItem.row()
        value = currItem.sibling(row, 1).data()
        listname = self.form.listView.selectedIndexes()[0].data()
        toolnum = int(value)
        tool = self.TLM.getTool(listname, toolnum)
        editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")

        editform.NameField.setText(tool.Name)
        editform.TypeField.setCurrentIndex(self.getType(tool.ToolType))
        editform.MaterialField.setCurrentIndex(self.getMaterial(tool.Material))
        editform.DiameterField.setValue(tool.Diameter)
        editform.LengthOffsetField.setValue(tool.LengthOffset)
        editform.FlatRadiusField.setValue(tool.FlatRadius)
        editform.CornerRadiusField.setValue(tool.CornerRadius)
        editform.CuttingEdgeAngleField.setValue(tool.CuttingEdgeAngle)
        editform.CuttingEdgeHeightField.setValue(tool.CuttingEdgeHeight)

        r = editform.exec_()
        if r:
            if editform.NameField.text():
                tool.Name = str(editform.NameField.text())
            tool.ToolType = self.getType(editform.TypeField.currentIndex())
            tool.Material = self.getMaterial(editform.MaterialField.currentIndex())
            tool.Diameter = editform.DiameterField.value()
            tool.LengthOffset = editform.LengthOffsetField.value()
            tool.FlatRadius = editform.FlatRadiusField.value()
            tool.CornerRadius = editform.CornerRadiusField.value()
            tool.CuttingEdgeAngle = editform.CuttingEdgeAngleField.value()
            tool.CuttingEdgeHeight = editform.CuttingEdgeHeightField.value()

            if self.TLM.updateTool(listname, toolnum, tool) is True:
                self.loadTable(self.form.listView.selectedIndexes()[0])

    def importFile(self):
        "imports a tooltable from a file"
        filename = QtGui.QFileDialog.getOpenFileName(self.form, _translate(
                "TooltableEditor", "Open tooltable", None), None, _translate("TooltableEditor", "Tooltable XML (*.xml);;HeeksCAD tooltable (*.tooltable)", None))
        if filename:
            listname = self.form.listView.selectedIndexes()[0].data()
            if self.TLM.read(filename, listname):
                self.loadTable(self.form.listView.selectedIndexes()[0])


    def exportFile(self):
        "imports a tooltable from a file"
        filename = QtGui.QFileDialog.getSaveFileName(self.form, _translate("TooltableEditor", "Save tooltable", None), None, _translate("TooltableEditor", "Tooltable XML (*.xml)", None))

        if filename:
            listname = self.form.listView.selectedIndexes()[0].data()
            self.TLM.write(filename, listname)

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
        currList = self.form.listView.selectedIndexes()[0].data()

        for target in targets:
            if target == currList:
                targets.remove(target)

        if len(targets) == 0:
            FreeCAD.Console.PrintWarning("no place to go")
            return

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
                    newtoolid = self.TLM.addnew(targetlist, tool.copy(), int(toolnum))
                    if form.chkMakeController.checkState() == QtCore.Qt.CheckState.Checked and targetlist != "<Main>":
                        snippet = '''
import Path, PathScripts
from PathScripts import PathUtils, PathLoadTool
obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","TC")
PathScripts.PathLoadTool.LoadTool(obj)
PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)
obj.ToolNumber = %d
PathUtils.addToJob(obj, "%s")
App.activeDocument().recompute()
''' % (newtoolid, targetlist)
                        FreeCADGui.doCommand(snippet)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        # Connect Signals and Slots
        self.form.ButtonNewTool.clicked.connect(self.addTool)
        #self.form.listWidget.currentItemChanged.connect(self.loadTable)
        sm = self.form.listView.selectionModel()
        sm.currentChanged.connect(self.loadTable)
        self.form.ButtonImport.clicked.connect(self.importFile)
        self.form.ButtonExport.clicked.connect(self.exportFile)
        self.form.ButtonDown.clicked.connect(self.moveDown)
        self.form.ButtonUp.clicked.connect(self.moveUp)
        self.form.ButtonDelete.clicked.connect(self.delete)
        self.form.ToolsList.doubleClicked.connect(self.editTool)
        self.form.btnCopyTools.clicked.connect(self.copyTools)

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
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolTable","Edit the Tool Library"),
                'Accel': "P, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolTable","Edit the Tool Library")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        self.edit()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolLibraryEdit',CommandToolLibraryEdit())

