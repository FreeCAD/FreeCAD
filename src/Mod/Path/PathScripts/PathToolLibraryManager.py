# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path
import PathScripts
import PathScripts.PathLog as PathLog
import PathScripts.PathUtil as PathUtil
import json
import os
import xml.sax
from PySide import QtGui

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate

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
            # print attrs["diameter"]
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
            # print attrs["diameter"]
            self.tool.Diameter = float(attrs["diameter"])
            self.tool.LengthOffset = float(attrs["tool_length_offset"])
            self.tool.FlatRadius = float(attrs["flat_radius"])
            self.tool.CornerRadius = float(attrs["corner_radius"])
            self.tool.CuttingEdgeAngle = float(attrs["cutting_edge_angle"])
            self.tool.CuttingEdgeHeight = float(attrs["cutting_edge_height"])

    # Call when an elements ends
    def endElement(self, name):
        if name == "Tool":
            if self.tooltable and self.tool and self.number:
                self.tooltable.setTool(self.number, self.tool)
                self.number = None
                self.tool = None


class ToolLibraryManager:
    """
    The Tool Library is a list of individual tool tables.  Each
    Tool Table can contain n tools.  The tool library will be persisted to user
    preferences and all or part of the library can be exported to other formats
    """

    TooltableTypeJSON = translate("PathToolLibraryManager", "Tooltable JSON (*.json)")
    TooltableTypeXML = translate("PathToolLibraryManager", "Tooltable XML (*.xml)")
    TooltableTypeHeekscad = translate(
        "PathToolLibraryManager", "HeeksCAD tooltable (*.tooltable)"
    )
    TooltableTypeLinuxCNC = translate(
        "PathToolLibraryManager", "LinuxCNC tooltable (*.tbl)"
    )

    PreferenceMainLibraryXML = "ToolLibrary"
    PreferenceMainLibraryJSON = "ToolLibrary-Main"

    def __init__(self):
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        self.toolTables = []
        self.currentTableName = None
        self.loadToolTables()

    def getToolTables(self):
        """Return tool table list"""
        return self.toolTables

    def getCurrentTableName(self):
        """return the name of the currently loaded tool table"""
        return self.currentTableName

    def getCurrentTable(self):
        """returns an object of the current tool table"""
        return self.getTableFromName(self.currentTableName)

    def getTableFromName(self, name):
        """get the tool table object from the name"""
        for table in self.toolTables:
            if table.Name == name:
                return table

    def getNextToolTableName(self, tableName="Tool Table"):
        """get a unique name for a new tool table"""
        iter = 1
        tempName = tableName[-2:]

        if tempName[0] == "-" and tempName[-1].isdigit():
            tableName = tableName[:-2]

        while any(
            table.Name == tableName + "-" + str(iter) for table in self.toolTables
        ):
            iter += 1

        return tableName + "-" + str(iter)

    def addNewToolTable(self):
        """creates a new tool table"""
        tt = Path.Tooltable()
        tt.Version = 1
        name = self.getNextToolTableName()
        tt.Name = name
        self.toolTables.append(tt)
        self.saveMainLibrary()
        return name

    def deleteToolTable(self):
        """deletes the selected tool table"""
        if len(self.toolTables):
            index = next(
                (
                    index
                    for (index, d) in enumerate(self.toolTables)
                    if d.Name == self.currentTableName
                ),
                None,
            )
            self.toolTables.pop(index)
            self.saveMainLibrary()

    def renameToolTable(self, newName, index):
        """renames a tool table with the new name"""
        currentTableName = self.toolTables[index].Name
        if newName == currentTableName:
            PathLog.error(translate("PathToolLibraryManager", "Tool Table Same Name"))
            return False
        if newName in self.toolTables:
            PathLog.error(translate("PathToolLibraryManager", "Tool Table Name Exists"))
            return False
        tt = self.getTableFromName(currentTableName)
        if tt:
            tt.Name = newName
            self.saveMainLibrary()
            return True

    def templateAttrs(self):
        """gets the tool table arributes"""
        toolTables = []
        for tt in self.toolTables:
            tableData = {}
            tableData["Version"] = 1
            tableData["TableName"] = tt.Name

            toolData = {}
            for tool in tt.Tools:
                toolData[tool] = tt.Tools[tool].templateAttrs()

            tableData["Tools"] = toolData
            toolTables.append(tableData)

        return toolTables

    def tooltableFromAttrs(self, stringattrs):
        if stringattrs.get("Version") and 1 == int(stringattrs["Version"]):

            tt = Path.Tooltable()
            tt.Version = 1
            tt.Name = self.getNextToolTableName()

            if stringattrs.get("Version"):
                tt.Version = stringattrs.get("Version")

            if stringattrs.get("TableName"):
                tt.Name = stringattrs.get("TableName")
                if any(table.Name == tt.Name for table in self.toolTables):
                    tt.Name = self.getNextToolTableName(tt.Name)

            for key, attrs in PathUtil.keyValueIter(stringattrs["Tools"]):
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
            PathLog.error(
                translate(
                    "PathToolLibraryManager",
                    "Unsupported Path tooltable template version %s",
                )
                % stringattrs.get("Version")
            )
        return None

    def loadToolTables(self):
        """loads the tool tables from the stored data"""
        self.toolTables = []
        self.currentTableName = ""

        def addTable(tt):
            if tt:
                self.toolTables.append(tt)
            else:
                PathLog.error(
                    translate("PathToolLibraryManager", "Unsupported Path tooltable")
                )

        prefString = self.prefs.GetString(self.PreferenceMainLibraryJSON, "")

        if not prefString:
            return

        prefsData = json.loads(prefString)

        if isinstance(prefsData, dict):
            tt = self.tooltableFromAttrs(prefsData)
            addTable(tt)

        if isinstance(prefsData, list):
            for table in prefsData:
                tt = self.tooltableFromAttrs(table)
                addTable(tt)

        if len(self.toolTables):
            self.currentTableName = self.toolTables[0].Name

    def saveMainLibrary(self):
        """Persists the permanent library to FreeCAD user preferences"""
        tmpstring = json.dumps(self.templateAttrs())
        self.prefs.SetString(self.PreferenceMainLibraryJSON, tmpstring)
        self.loadToolTables()
        return True

    def getJobList(self):
        """Builds the list of all Tool Table lists"""
        tablelist = []

        for o in FreeCAD.ActiveDocument.Objects:
            if hasattr(o, "Proxy"):
                if isinstance(o.Proxy, PathScripts.PathJob.ObjectJob):
                    tablelist.append(o.Label)

        return tablelist

    def getTool(self, listname, toolnum):
        """gets the tool object"""
        tt = self.getTableFromName(listname)
        return tt.getTool(toolnum)

    def getTools(self, tablename):
        """returns the tool data for a given table"""
        tooldata = []
        tableExists = any(table.Name == tablename for table in self.toolTables)
        if tableExists:
            self.currentTableName = tablename
        else:
            return None

        tt = self.getTableFromName(tablename)
        headers = ["", "Tool Num.", "Name", "Tool Type", "Diameter"]
        model = QtGui.QStandardItemModel()
        model.setHorizontalHeaderLabels(headers)

        def unitconv(ivalue):
            val = FreeCAD.Units.Quantity(ivalue, FreeCAD.Units.Length)
            displayed_val = (
                val.UserString
            )  # just the displayed value-not the internal one
            return displayed_val

        if tt:
            if len(tt.Tools) == 0:
                tooldata.append([])
            for number, t in PathUtil.keyValueIter(tt.Tools):

                itemcheck = QtGui.QStandardItem()
                itemcheck.setCheckable(True)
                itemNumber = QtGui.QStandardItem(str(number))
                itemName = QtGui.QStandardItem(t.Name)
                itemToolType = QtGui.QStandardItem(t.ToolType)
                itemDiameter = QtGui.QStandardItem(unitconv(t.Diameter))

                row = [itemcheck, itemNumber, itemName, itemToolType, itemDiameter]
                model.appendRow(row)

        return model

    # methods for importing and exporting
    def read(self, filename, listname):
        "imports a tooltable from a file"

        importedTables = []

        try:
            fileExtension = os.path.splitext(filename[0])[1].lower()
            xmlHandler = None
            if fileExtension == ".tooltable":
                xmlHandler = HeeksTooltableHandler()
            if fileExtension == ".xml":
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

        except Exception as e:  # pylint: disable=broad-except
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
                    return (open(PathUtil.toUnicode(name), "w"), name)

                if filename[1] == self.TooltableTypeXML:
                    fp, fname = openFileWithExtension(filename[0], ".xml")
                    fp.write('<?xml version="1.0" encoding="UTF-8"?>\n')
                    fp.write(tt.Content)
                elif filename[1] == self.TooltableTypeLinuxCNC:
                    fp, fname = openFileWithExtension(filename[0], ".tbl")
                    for key in tt.Tools:
                        t = tt.Tools[key]
                        fp.write(
                            "T{0} P{0} Y{1} Z{2} A{3} B{4} C{5} U{6} V{7} W{8} D{9} I{10} J{11} Q{12} ;{13}\n".format(
                                key,
                                0,
                                t.LengthOffset,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                t.Diameter,
                                0,
                                0,
                                0,
                                t.Name,
                            )
                        )
                else:
                    fp, fname = openFileWithExtension(filename[0], ".json")
                    json.dump(self.templateAttrs(), fp, sort_keys=True, indent=2)

                fp.close()
                print("Written ", PathUtil.toUnicode(fname))

            except Exception as e:  # pylint: disable=broad-except
                print("Could not write file:", e)

    def addnew(self, listname, tool, position=None):
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
        """updates tool data"""
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

    def duplicate(self, number, listname):
        """duplicates the selected tool in the selected tool table"""
        tt = self.getTableFromName(listname)
        tool = tt.getTool(number).copy()
        tt.addTools(tool)

        newID = list(tt.Tools)[-1]

        if listname == self.getCurrentTableName():
            self.saveMainLibrary()
        return True, newID

    def moveToTable(self, number, listname):
        """Moves the tool to selected tool table"""
        fromTable = self.getTableFromName(self.getCurrentTableName())
        toTable = self.getTableFromName(listname)
        tool = fromTable.getTool(number).copy()
        toTable.addTools(tool)
        fromTable.deleteTool(number)

    def delete(self, number, listname):
        """deletes a tool from the current list"""
        tt = self.getTableFromName(listname)
        tt.deleteTool(number)
        if listname == self.getCurrentTableName():
            self.saveMainLibrary()
        return True
