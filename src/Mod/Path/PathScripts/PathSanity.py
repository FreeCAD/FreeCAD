# ***************************************************************************
# *   (c) Sliptonic (shopinthewoods@gmail.com)  2016                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

'''
This file has utilities for checking and catching common errors in FreeCAD
Path projects.  Ideally, the user could execute these utilities from an icon
to make sure tools are selected and configured and defaults have been revised
'''

from __future__ import print_function
from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import PathScripts
import PathScripts.PathLog as PathLog
import PathScripts.PathUtil as PathUtil
import PathScripts.PathPreferences as PathPreferences
from collections import Counter
from datetime import datetime
import os
import webbrowser
# Qt translation handling


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


LOG_MODULE = 'PathSanity'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
# PathLog.trackModule('PathSanity')


class CommandPathSanity:
    baseobj = None
    outputpath = PathPreferences.defaultOutputFile()
    if outputpath == "":
        outputpath = PathPreferences.macroFilePath()
    if outputpath[-1] !=  os.sep:
        outputpath += os.sep
    if not os.path.exists(outputpath):
        os.makedirs(outputpath)

    squawkData = {"items": []}

    def GetResources(self):
        return {'Pixmap': 'Path-Sanity',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Sanity",
                    "Check the path job for common errors"),
                'Accel': "P, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Sanity",
                    "Check the path job for common errors")}

    def IsActive(self):
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        return isinstance(obj.Proxy, PathScripts.PathJob.ObjectJob)

    def Activated(self):
        # if everything is ok, execute
        self.squawkData["items"] = []

        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        data = self.__summarize(obj)
        html = self.__report(data)
        if html is not None:
            FreeCAD.Console.PrintMessage("HTML report written to {}".format(html))
            webbrowser.open(html)

    def __makePicture(self, obj, imageName):
        """
        Makes an image of the target object.  Returns filename
        """

        # remember vis state of document objects. Turn off all but target
        visible = [o for o in obj.Document.Objects if o.Visibility]
        for o in obj.Document.Objects:
            o.Visibility = False
        obj.Visibility = True

        aview = FreeCADGui.activeDocument().activeView()
        aview.setAnimationEnabled(False)

        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtGui.QMdiArea)
        view = mdi.activeSubWindow()
        view.showNormal()
        view.resize(320, 320)

        imagepath = self.outputpath + '{}'.format(imageName)

        aview.viewIsometric()
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.SendMsgToActiveView("PerspectiveCamera")
        FreeCADGui.Selection.addSelection(obj)
        FreeCADGui.SendMsgToActiveView("ViewSelection")
        FreeCADGui.Selection.clearSelection()
        aview.saveImage(imagepath + '.png', 320, 320, 'Current')
        aview.saveImage(imagepath + '_t.png', 320, 320, 'Transparent')

        view.showMaximized()

        aview.setAnimationEnabled(True)

        # Restore visibility
        obj.Visibility = False
        for o in visible:
            o.Visibility = True

        return "{}_t.png".format(imagepath)

    def __report(self, data):
        """
        generates an asciidoc file with the report information
        """

        reportTemplate = """
= Setup Report for FreeCAD Job: {jobname}
:toc:
:icons: font
:imagesdir: ""
:data-uri:

== Part Information

|===
{infoTable}
|===


== Run Summary

|===
{runTable}
|===

== Rough Stock

|===
{stockTable}
|===

== Tool Data

{toolTables}

== Output (Gcode)

|===
{outTable}
|===

== Fixtures and Workholding

|===
{fixtureTable}
|===

== Squawks

|===
{squawkTable}
|===

"""
        # Generate the markup for the Part Information Section

        infoTable = ""

        PartLabel = translate("Path_Sanity", "Base Object(s)")
        SequenceLabel = translate("Path_Sanity", "Job Sequence")
        DescriptionLabel = translate("Path_Sanity", "Job Description")
        JobTypeLabel = translate("Path_Sanity", "Job Type")
        CADLabel = translate("Path_Sanity", "CAD File Name")
        LastSaveLabel = translate("Path_Sanity", "Last Save Date")
        CustomerLabel = translate("Path_Sanity", "Customer")
        DesignerLabel = translate("Path_Sanity", "Designer")

        d = data['designData']
        b = data['baseData']

        jobname = d['JobLabel']

        basestable = "!===\n"
        for key, val in b['bases'].items():
            basestable += "! " + key + " ! " + val + "\n"

        basestable += "!==="

        infoTable += "|*" + PartLabel + "* a| " + basestable + " .7+a|" + \
            "image::" + b['baseimage'] + "[" + PartLabel + "]\n"
        infoTable += "|*" + SequenceLabel + "*|" + d['Sequence']
        infoTable += "|*" + JobTypeLabel + "*|" + d['JobType']
        infoTable += "|*" + DescriptionLabel + "*|" + d['JobDescription']
        infoTable += "|*" + CADLabel + "*|" + d['FileName']
        infoTable += "|*" + LastSaveLabel + "*|" + d['LastModifiedDate']
        infoTable += "|*" + CustomerLabel + "*|" + d['Customer']
        infoTable += "|*" + DesignerLabel + "*|" + d['Designer']

        # Generate the markup for the Run Summary Section

        runTable = ""
        opLabel = translate("Path_Sanity", "Operation")
        zMinLabel = translate("Path_Sanity", "Minimum Z Height")
        zMaxLabel = translate("Path_Sanity", "Maximum Z Height")
        cycleTimeLabel = translate("Path_Sanity", "Cycle Time")
        coolantLabel = translate("Path_Sanity", "Coolant")
        jobTotalLabel = translate("Path_Sanity", "TOTAL JOB")

        d = data['runData']

        runTable += "|*" + opLabel + "*|*" + zMinLabel + "*|*" + zMaxLabel + \
            "*|*" + coolantLabel + "*|*" + cycleTimeLabel + "*\n"

        for i in d['items']:
            runTable += "|{}".format(i['opName'])
            runTable += "|{}".format(i['minZ'])
            runTable += "|{}".format(i['maxZ'])
            runTable += "|{}".format(i['coolantMode'])
            runTable += "|{}".format(i['cycleTime'])

        runTable += "|*" + jobTotalLabel + "* |{} |{} |{}".format(
            d['jobMinZ'],
            d['jobMaxZ'],
            d['cycletotal'])

        # Generate the markup for the Tool Data Section
        toolTables = ""

        toolLabel = translate("Path_Sanity", "Tool Number")
        descriptionLabel = translate("Path_Sanity", "Description")
        manufLabel = translate("Path_Sanity", "Manufacturer")
        partNumberLabel = translate("Path_Sanity", "Part Number")
        urlLabel = translate("Path_Sanity", "URL")
        inspectionNotesLabel = translate("Path_Sanity", "Inspection Notes")
        opLabel = translate("Path_Sanity", "Operation")
        tcLabel = translate("Path_Sanity", "Tool Controller")
        feedLabel = translate("Path_Sanity", "Feed Rate")
        speedLabel = translate("Path_Sanity", "Spindle Speed")
        shapeLabel = translate("Path_Sanity", "Tool Shape")
        diameterLabel = translate("Path_Sanity", "Tool Diameter")

        d = data['toolData']

        for key, value in d.items():
            toolTables += "=== {}: T{}\n".format(toolLabel, key)

            toolTables += "|===\n"

            toolTables += "|*{}*| {} a| image::{}[{}]\n".format(descriptionLabel, value['description'], value['imagepath'], key)
            toolTables += "|*{}* 2+| {}\n".format(manufLabel, value['manufacturer'])
            toolTables += "|*{}* 2+| {}\n".format(partNumberLabel, value['partNumber'])
            toolTables += "|*{}* 2+| {}\n".format(urlLabel, value['url'])
            toolTables += "|*{}* 2+| {}\n".format(inspectionNotesLabel, value['inspectionNotes'])
            toolTables += "|*{}* 2+| {}\n".format(shapeLabel, value['shape'])
            toolTables += "|*{}* 2+| {}\n".format(diameterLabel, value['diameter'])
            toolTables += "|===\n"

            toolTables += "|===\n"
            toolTables += "|*" + opLabel + "*|*" + tcLabel + "*|*" + feedLabel + "*|*" + speedLabel + "*\n"
            for o in value['ops']:
                toolTables += "|" + o['Operation'] + "|" + o['ToolController'] + "|" + o['Feed'] + "|" + o['Speed'] + "\n"
            toolTables += "|===\n"

            toolTables += "\n"

        # Generate the markup for the Rough Stock Section
        stockTable = ""
        xDimLabel = translate("Path_Sanity", "X Size")
        yDimLabel = translate("Path_Sanity", "Y Size")
        zDimLabel = translate("Path_Sanity", "Z Size")
        materialLabel = translate("Path_Sanity", "Material")

        d = data['stockData']

        stockTable += "|*{}*|{} .4+a|image::{}[stock]\n".format(
                materialLabel,
                d['material'],
                d['stockImage'])
        stockTable += "|*{}*|{}".format(xDimLabel, d['xLen'])
        stockTable += "|*{}*|{}".format(yDimLabel, d['yLen'])
        stockTable += "|*{}*|{}".format(zDimLabel, d['zLen'])

        # Generate the markup for the Fixture Section

        fixtureTable = ""
        offsetsLabel = translate("Path_Sanity", "Work Offsets")
        orderByLabel = translate("Path_Sanity", "Order By")
        datumLabel = translate("Path_Sanity", "Part Datum")

        d = data['fixtureData']

        fixtureTable += "|*{}*|{}\n".format(offsetsLabel, d['fixtures'])
        fixtureTable += "|*{}*|{}\n".format(orderByLabel, d['orderBy'])
        fixtureTable += "|*{}* a| image::{}[]".format(datumLabel, d['datumImage'])

        # Generate the markup for the Output Section

        outTable = ""
        d = data['outputData']

        gcodeFileLabel = translate("Path_Sanity", "Gcode File")
        lastpostLabel = translate("Path_Sanity", "Last Post Process Date")
        stopsLabel = translate("Path_Sanity", "Stops")
        programmerLabel = translate("Path_Sanity", "Programmer")
        machineLabel = translate("Path_Sanity", "Machine")
        postLabel = translate("Path_Sanity", "Postprocessor")
        flagsLabel = translate("Path_Sanity", "Post Processor Flags")
        fileSizeLabel = translate("Path_Sanity", "File Size (kbs)")
        lineCountLabel = translate("Path_Sanity", "Line Count")

        outTable += "|*{}*|{}\n".format(gcodeFileLabel, d['lastgcodefile'])
        outTable += "|*{}*|{}\n".format(lastpostLabel, d['lastpostprocess'])
        outTable += "|*{}*|{}\n".format(stopsLabel, d['optionalstops'])
        outTable += "|*{}*|{}\n".format(programmerLabel, d['programmer'])
        outTable += "|*{}*|{}\n".format(machineLabel, d['machine'])
        outTable += "|*{}*|{}\n".format(postLabel, d['postprocessor'])
        outTable += "|*{}*|{}\n".format(flagsLabel, d['postprocessorFlags'])
        outTable += "|*{}*|{}\n".format(fileSizeLabel, d['filesize'])
        outTable += "|*{}*|{}\n".format(lineCountLabel, d['linecount'])

        # Generate the markup for the Squawk Section

        noteLabel = translate("Path_Sanity", "Note")
        operatorLabel = translate("Path_Sanity", "Operator")
        dateLabel = translate("Path_Sanity", "Date")

        squawkTable = "|*{}*|*{}*|*{}*\n".format(noteLabel,
                                                 operatorLabel,
                                                 dateLabel)

        d = data['squawkData']
        for i in d['items']:
            squawkTable += "a|{}: {}".format(i['squawkType'], i['Note'])
            squawkTable += "|{}".format(i['Operator'])
            squawkTable += "|{}".format(i['Date'])
            squawkTable += "\n"

        # merge template and custom markup

        report = reportTemplate.format(
            jobname=jobname,
            infoTable=infoTable,
            runTable=runTable,
            toolTables=toolTables,
            stockTable=stockTable,
            fixtureTable=fixtureTable,
            outTable=outTable,
            squawkTable=squawkTable)

        # Save the report

        reportraw = self.outputpath + '/setupreport.asciidoc'
        reporthtml = self.outputpath + '/setupreport.html'
        with open(reportraw, 'w') as fd:
            fd.write(report)
            fd.close()
            FreeCAD.Console.PrintMessage('asciidoc file written to {}\n'.format(reportraw))

        try:
            result = os.system('asciidoctor {} -o {}'.format(reportraw,
                reporthtml))
            if str(result) == "32512":
                msg = "asciidoctor not found. html cannot be generated."
                QtGui.QMessageBox.information(None, "Path Sanity", msg)
                FreeCAD.Console.PrintMessage(msg)
                reporthtml = None

        except Exception as e:
            FreeCAD.Console.PrintError(e)

        return reporthtml

    def __summarize(self, obj):
        """
        Top level function to summarize information for the report
        Returns a dictionary of sections
        """
        data = {}
        data['baseData'] = self.__baseObjectData(obj)
        data['designData'] = self.__designData(obj)
        data['toolData'] = self.__toolData(obj)
        data['runData'] = self.__runData(obj)
        data['outputData'] = self.__outputData(obj)
        data['fixtureData'] = self.__fixtureData(obj)
        data['stockData'] = self.__stockData(obj)
        data['squawkData'] = self.squawkData
        return data

    def squawk(self, operator, note, date=datetime.now(), squawkType="NOTE"):
        squawkType = squawkType if squawkType in \
            ["NOTE", "WARNING", "CAUTION", "TIP"] else "NOTE"

        self.squawkData['items'].append({"Date": str(date),
                                         "Operator": operator,
                                         "Note": note,
                                         "squawkType": squawkType})

    def __baseObjectData(self, obj):
        data = {'baseimage': '',
                'bases': ''}
        try:
            bases = {}
            for name, count in \
                    PathUtil.keyValueIter(Counter([obj.Proxy.baseObject(obj,
                        o).Label for o in obj.Model.Group])):
                bases[name] = str(count)

            data['baseimage'] = self.__makePicture(obj.Model, "baseimage")
            data['bases'] = bases

        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__baseObjectData)", e, squawkType="CAUTION")

        return data

    def __designData(self, obj):
        """
        Returns header information about the design document
        Returns information about issues and concerns (squawks)
        """

        data = {'FileName': '',
                'LastModifiedDate': '',
                'Customer': '',
                'Designer': '',
                'JobDescription': '',
                'JobLabel': '',
                'Sequence': '',
                'JobType': ''}
        try:
            data['FileName'] = obj.Document.FileName
            data['LastModifiedDate'] = str(obj.Document.LastModifiedDate)
            data['Customer'] = obj.Document.Company
            data['Designer'] = obj.Document.LastModifiedBy
            data['JobDescription'] = obj.Description
            data['JobLabel'] = obj.Label

            n = 0
            m = 0
            for i in obj.Document.Objects:
                if hasattr(i, "Proxy"):
                    if isinstance(i.Proxy, PathScripts.PathJob.ObjectJob):
                        m += 1
                        if i is obj:
                            n = m
            data['Sequence'] = "{} of {}".format(n, m)
            data['JobType'] = "2.5D Milling"  # improve after job types added

        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__designData)", e, squawkType="CAUTION")

        return data

    def __toolData(self, obj):
        """
        Returns information about the tools used in the job, and associated
        toolcontrollers
        Returns information about issues and problems with the tools (squawks)
        """
        data = {}

        try:
            for TC in obj.ToolController:
                if not hasattr(TC.Tool, 'BitBody'):
                    self.squawk("PathSanity",
                    "Tool number {} is a legacy tool. Legacy tools not \
                        supported by Path-Sanity".format(TC.ToolNumber),
                    squawkType="WARNING")
                    continue  # skip old-style tools
                tooldata = data.setdefault(str(TC.ToolNumber), {})
                bitshape = tooldata.setdefault('BitShape', "")
                if bitshape not in ["", TC.Tool.BitShape]:
                    self.squawk("PathSanity",
                    "Tool number {} used by multiple tools".format(TC.ToolNumber),
                    squawkType="CAUTION")
                tooldata['bitShape'] = TC.Tool.BitShape
                tooldata['description'] = TC.Tool.Label
                tooldata['manufacturer'] = ""
                tooldata['url'] = ""
                tooldata['inspectionNotes'] = ""
                tooldata['diameter'] = str(TC.Tool.Diameter)
                tooldata['shape'] = TC.Tool.ShapeName

                tooldata['partNumber'] = ""
                imagedata = TC.Tool.Proxy.getBitThumbnail(TC.Tool)
                imagepath = '{}/T{}.png'.format(self.outputpath, TC.ToolNumber)
                tooldata['feedrate'] = str(TC.HorizFeed)
                if TC.HorizFeed.Value == 0.0:
                    self.squawk("PathSanity",
                        "Tool Controller '{}' has no feedrate".format(TC.Label),
                        squawkType="WARNING")

                tooldata['spindlespeed'] = str(TC.SpindleSpeed)
                if TC.SpindleSpeed == 0.0:
                    self.squawk("PathSanity",
                        "Tool Controller '{}' has no spindlespeed".format(TC.Label),
                        squawkType="WARNING")

                with open(imagepath, 'wb') as fd:
                    fd.write(imagedata)
                    fd.close()
                tooldata['imagepath'] = imagepath

                used = False
                for op in obj.Operations.Group:
                    if hasattr(op, "ToolController") and op.ToolController is TC:
                        used = True
                        tooldata.setdefault('ops', []).append(
                            {"Operation": op.Label,
                             "ToolController": TC.Label,
                             "Feed": str(TC.HorizFeed),
                             "Speed": str(TC.SpindleSpeed)})

                if used is False:
                    tooldata.setdefault('ops', [])
                    self.squawk("PathSanity",
                        "Tool Controller '{}' is not used".format(TC.Label),
                        squawkType="WARNING")

        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__toolData)", e, squawkType="CAUTION")

        return data

    def __runData(self, obj):
        data = {'cycletotal': '',
                'jobMinZ': '',
                'jobMaxZ': '',
                'jobDescription': '',
                'items': []}
        try:
            data['cycletotal'] = str(obj.CycleTime)
            data['jobMinZ'] = FreeCAD.Units.Quantity(obj.Path.BoundBox.ZMin,
                    FreeCAD.Units.Length).UserString
            data['jobMaxZ'] = FreeCAD.Units.Quantity(obj.Path.BoundBox.ZMax,
                    FreeCAD.Units.Length).UserString
            data['jobDescription'] = obj.Description

            data['items'] = []
            for op in obj.Operations.Group:

                oplabel = op.Label
                ctime = op.CycleTime if hasattr(op, "CycleTime") else 0.0
                cool = op.CoolantMode if hasattr(op, "CoolantMode") else "N/A"

                o = op
                while len(o.ViewObject.claimChildren()) != 0:  # dressup
                    oplabel = "{}:{}".format(oplabel, o.Base.Label)
                    o = o.Base
                    if hasattr(o, 'CycleTime'):
                        ctime = o.CycleTime
                    cool = o.CoolantMode if hasattr(o, "CoolantMode") else cool

                if hasattr(op, 'Active') and not op.Active:
                    oplabel = "{} (INACTIVE)".format(oplabel)
                    ctime = 0.0

                if op.Path.BoundBox.isValid():
                    zmin = FreeCAD.Units.Quantity(op.Path.BoundBox.ZMin,
                              FreeCAD.Units.Length).UserString
                    zmax = FreeCAD.Units.Quantity(op.Path.BoundBox.ZMax,
                              FreeCAD.Units.Length).UserString
                else:
                    zmin = ''
                    zmax = ''

                opdata = {"opName": oplabel,
                          "minZ": zmin,
                          "maxZ": zmax,
                          "cycleTime": ctime,
                          "coolantMode": cool}
                data['items'].append(opdata)

        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__runData)", e, squawkType="CAUTION")

        return data

    def __stockData(self, obj):
        data = {'xLen': '',
                'yLen': '',
                'zLen': '',
                'material': '',
                'stockImage': ''}

        try:
            bb = obj.Stock.Shape.BoundBox
            data['xLen'] = FreeCAD.Units.Quantity(bb.XLength, FreeCAD.Units.Length).UserString
            data['yLen'] = FreeCAD.Units.Quantity(bb.YLength, FreeCAD.Units.Length).UserString
            data['zLen'] = FreeCAD.Units.Quantity(bb.ZLength, FreeCAD.Units.Length).UserString

            data['material'] = "Not Specified"
            if hasattr(obj.Stock, 'Material'):
                if obj.Stock.Material is not None:
                    data['material'] = obj.Stock.Material.Material['Name']

            if data['material'] == "Not Specified":
                self.squawk("PathSanity", "Consider Specifying the Stock Material", squawkType="TIP")

            data['stockImage'] = self.__makePicture(obj.Stock, "stockImage")
        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__stockData)", e, squawkType="CAUTION")

        return data

    def __fixtureData(self, obj):
        data = {'fixtures': '', 'orderBy': '', 'datumImage': ''}
        try:
            data['fixtures'] = str(obj.Fixtures)
            data['orderBy'] = str(obj.OrderOutputBy)

            aview = FreeCADGui.activeDocument().activeView()
            aview.setAnimationEnabled(False)

            obj.Visibility = False
            obj.Operations.Visibility = False

            mw = FreeCADGui.getMainWindow()
            mdi = mw.findChild(QtGui.QMdiArea)
            view = mdi.activeSubWindow()
            view.showNormal()
            view.resize(320, 320)

            imagepath = '{}/origin'.format(self.outputpath)

            FreeCADGui.Selection.clearSelection()
            FreeCADGui.SendMsgToActiveView("PerspectiveCamera")
            aview.viewIsometric()
            for i in obj.Model.Group:
                FreeCADGui.Selection.addSelection(i)
            FreeCADGui.SendMsgToActiveView("ViewSelection")
            FreeCADGui.Selection.clearSelection()
            obj.ViewObject.Proxy.editObject(obj)
            aview.saveImage('{}.png'.format(imagepath), 320, 320, 'Current')
            aview.saveImage('{}_t.png'.format(imagepath), 320, 320, 'Transparent')
            obj.ViewObject.Proxy.uneditObject(obj)
            obj.Visibility = True
            obj.Operations.Visibility = True

            view.showMaximized()

            aview.setAnimationEnabled(True)
            data['datumImage'] = '{}_t.png'.format(imagepath)

        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__fixtureData)", e, squawkType="CAUTION")

        return data

    def __outputData(self, obj):
        data = {'lastpostprocess': '',
                'lastgcodefile': '',
                'optionalstops': '',
                'programmer': '',
                'machine': '',
                'postprocessor': '',
                'postprocessorFlags': '',
                'filesize': '',
                'linecount': ''}
        try:
            data['lastpostprocess'] = str(obj.LastPostProcessDate)
            data['lastgcodefile'] = str(obj.LastPostProcessOutput)
            data['optionalstops'] = "False"
            data['programmer'] = ""
            data['machine'] = ""
            data['postprocessor'] = str(obj.PostProcessor)
            data['postprocessorFlags'] = str(obj.PostProcessorArgs)

            for op in obj.Operations.Group:
                if isinstance(op.Proxy, PathScripts.PathStop.Stop) and op.Stop is True:
                    data['optionalstops'] = "True"

            if obj.LastPostProcessOutput == "":
                data['filesize'] = str(0.0)
                data['linecount'] = str(0)
                self.squawk("PathSanity", "The Job has not been post-processed")
            else:
                data['filesize'] = str(os.path.getsize(obj.LastPostProcessOutput))
                data['linecount'] = str(sum(1 for line in open(obj.LastPostProcessOutput)))

        except Exception as e:
            data['errors'] = e
            self.squawk("PathSanity(__outputData)", e, squawkType="CAUTION")

        return data


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Sanity', CommandPathSanity())
