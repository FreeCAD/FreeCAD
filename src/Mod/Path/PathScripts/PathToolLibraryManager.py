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
import FreeCADGui
import Path
import PathScripts
import PathScripts.PathLog as PathLog
import PathScripts.PathToolEdit as PathToolEdit
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
import json
import os
import xml.sax

from PySide import QtCore, QtGui

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

# Tooltable XML readers
class FreeCADTooltableHandler(xml.sax.ContentHandler):
    # http://www.tutorialspoint.com/python/python_xml_processing.htm

    def __init__(self):
        xml.sax.ContentHandler.__init__(self)
        self.tooltable = None
        self.tool = None
        self.number = None

    # Call when an element is found
    def startElement(self, name, attrs):
        if name == "Tooltable":
            self.tooltable = Path.Tooltable()
        elif name == "Toolslot":
            self.number = int(attrs["number"])
        elif name == "Tool":
            self.tool = Path.Tool()
            self.tool.Name = str(attrs["name"])
            self.tool.ToolType = str(attrs["type"])
            self.tool.Material = str(attrs["mat"])
            # for some reason without the following line I get an error
            #print attrs["diameter"]
            self.tool.Diameter = float(attrs["diameter"])
            self.tool.LengthOffset = float(attrs["length"])
            self.tool.FlatRadius = float(attrs["flat"])
            self.tool.CornerRadius = float(attrs["corner"])
            self.tool.CuttingEdgeAngle = float(attrs["angle"])
            self.tool.CuttingEdgeHeight = float(attrs["height"])

    # Call when an elements ends
    def endElement(self, name):
        if name == "Toolslot":
            if self.tooltable and self.tool and self.number:
                self.tooltable.setTool(self.number, self.tool)
                self.number = None
                self.tool = None


