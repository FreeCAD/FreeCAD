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
import Draft
import Part
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
            print attributes["diameter"]
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
            print attributes["diameter"]
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
        self.ToolLibrary = []
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path/ToolLibrary")
        return

    def saveLibrary(self):
        '''Persists the entire library to FreeCAD user preferences'''
        tmpstring = ""
        for table in self.ToolLibrary:
            if table["listtype"] == 'User':
                tmpstring += table["list"].Content
        self.prefs.SetString("ToolLibrary", tmpstring)

        #    FreeCAD.ConfigSet("PathToolTable:" + table[0], table[2].Content)

    def loadLibrary(self):
        '''Loads the current library from FreeCAD user preferences'''
        # Get persisted libraries from user prefs
        tmpstring = self.prefs.GetString("ToolLibrary", "")
        ToolLibrary = []
        if tmpstring != "":
            Handler = FreeCADTooltableHandler()
            try:
                xml.sax.parseString(tmpstring, Handler)
                tt = Handler.tooltable
                toollist = {'name': "main", 'listtype': "User", 'list': tt}
                ToolLibrary.append(toollist)
            except:
                FreeCAD.Console.PrintError(
                    "Unable to import tools from user preferences")

        # Get ToolTables from any open CNC jobs
        for o in FreeCAD.ActiveDocument.Objects:
            if "Proxy" in o.PropertiesList:
                if hasattr(o, "Tooltable"):
                    toollist = {'name': o.Name,
                                'listtype': "Job", 'list': o.Tooltable}
                    ToolLibrary.append(toollist)
        self.ToolLibrary = ToolLibrary
        return self.ToolLibrary

    # methods for lists
    def addList(self, tablename, listtype="User", TL=None):
        '''Add a new tooltable to the user library'''
        if TL is None:
            TL = Path.Tooltable()
        toollist = {'name': tablename, 'listtype': listtype, 'list': TL}
        self.ToolLibrary.append(toollist)
        return TL

    def deleteList(self, tablename):
        '''Delete all lists from the user library with the given listname'''
        for l in self.ToolLibrary:
            if l['name'] == tablename:
                # maybe check if tools exist in list
                self.ToolLibrary.remove(l)
        return

    def findList(self, tablename):
        '''Finds and returns list by name'''
        returnlist = []
        for l in self.ToolLibrary:
            if l['name'] == tablename:
                returnlist.append(l)
        return returnlist

    # methods for importing and exporting
    def read(self):
        "imports a tooltable from a file"
        filename = QtGui.QFileDialog.getOpenFileName(None, _translate("ToolLibraryManager", "Import tooltable", None), None, _translate(
            "ToolLibraryManager", "Tooltable XML (*.xml);;HeeksCAD tooltable (*.tooltable)", None))
        if filename:
            parser = xml.sax.make_parser()
            parser.setFeature(xml.sax.handler.feature_namespaces, 0)
            if os.path.splitext(filename[0])[1].lower() == ".tooltable":
                Handler = HeeksTooltableHandler()
            else:
                Handler = FreeCADTooltableHandler()
            parser.setContentHandler(Handler)
            parser.parse(str(filename[0]))
            if Handler.tooltable:
                self.addList(filename[0], Handler.tooltable)
                # self.reset()

    def createToolController(self, job, tool):
        pass

    def exportListHeeks(self, tooltable):
        '''exports one or more Lists as a HeeksCNC tooltable'''
        pass

    def exportListLinuxCNC(self, tooltable):
        '''exports one or more Lists as a LinuxCNC tooltable'''
        pass

    def exportListXML(self, tooltable):
        '''exports one or more Lists as an XML file'''
        pass