class HeeksTooltableHandler(xml.sax.ContentHandler):

    def __init__(self):
        xml.sax.ContentHandler.__init__(self)
        self.tooltable = Path.Tooltable()
        self.tool = None
        self.number = None

    # Call when an element is found
    def startElement(self, name, attrs):
        if name == "Tool":
            self.tool = Path.Tool()
            self.number = int(attrs["tool_number"])
            self.tool.Name = str(attrs["title"])
        elif name == "params":
            t = str(attrs["type"])
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
            m = str(attrs["material"])
            if m == "0":
                self.tool.Material = "HighSpeedSteel"
            elif m == "1":
                self.tool.Material = "Carbide"
            # for some reason without the following line I get an error
            #print attrs["diameter"]
            self.tool.Diameter = float(attrs["diameter"])
            self.tool.LengthOffset = float(attrs["tool_length_offset"])
            self.tool.FlatRadius = float(attrs["flat_radius"])
            self.tool.CornerRadius = float(attrs["corner_radius"])
            self.tool.CuttingEdgeAngle = float(
                attrs["cutting_edge_angle"])
            self.tool.CuttingEdgeHeight = float(
                attrs["cutting_edge_height"])

    # Call when an elements ends
    def endElement(self, name):
        if name == "Tool":
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

    #TODO: copy & Duplicate tools between lists

    TooltableTypeJSON     = translate("TooltableEditor", "Tooltable JSON (*.json)")
    TooltableTypeXML      = translate("TooltableEditor", "Tooltable XML (*.xml)")
    TooltableTypeHeekscad = translate("TooltableEditor", "HeeksCAD tooltable (*.tooltable)")
    TooltableTypeLinuxCNC = translate("TooltableEditor", "LinuxCNC tooltable (*.tbl)")

    PreferenceMainLibraryXML = "ToolLibrary"
    PreferenceMainLibraryJSON = "ToolLibrary-Main"

    def __init__(self):
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        self.toolTables = []
        self.currentTableName = None
        self.loadToolTables()
        
        if len(self.toolTables):
            self.currentTableName = self.toolTables[0].Name

        return

    def getToolTables(self):
        ''' Return tool table list '''
        return self.toolTables

    def getCurrentTableName(self):
        ''' return the name of the currently loaded tool table '''
        return self.currentTableName

    def getCurrentTable(self):
        ''' returns an object of the current tool table '''
        return self.getTableFromName(self.currentTableName)
    
    def getTableFromName(self, name):
        ''' get the tool table object from the name '''
        for table in self.toolTables:
            if table.Name == name:
                return table

    def getNextToolTableName(self, tableName='Tool Table'):
        ''' get a unique name for a new tool table '''
        iter = 1
        tempName = tableName[-2:] 

        if tempName[0] == '-' and tempName[-1].isdigit():
            tableName = tableName[:-2]

        while any(table.Name == tableName + '-' + str(iter) for table in self.toolTables):
            iter += 1              
        
        return tableName + '-' + str(iter) 

    def addNewToolTable(self):
        ''' creates a new tool table '''
        tt = Path.Tooltable()
        tt.Version = 1
        name = self.getNextToolTableName()
        tt.Name = name
        self.toolTables.append(tt)
        self.saveMainLibrary()
        return name

    def deleteToolTable(self):
        ''' deletes the selected tool table '''
        index = next((index for (index, d) in enumerate(self.toolTables) if d.Name == self.currentTableName), None) 
        self.toolTables.pop(index)
        self.saveMainLibrary()

    def renameToolTable(self, newName, index):
        ''' renames a tool table with the new name'''
        currentTableName = self.toolTables[index].Name
        if newName == currentTableName:
            PathLog.error(translate('PathToolLibraryManager', "Tool Table Same Name"))
            return False
        if newName in self.toolTables:
            PathLog.error(translate('PathToolLibraryManager', "Tool Table Name Exists"))
            return False
        tt = self.getTableFromName(currentTableName)
        if tt:
            tt.Name = newName
            self.saveMainLibrary()
            return True
        

    def templateAttrs(self):
        ''' gets the tool table arributes '''
        toolTables = []
        for tt in self.toolTables:
            tableData = {}
            tableData['Version'] = 1
            tableData['TableName'] = tt.Name

            toolData = {}
            for tool in tt.Tools:
                toolData[tool] = tt.Tools[tool].templateAttrs()

            tableData['Tools'] = toolData
            toolTables.append(tableData)
                  
        return toolTables

    def tooltableFromAttrs(self, stringattrs):
        if stringattrs.get('Version') and 1 == int(stringattrs['Version']):
            
            tt = Path.Tooltable()
            tt.Version = 1
            tt.Name = self.getNextToolTableName()

            if stringattrs.get('Version'):
                tt.Version = stringattrs.get('Version')

            if stringattrs.get('TableName'):
                tt.Name = stringattrs.get('TableName')
                if any(table.Name == tt.Name for table in self.toolTables):
                    tt.Name = self.getNextToolTableName(tt.Name)
            
            for key, attrs in PathUtil.keyValueIter(stringattrs['Tools']):
                    tool = Path.Tool() 
                    tool.Name = str(attrs["name"])
                    tool.ToolType = str(attrs["tooltype"])
                    tool.Material = str(attrs["material"])
                    tool.Diameter = float(attrs["diameter"])
                    tool.LengthOffset = float(attrs["lengthOffset"])
                    tool.FlatRadius = float(attrs["flatRadius"])
                    tool.CornerRadius = float(attrs["cornerRadius"])
                    tool.CuttingEdgeAngle = float(attrs["cuttingEdgeAngle"])
                    tool.CuttingEdgeHeight = float(attrs["cuttingEdgeHeight"])
                    tt.setTool(int(key), tool)

            return tt
        else:
            PathLog.error(translate('PathToolLibraryManager', "Unsupported Path tooltable template version %s") % stringattrs.get('Version'))
        return None

    def loadToolTables(self):
        ''' loads the tool tables from the stored data '''
        self.toolTables = []
        self.currentTableName = ''

        def addTable(tt):
            if tt:
                self.toolTables.append(tt)
            else:
                PathLog.error(translate('PathToolLibraryManager', "Unsupported Path tooltable"))

        prefsData = json.loads(self.prefs.GetString(self.PreferenceMainLibraryJSON, ""))

        if isinstance(prefsData, dict):
            tt = self.tooltableFromAttrs(prefsData)
            addTable(tt)

        if isinstance(prefsData, list):
            for table in prefsData:
                tt = self.tooltableFromAttrs(table)
                addTable(tt)

    def saveMainLibrary(self):
        '''Persists the permanent library to FreeCAD user preferences'''
        tmpstring = json.dumps(self.templateAttrs())
        self.prefs.SetString(self.PreferenceMainLibraryJSON, tmpstring)
        self.loadToolTables()
        return True

    def getJobList(self):
        '''Builds the list of all Tool Table lists'''
        tablelist = []

        for o in FreeCAD.ActiveDocument.Objects:
            if hasattr(o, "Proxy"):
                if isinstance(o.Proxy, PathScripts.PathJob.ObjectJob):
                    tablelist.append(o.Label)
                    
        return tablelist

    def getTool(self, listname, toolnum):
        ''' gets the tool object '''
        tt = self.getTableFromName(listname)
        return tt.getTool(toolnum)

    def getTools(self, tablename):
        '''returns the tool data for a given table'''
        tooldata = []
        tableExists =  any(table.Name == tablename for table in self.toolTables)
        if tableExists:
            self.currentTableName = tablename
        else:
            return None

        tt = self.getTableFromName(tablename)
        headers = ["","Tool Num.","Name","Tool Type","Material","Diameter","Length Offset","Flat Radius","Corner Radius","Cutting Edge Angle","Cutting Edge Height"]
        model = QtGui.QStandardItemModel()
        model.setHorizontalHeaderLabels(headers)

        def unitconv(ivalue):
            val = FreeCAD.Units.Quantity(ivalue, FreeCAD.Units.Length)
            displayed_val = val.UserString      #just the displayed value-not the internal one
            return displayed_val

        if tt:
            if len(tt.Tools) == 0:
                tooldata.append([])
            for number, t in PathUtil.keyValueIter(tt.Tools):

                itemcheck = QtGui.QStandardItem()
                itemcheck.setCheckable(True)
                itemNumber =  QtGui.QStandardItem(str(number))
                itemName =  QtGui.QStandardItem(t.Name)
                itemToolType =  QtGui.QStandardItem(t.ToolType)
                itemMaterial =  QtGui.QStandardItem(t.Material)
                itemDiameter =  QtGui.QStandardItem(unitconv(t.Diameter))
                itemLengthOffset =  QtGui.QStandardItem(unitconv(t.LengthOffset))
                itemFlatRadius =  QtGui.QStandardItem(unitconv(t.FlatRadius))
                itemCornerRadius =  QtGui.QStandardItem(unitconv(t.CornerRadius))
                itemCuttingEdgeAngle =  QtGui.QStandardItem(str(t.CuttingEdgeAngle))
                itemCuttingEdgeHeight =  QtGui.QStandardItem(unitconv(t.CuttingEdgeHeight))

                row = [itemcheck, itemNumber, itemName, itemToolType, itemMaterial, itemDiameter, itemLengthOffset, itemFlatRadius, itemCornerRadius, itemCuttingEdgeAngle, itemCuttingEdgeHeight]
                model.appendRow(row)

        return model

    # methods for importing and exporting
    def read(self, filename, listname):
        "imports a tooltable from a file"

        importedTables = []

        try:
            fileExtension = os.path.splitext(filename[0])[1].lower()
            xmlHandler = None
            if fileExtension == '.tooltable':
                xmlHandler = HeeksTooltableHandler()
            if fileExtension == '.xml':
                xmlHandler = FreeCADTooltableHandler()

            if xmlHandler:
                parser = xml.sax.make_parser()
                parser.setFeature(xml.sax.handler.feature_namespaces, 0)
                parser.setContentHandler(xmlHandler)
                parser.parse(PathUtil.toUnicode(filename[0]))
                if not xmlHandler.tooltable:
                    return None

                ht = xmlHandler.tooltable
            else:
                with open(PathUtil.toUnicode(filename[0]), "rb") as fp:
                    tableData = json.load(fp)

                    if isinstance(tableData, dict):
                        ht = self.tooltableFromAttrs(tableData)
                        if ht:
                            importedTables.append(ht)

                    if isinstance(tableData, list):
                        for table in tableData:
                            ht = self.tooltableFromAttrs(table)                 
                            if ht:
                                importedTables.append(ht)

            if importedTables:
                for tt in importedTables:
                    self.toolTables.append(tt)
                
                self.saveMainLibrary()
                return True
            else:
                return False

        except Exception as e: # pylint: disable=broad-except
            print("could not parse file", e)


    def write(self, filename, listname):
        "exports the tooltable to a file"
        tt = self.getTableFromName(listname)
        if tt:
            try:
                def openFileWithExtension(name, ext):
                    fext = os.path.splitext(name)[1].lower()
                    if fext != ext:
                        name = "{}{}".format(name, ext)
                    return (open(PathUtil.toUnicode(name), 'w'), name)

                if filename[1] == self.TooltableTypeXML:
                    fp,fname = openFileWithExtension(filename[0], '.xml')
                    fp.write('<?xml version="1.0" encoding="UTF-8"?>\n')
                    fp.write(tt.Content)
                elif filename[1] == self.TooltableTypeLinuxCNC:
                    fp,fname = openFileWithExtension(filename[0], '.tbl')
                    for key in tt.Tools:
                        t = tt.Tools[key]
                        fp.write("T{0} P{0} Y{1} Z{2} A{3} B{4} C{5} U{6} V{7} W{8} D{9} I{10} J{11} Q{12} ;{13}\n".format(key,0,t.LengthOffset,0,0,0,0,0,0,t.Diameter,0,0,0,t.Name))
                else:
                    fp,fname = openFileWithExtension(filename[0], '.json')
                    json.dump(self.templateAttrs(), fp, sort_keys=True, indent=2)

                fp.close()
                print("Written ", PathUtil.toUnicode(fname))

            except Exception as e: # pylint: disable=broad-except
                print("Could not write file:", e)

    def addnew(self, listname, tool, position = None):
        "adds a new tool at the end of the table"
        tt = self.getTableFromName(listname)
        if not tt:
            tt = Path.Tooltable()
        if position is None:
            tt.addTools(tool)
            newID = list(tt.Tools)[-1]
        else:
            tt.setTool(position, tool)
            newID = position

        if listname == self.getCurrentTableName():
            self.saveMainLibrary()
        return newID

    def updateTool(self, listname, toolnum, tool):
        '''updates tool data'''
        tt = self.getTableFromName(listname)
        tt.deleteTool(toolnum)
        tt.setTool(toolnum, tool)
        if listname == self.getCurrentTableName():
            return self.saveMainLibrary()
        return True

    def moveup(self, number, listname):
        "moves a tool to a lower number, if possible"
        target = number - 1
        if number < 2:
            return False, target
        target = number - 1
        tt = self.getTableFromName(listname)
        t1 = tt.getTool(number).copy()
        tt.deleteTool(number)
        if target in tt.Tools.keys():
            t2 = tt.getTool(target).copy()
            tt.deleteTool(target)
            tt.setTool(number, t2)
        tt.setTool(target, t1)
        if listname == self.getCurrentTableName():
            self.saveMainLibrary()
        return True, target

    def movedown(self, number, listname):
        "moves a tool to a higher number, if possible"
        tt = self.getTableFromName(listname)
        target = number + 1
        t1 = tt.getTool(number).copy()
        tt.deleteTool(number)
        if target in tt.Tools.keys():
            t2 = tt.getTool(target).copy()
            tt.deleteTool(target)
            tt.setTool(number, t2)
        tt.setTool(target, t1)
        if listname == self.getCurrentTableName():
            self.saveMainLibrary()
        return True, target

    def delete(self, number, listname):
        '''deletes a tool from the current list'''
        tt = self.getTableFromName(listname)
        tt.deleteTool(number)
        if listname == self.getCurrentTableName():
            self.saveMainLibrary()
        return True


class EditorPanel():

    def __init__(self, job, cb):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolLibraryEditor.ui")
        self.TLM = ToolLibraryManager()
        listname = self.TLM.getCurrentTableName()
        
        if listname:
            self.loadToolTables()

        self.job = job
        self.cb = cb

    def toolEditor(self, tool):
        dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolEdit.ui")
        editor = PathToolEdit.ToolEditor(tool, dialog.toolEditor, dialog)
        editor.setupUI()
        return editor

    def accept(self):
        pass

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        pass

    def setFields(self):
        pass

    def open(self):
        pass

    def getType(self, tooltype):
        "gets a combobox index number for a given type or viceversa"
        toolslist = Path.Tool.getToolTypes(Path.Tool())
        if isinstance(tooltype, str):
            if tooltype in toolslist:
                return toolslist.index(tooltype)
            else:
                return 0
        else:
            return toolslist[tooltype]

    def getMaterial(self, material):
        '''gets a combobox index number for a given material or viceversa'''
        matslist = Path.Tool.getToolMaterials(Path.Tool())
        if isinstance(material, str):
            if material in matslist:
                return matslist.index(material)
            else:
                return 0
        else:
            return matslist[material]

    def addTool(self):
        '''adds new tool to the current tool table'''
        tool = Path.Tool()
        editor = self.toolEditor(tool)

        r = editor.Parent.exec_()
        if r:
            editor.accept()
            listname = self.TLM.getCurrentTableName()
            self.TLM.addnew(listname, editor.Tool) # is True:
            self.loadTable(listname)

    def delete(self):
        '''deletes the selected tool'''
        #listname  =  self.form.listView.selectedIndexes()[0].data()
        listname = self.TLM.getCurrentTableName()
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                t = model.index(i, 1)
                self.TLM.delete(int(t.data()) ,listname)
        self.loadTable(listname)

    def editTool(self, currItem):
        '''load the tool edit dialog'''
        row = currItem.row()
        value = currItem.sibling(row, 1).data()
        listname = self.TLM.getCurrentTableName()
        toolnum = int(value)
        tool = self.TLM.getTool(listname, toolnum)
        editor = self.toolEditor(tool)

        r = editor.Parent.exec_()
        if r:
            editor.accept()
            if self.TLM.updateTool(listname, toolnum, editor.Tool) is True:
                self.loadTable(listname)

    def moveUp(self):
        '''moves a tool to a lower number, if possible'''
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.TLM.getCurrentTableName()
            success, newNum = self.TLM.moveup(number, listname)
            if success:
                self.loadTable(listname)
                self.updateSelection(newNum)

    def moveDown(self):
        '''moves a tool to a higher number, if possible'''
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.TLM.getCurrentTableName()
            success, newNum = self.TLM.movedown(number, listname) 
            if success:
                self.loadTable(listname)
                self.updateSelection(newNum)

    def updateSelection(self, number):
        '''update the tool list selection to track moves'''
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            if int(model.index(i, 1).data()) == number:
                self.form.ToolsList.selectRow(i)
                self.form.ToolsList.model().item(i, 0).setCheckState(QtCore.Qt.Checked)
                return


    def importFile(self):
        '''imports a tooltable from a file'''
        filename = QtGui.QFileDialog.getOpenFileName(self.form, translate( "TooltableEditor", "Open tooltable", None), None, "{};;{};;{}".format(ToolLibraryManager.TooltableTypeJSON, ToolLibraryManager.TooltableTypeXML, ToolLibraryManager.TooltableTypeHeekscad))
        if filename[0]:
            listname = self.TLM.getNextToolTableName()
            if self.TLM.read(filename, listname):
                self.loadToolTables()
                #self.loadTable(listname)


    def exportFile(self):
        '''export a tooltable to a file'''
        filename = QtGui.QFileDialog.getSaveFileName(self.form, translate("TooltableEditor", "Save tooltable", None), None, "{};;{};;{}".format(ToolLibraryManager.TooltableTypeJSON, ToolLibraryManager.TooltableTypeXML, ToolLibraryManager.TooltableTypeLinuxCNC))
        if filename[0]:
            listname = self.TLM.getCurrentTableName()
            self.TLM.write(filename, listname)

    def toolSelected(self, index):
        ''' updates the ui when tools are selected'''
        self.form.ToolsList.selectRow(index.row())
        
        self.form.btnCopyTools.setEnabled(False)
        self.form.ButtonDelete.setEnabled(False)
        self.form.ButtonUp.setEnabled(False)
        self.form.ButtonDown.setEnabled(False)

        model = self.form.ToolsList.model()
        checkCount = 0
        checkList = []
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                checkCount += 1
                checkList.append(i)
                self.form.btnCopyTools.setEnabled(True)

        # only allow moving or deleting a single tool at a time.
        if checkCount == 1:
            #make sure the row is highlighted when the check box gets ticked   
            self.form.ToolsList.selectRow(checkList[0])       
            self.form.ButtonDelete.setEnabled(True)
            self.form.ButtonUp.setEnabled(True)
            self.form.ButtonDown.setEnabled(True)

        if len(PathUtils.GetJobs()) == 0:
            self.form.btnCopyTools.setEnabled(False)

    def copyTools(self):
        ''' copy selected tool '''
        tools = []
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                item = model.index(i, 1)
                tools.append(item.data())
        if len(tools) == 0:
            return

        targets = self.TLM.getJobList()
        currList = self.TLM.getCurrentTableName()

        for target in targets:
            if target == currList:
                targets.remove(target)

        if len(targets) == 0:
            FreeCAD.Console.PrintWarning("No Path Jobs in current document")
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
            if self.job:
                label = "T{}: {}".format(toolnum, tool.Name)
                tc = PathScripts.PathToolController.Create(label, tool=tool, toolNumber=int(toolnum))
                self.job.Proxy.addToolController(tc)
            else:
                for job in FreeCAD.ActiveDocument.findObjects("Path::Feature"):
                    if isinstance(job.Proxy, PathScripts.PathJob.ObjectJob) and job.Label == targetlist:
                        label = "T{}: {}".format(toolnum, tool.Name)
                        tc = PathScripts.PathToolController.Create(label, tool=tool, toolNumber=int(toolnum))
                        job.Proxy.addToolController(tc)
        if self.cb:
            self.cb()
        FreeCAD.ActiveDocument.recompute()

    def tableSelected(self, index):
        ''' loads the tools for the selected tool table '''
        name = self.form.TableList.itemFromIndex(index).text()
        self.loadTable(name)

    def loadTable(self, name):
        ''' loads the tools for the selected tool table '''
        tooldata = self.TLM.getTools(name)
        if tooldata:
            self.form.ToolsList.setModel(tooldata)
            self.form.ToolsList.resizeColumnsToContents()
            self.setCurrentToolTableByName(name)
            

    def addNewToolTable(self):
        ''' adds new tool to selected tool table '''
        name = self.TLM.addNewToolTable()
        self.loadToolTables()
        self.loadTable(name)
         
    def loadToolTables(self):
        ''' Load list of available tool tables '''
        self.form.TableList.clear()
        model = self.form.ToolsList.model()
        if model:
            model.clear()
        if len(self.TLM.getToolTables()) > 0:
            for table in self.TLM.getToolTables():
                listItem = QtGui.QListWidgetItem(table.Name)
                listItem.setIcon(QtGui.QIcon(':/icons/Path-ToolTable.svg'))
                listItem.setFlags(listItem.flags() | QtCore.Qt.ItemIsEditable)
                listItem.setSizeHint(QtCore.QSize(0,40))
                self.form.TableList.addItem(listItem)
                self.loadTable(self.TLM.getToolTables()[0].Name)
        
    def setCurrentToolTableByName(self, name):
        ''' get the current tool table '''
        item = self.form.TableList.findItems(name, QtCore.Qt.MatchExactly)[0]
        self.form.TableList.setCurrentItem(item)

    def removeToolTable(self):
        ''' delete the selected tool table '''
        self.TLM.deleteToolTable()
        self.loadToolTables()
    
    def initTableRename(self):
        ''' update the tool table list entry to allow renaming '''
        name = self.TLM.getCurrentTableName()
        item = self.form.TableList.findItems(name, QtCore.Qt.MatchExactly)[0]
        self.form.TableList.editItem(item)

    def renameTable(self, listItem):
        ''' rename the selected too table '''
        newName = listItem.text()
        index = self.form.TableList.indexFromItem(listItem).row()
        reloadTables = self.TLM.renameToolTable(newName, index)
        if reloadTables:
            self.loadToolTables()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    # def openMenu(self, position):
    #     menu = QtGui.QMenu()
    #     newAction = menu.addAction("New Tool Table")
    #     deleteAction = menu.addAction("Delete")
    #     renameAction = menu.addAction("Rename")
    #     action = menu.exec_(self.form.TableList.mapToGlobal(position))
    #     if action == newAction:
    #         self.addNewToolTable()
    #         pass
    #     if action == deleteAction:
    #         self.removeToolTable()
    #         pass
    #     if action == renameAction:
    #         self.initTableRename()
    #         pass

    def setupUi(self):
        # Connect Signals and Slots
        self.form.ButtonNewTool.clicked.connect(self.addTool)
        self.form.ButtonImport.clicked.connect(self.importFile)
        self.form.ButtonExport.clicked.connect(self.exportFile)
        self.form.ButtonDown.clicked.connect(self.moveDown)
        self.form.ButtonUp.clicked.connect(self.moveUp)
        self.form.ButtonDelete.clicked.connect(self.delete)

        self.form.ToolsList.doubleClicked.connect(self.editTool)
        self.form.ToolsList.clicked.connect(self.toolSelected)

        self.form.btnCopyTools.clicked.connect(self.copyTools)

        self.form.TableList.clicked.connect(self.tableSelected)
        self.form.TableList.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        #self.form.TableList.customContextMenuRequested.connect(self.openMenu)
        self.form.TableList.itemChanged.connect(self.renameTable)

        self.form.ButtonAddToolTable.clicked.connect(self.addNewToolTable)
        self.form.ButtonAddToolTable.setToolTip(translate("TooltableEditor","Add New Tool Table"))
        self.form.ButtonRemoveToolTable.clicked.connect(self.removeToolTable)
        self.form.ButtonRemoveToolTable.setToolTip(translate("TooltableEditor","Delete Selected Tool Table"))
        self.form.ButtonRenameToolTable.clicked.connect(self.initTableRename)
        self.form.ButtonRenameToolTable.setToolTip(translate("TooltableEditor","Rename Selected Tool Table"))


        self.form.btnCopyTools.setEnabled(False)
        self.form.ButtonDelete.setEnabled(False)
        self.form.ButtonUp.setEnabled(False)
        self.form.ButtonDown.setEnabled(False)

        self.setFields()

class CommandToolLibraryEdit():
    def __init__(self):
        pass

    def edit(self, job=None, cb=None):
        editor = EditorPanel(job, cb)
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
