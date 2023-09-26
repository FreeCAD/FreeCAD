# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

"""
This file has utilities for checking and catching common errors in FreeCAD
Path projects.  Ideally, the user could execute these utilities from an icon
to make sure tools are selected and configured and defaults have been revised
"""

from PySide import QtGui
import FreeCAD
import FreeCADGui
import Path
import Path.Log
from collections import Counter
from datetime import datetime
import codecs
import os
import time
import webbrowser
import subprocess
from PySide.QtCore import QT_TRANSLATE_NOOP

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class CommandPathSanity:
    def resolveOutputFile(self, job):
        if job.PostProcessorOutputFile != "":
            filepath = job.PostProcessorOutputFile
        elif Path.Preferences.defaultOutputFile() != "":
            filepath = Path.Preferences.defaultOutputFile()
        else:
            filepath = Path.Preferences.macroFilePath()

        Path.Log.debug(filepath)

        D = FreeCAD.ActiveDocument.FileName
        if D:
            D = os.path.dirname(D)
            # in case the document is in the current working directory
            if not D:
                D = "."
        else:
            FreeCAD.Console.PrintError(
                "Please save document in order to resolve output path!\n"
            )
            return None
        filepath = filepath.replace("%D", D + os.path.sep)

        filepath = filepath.replace("%d", FreeCAD.ActiveDocument.Label)

        filepath = filepath.replace("%j", job.Label)

        if "%M" in filepath:
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
            M = pref.GetString("MacroPath", FreeCAD.getUserAppDataDir())
            filepath = filepath.replace("%M", M + os.path.sep)

        # strip out all substitutions related to output splitting
        for elem in ["%O", "%W", "%T", "%t", "%S"]:
            filepath = filepath.replace(elem, "")

        Path.Log.debug("filepath: {}".format(filepath))

        # if there's no basename left, use the activedocument basename
        fname = os.path.splitext(os.path.basename(filepath))
        if fname[0] == "":
            final = os.path.splitext(os.path.basename(FreeCAD.ActiveDocument.FileName))[
                0
            ]
            filepath = os.path.dirname(filepath) + os.path.sep + final + fname[1]

        Path.Log.debug("filepath: {}".format(filepath))

        return filepath

    def resolveOutputPath(self, filepath):

        # Make sure the filepath is fully qualified
        if os.path.basename(filepath) == filepath:
            filepath = f"{os.path.dirname(FreeCAD.ActiveDocument.FileName)}/{filepath}"

        Path.Log.debug("filepath: {}".format(filepath))

        # starting at the derived filename, iterate up until we have a valid
        # directory to write to
        while not os.path.isdir(filepath):
            filepath = os.path.dirname(filepath)

        Path.Log.debug("filepath: {}".format(filepath))
        return filepath + os.sep

    def GetResources(self):
        return {
            "Pixmap": "Path_Sanity",
            "MenuText": QT_TRANSLATE_NOOP(
                "Path_Sanity", "Check the path job for common errors"
            ),
            "Accel": "P, S",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_Sanity", "Check the path job for common errors"
            ),
        }

    def IsActive(self):
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        return isinstance(obj.Proxy, Path.Main.Job.ObjectJob)

    def Activated(self):
        # if everything is ok, execute

        if FreeCAD.GuiUp:
            currentCamera = FreeCADGui.ActiveDocument.ActiveView.getCameraType()
            if currentCamera != "Perspective":
                FreeCADGui.SendMsgToActiveView("PerspectiveCamera")
                FreeCADGui.updateGui()
                time.sleep(4)
                FreeCAD.Console.PrintLog(
                    "Path - Sanity - Changing to Perspective Camera temporarily\n"
                )
        FreeCADGui.addIconPath(":/icons")
        self.squawkData = {"items": []}
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        self.outputFile = self.resolveOutputFile(obj)
        self.outputpath = self.resolveOutputPath(self.outputFile)
        Path.Log.debug(f"outputstring: {self.outputpath}")
        data = self.__summarize(obj)
        html = self.__report(data, obj)
        if html is not None:
            webbrowser.open(html)
        if FreeCAD.GuiUp:
            if currentCamera != "Perspective":
                FreeCADGui.SendMsgToActiveView("OrthographicCamera")
                FreeCADGui.updateGui()
                time.sleep(0.5)
                FreeCAD.Console.PrintLog(
                    "Path - Sanity - Changing back to Orthographic Camera\n"
                )

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

        imagepath = self.outputpath + "{}".format(imageName)

        aview.viewIsometric()
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.SendMsgToActiveView("PerspectiveCamera")
        FreeCADGui.Selection.addSelection(obj)
        FreeCADGui.SendMsgToActiveView("ViewSelection")
        FreeCADGui.Selection.clearSelection()
        aview.saveImage(imagepath + ".png", 320, 320, "Current")
        aview.saveImage(imagepath + "_t.png", 320, 320, "Transparent")

        view.showMaximized()

        aview.setAnimationEnabled(True)

        # Restore visibility
        obj.Visibility = False
        for o in visible:
            o.Visibility = True

        return "{}_t.png".format(imagepath)

    def __report(self, data, obj):
        """
        generates an asciidoc file with the report information
        """
        Title = translate("Path_Sanity", "Setup Report for FreeCAD Job")
        ToC = translate("Path_Sanity", "Table of Contents")
        PartInfoHeading = translate("Path_Sanity", "Part Information")
        RunSumHeading = translate("Path_Sanity", "Run Summary")
        RoughStkHeading = translate("Path_Sanity", "Rough Stock")
        ToolDataHeading = translate("Path_Sanity", "Tool Data")
        OutputHeading = translate("Path_Sanity", "Output")
        FixturesHeading = translate("Path_Sanity", "Fixtures")
        SquawksHeading = translate("Path_Sanity", "Squawks")

        PartLabel = translate("Path_Sanity", "Base Object(s)")
        SequenceLabel = translate("Path_Sanity", "Job Sequence")
        DescriptionLabel = translate("Path_Sanity", "Job Description")
        JobTypeLabel = translate("Path_Sanity", "Job Type")
        CADLabel = translate("Path_Sanity", "CAD File Name")
        LastSaveLabel = translate("Path_Sanity", "Last Save Date")
        CustomerLabel = translate("Path_Sanity", "Customer")
        DesignerLabel = translate("Path_Sanity", "Designer")

        b = data["baseData"]
        d = data["designData"]
        jobname = d["JobLabel"]

        opLabel = translate("Path_Sanity", "Operation")
        zMinLabel = translate("Path_Sanity", "Minimum Z Height")
        zMaxLabel = translate("Path_Sanity", "Maximum Z Height")
        cycleTimeLabel = translate("Path_Sanity", "Cycle Time")

        coolantLabel = translate("Path_Sanity", "Coolant")
        jobTotalLabel = translate("Path_Sanity", "TOTAL JOB")
        d = data["toolData"]
        toolLabel = translate("Path_Sanity", "Tool Number")
        imageCounter = 1
        reportHtmlTemplate = """

<!DOCTYPE html>
<html>
<head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8"/>
	<title>Setup Report for FreeCAD Job: Path Special
</title>
	<style type="text/css">
       /* Light mode */
       :root {
          --body-bg: #FFFFFF;
          --body-color: #000000;
       }

        body { background: var(--body-bg); color: var(--body-color);}
		@page { size: 21cm 29.7cm; margin: 2cm }
		p { line-height: 115%; margin-bottom: 0.25cm; background: transparent }
		h1 { margin-bottom: 0.21cm; background: transparent; page-break-after: avoid }
		h1.western { font-family: "Liberation Serif", serif; font-size: 24pt; font-weight: normal }
		h1.cjk { font-family: "Noto Serif CJK SC"; font-size: 24pt; font-weight: normal }
		h1.ctl { font-family: "Lohit Devanagari"; font-size: 24pt; font-weight: normal }
        .ToC { font-size: 20pt; color: #7a2518 }
		h2 { margin-top: 0.35cm; margin-bottom: 0.21cm; background: transparent; page-break-after: avoid }
		h2.western { font-family: "Liberation Serif", serif; font-size: 18pt; font-weight: bold }
		h2.cjk { font-family: "Noto Serif CJK SC"; font-size: 18pt; font-weight: bold }
		h2.ctl { font-family: "Lohit Devanagari"; font-size: 18pt; font-weight: bold }
		td p { orphans: 0; widows: 0; background: transparent }
		h3 { margin-top: 0.25cm; margin-bottom: 0.21cm; background: transparent; page-break-after: avoid }
		h3.western { font-family: "Liberation Serif", serif; font-size: 14pt; font-weight: bold }
		h3.cjk { font-family: "Noto Serif CJK SC"; font-size: 14pt; font-weight: bold }
		h3.ctl { font-family: "Lohit Devanagari"; font-size: 14pt; font-weight: bold }
		strong { font-weight: bold }
		a:link { color: #000080; text-decoration: underline }
	</style>
</head>
<body><h1 class="western">
<font color="#000000"><font face="Open Sans, DejaVu Sans, sans-serif">"""
        reportHtmlTemplate += Title + ": " + jobname
        reportHtmlTemplate += """
</font></font></h1>
<div id="toc" dir="ltr"><p style="orphans: 2; widows: 2; margin-top: 0.21cm; margin-bottom: 0cm">
	<br/>

	</p>
	<div id="toctitle" dir="ltr"><p style="font-variant: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0.21cm; margin-bottom: 0cm">
		<span class="ToC "style="display: inline-block; border: none; padding: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif">
"""
        reportHtmlTemplate += ToC
        reportHtmlTemplate += """
</span></font></font></p>
	</div>
</div>
<p style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal">
<span style="display: inline-block; border: none; padding: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 12pt">
<font color="#ba3925"><a href="#_part_information"><font color="#2156a5"><span style="text-decoration: none">
"""
        reportHtmlTemplate += PartInfoHeading
        reportHtmlTemplate += """
</span></font></a></span></font></font></font></p>
<ul>
	<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
	<a href="#_run_summary"><span style="font-variant: normal"><font color="#2156a5"><span style="text-decoration: none"><font face="Open Sans, DejaVu Sans, sans-serif">
<font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal">
"""
        reportHtmlTemplate += RunSumHeading
        reportHtmlTemplate += """
</span></span></span></font></font></span></font></span></a></p>
	<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
	<span style="display: inline-block; border: none; padding: 0cm"><span style="font-variant: normal"><font color="#1d4b8f"><span style="text-decoration: none">
<font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal"><a href="#_rough_stock">
"""
        reportHtmlTemplate += RoughStkHeading
        reportHtmlTemplate += """
</span></span></span></span></font></font></span></font></span></a></p>
	<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
	<a href="#_tool_data"><span style="font-variant: normal"><font color="#2156a5"><span style="text-decoration: none"><font face="Open Sans, DejaVu Sans, sans-serif">
<font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal">
"""
        reportHtmlTemplate += ToolDataHeading
        reportHtmlTemplate += """
</span></span></span></font></font></span></font></span></a></p>"""
        for key, value in d.items():
            reportHtmlTemplate += """
	<ul>
		<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
		<a href='#"""
            reportHtmlTemplate += toolLabel + key + "'<"
            reportHtmlTemplate += """
<span style="font-variant: normal"><font color="#2156a5"><span style="text-decoration: none"><font face="Open Sans, DejaVu Sans, sans-serif">
<font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal">
"""
            reportHtmlTemplate += (
                toolLabel
                + ": T"
                + key
                + "</span></span></span></font></font></span></font></span></a></p>"
            )
            reportHtmlTemplate += """
	</ul>"""
        reportHtmlTemplate += """
	<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
	<a href="#_output_gcode"><span style="font-variant: normal"><font color="#2156a5"><span style="text-decoration: none"><font face="Open Sans, DejaVu Sans, sans-serif">
<font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal">
"""
        reportHtmlTemplate += OutputHeading + " (Gcode)"
        reportHtmlTemplate += """
</span></span></span></font></font></span></font></span></a></p>
	<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
	<a href="#_fixtures_and_workholding"><span style="font-variant: normal"><font color="#2156a5"><span style="text-decoration: none"><font face="Open Sans, DejaVu Sans, sans-serif">
<font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal">
"""
        reportHtmlTemplate += FixturesHeading
        reportHtmlTemplate += """
</span></span></span></font></font></span></font></span></a></p>
	<li><p style="line-height: 133%; orphans: 2; widows: 2; margin-bottom: 0cm; border: none; padding: 0cm">
	<a href="#_squawks"><span style="font-variant: normal"><font color="#2156a5"><span style="text-decoration: none"><font face="Open Sans, DejaVu Sans, sans-serif">
<font size="3" style="font-size: 12pt"><span style="letter-spacing: normal"><span style="font-style: normal"><span style="font-weight: normal">
"""
        reportHtmlTemplate += SquawksHeading
        reportHtmlTemplate += """
</span></span></span></font></font></span></font></span></a></p>
</ul>
<p style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal">
<br/>
<br/>

</p>
<h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal"><a name="_part_information"></a>
<span style="display: inline-block; border: none; padding: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += PartInfoHeading
        reportHtmlTemplate += """
</span></font></font></font></h2>
<table style="table-layout: fixed; cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="200"/>

	<col width="525"/>

	<col width="250"/>

	<tr valign=top>
		<td style="border: 1px solid #dedede; padding: 0.05cm"; top><p align="left" "style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += PartLabel
        reportHtmlTemplate += """
</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm">
			<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
				<col width="175"/>

				<col width="175"/>"""
        d = data["designData"]
        for key, val in b["bases"].items():
            reportHtmlTemplate += """
				<tr>
					<td style='border: 1px solid #dedede; padding: 0.05cm'><p align='left' style='border: none; padding: 0cm'>
						<font color='#000000'>"""
            reportHtmlTemplate += key
            reportHtmlTemplate += """
</font></p>
					</td>
					<td style='border: 1px solid #dedede; padding: 0.05cm'><p align='left' style='border: none; padding: 0cm'>
						<font color='#000000'>"""
            reportHtmlTemplate += val
            reportHtmlTemplate += """
</font></p>
					</td>
				</tr>"""
        reportHtmlTemplate += """
			</table>
			<p><br/>

			</p>
		</td>
		<td rowspan="7" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%">
			<span style="display: inline-block; border: none; padding: 0cm"><font color="#000000"><img src='
"""
        reportHtmlTemplate += b["baseimage"] + "'" + "name='Image" + str(imageCounter)
        reportHtmlTemplate += "' alt='Base Object(s)' align='bottom'"
        reportHtmlTemplate += " width='320' height='320' border='0'/>"
        imageCounter += 1
        reportHtmlTemplate += """
</span></font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += SequenceLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["Sequence"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += JobTypeLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["JobType"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += DescriptionLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["JobDescription"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += CADLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="word-wrap: break-word; border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["FileName"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += LastSaveLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["LastModifiedDate"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += CustomerLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["Customer"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
</table>"""

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

        xDimLabel = translate("Path_Sanity", "X Size")
        yDimLabel = translate("Path_Sanity", "Y Size")
        zDimLabel = translate("Path_Sanity", "Z Size")
        materialLabel = translate("Path_Sanity", "Material")

        offsetsLabel = translate("Path_Sanity", "Work Offsets")
        orderByLabel = translate("Path_Sanity", "Order By")
        datumLabel = translate("Path_Sanity", "Part Datum")

        gcodeFileLabel = translate("Path_Sanity", "G-code File")
        lastpostLabel = translate("Path_Sanity", "Last Post Process Date")
        stopsLabel = translate("Path_Sanity", "Stops")
        programmerLabel = translate("Path_Sanity", "Programmer")
        machineLabel = translate("Path_Sanity", "Machine")
        postLabel = translate("Path_Sanity", "Postprocessor")
        flagsLabel = translate("Path_Sanity", "Post Processor Flags")
        fileSizeLabel = translate("Path_Sanity", "File Size (kB)")
        lineCountLabel = translate("Path_Sanity", "Line Count")

        noteLabel = translate("Path_Sanity", "Note")
        operatorLabel = translate("Path_Sanity", "Operator")
        dateLabel = translate("Path_Sanity", "Date")

        d = data["runData"]
        reportHtmlTemplate += """
<p><h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm"><a name="_run_summary"></a>
<span style="display: inline-block; border-top: 1px solid #e7e7e9; border-bottom: none; border-left: none; border-right: none; padding-top: 0.05cm; padding-bottom: 0cm; padding-left: 0cm; padding-right: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += RunSumHeading
        reportHtmlTemplate += """
</span></font></font></font></h2>"""
        for i in d["items"]:
            reportHtmlTemplate += """

<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="210"/>

	<col width="210"/>

	<col width="210"/>

	<col width="210"/>

	<col width="210"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += opLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += zMinLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += zMaxLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += coolantLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += cycleTimeLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["opName"]
            reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["minZ"]
            reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["maxZ"]
            reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["coolantMode"]
            reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["cycleTime"]
            reportHtmlTemplate += """
</font></p>
		</td>
	</tr>
</table>"""
        d = data["stockData"]
        reportHtmlTemplate += """
<p><h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm"><a name="_rough_stock"></a>
<span style="display: inline-block; border-top: 1px solid #e7e7e9; border-bottom: none; border-left: none; border-right: none; padding-top: 0.05cm; padding-bottom: 0cm; padding-left: 0cm; padding-right: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += RoughStkHeading
        reportHtmlTemplate += """
</span></font></font></font></h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="350"/>

	<col width="350"/>

	<col width="350"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += materialLabel
        reportHtmlTemplate += """
</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["material"] + "</font></p>"
        reportHtmlTemplate += """
		<td rowspan="4" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left">
			<span style="display: inline-block; border: none; padding: 0cm"><font color="#000000"><img src='
"""
        reportHtmlTemplate += d["stockImage"] + "'" + "name='Image" + str(imageCounter)
        reportHtmlTemplate += "' alt='stock' align='bottom' width='320' height='320' border='0'/>"
        imageCounter += 1
        reportHtmlTemplate += """
</span></font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += xDimLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["xLen"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += yDimLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["yLen"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += zDimLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["zLen"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
</table>"""
        d = data["toolData"]
        reportHtmlTemplate += """
<p><h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm"><a name="_tool_data"></a>
<span style="display: inline-block; border-top: 1px solid #e7e7e9; border-bottom: none; border-left: none; border-right: none; padding-top: 0.05cm; padding-bottom: 0cm; padding-left: 0cm; padding-right: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += ToolDataHeading
        reportHtmlTemplate += """
</span></font></font></font></h2>"""
        for key, value in d.items():
            reportHtmlTemplate += """
<h3 class='western' style='font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm'><a name='
"""
            reportHtmlTemplate += toolLabel + key
            reportHtmlTemplate += "'>"
            reportHtmlTemplate += """
</a><span style='display: inline-block; border: none; padding: 0cm'><font face='Open Sans, DejaVu Sans, sans-serif'><font size='3' style='font-size: 16pt'><font color='#ba3925'>
"""
            reportHtmlTemplate += toolLabel + ": T" + key
            reportHtmlTemplate += """</span></font></font></font></h3>"""
            reportHtmlTemplate += """
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="350"/>

	<col width="350"/>

	<col width="350"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += descriptionLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["description"]
            reportHtmlTemplate += """
</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left">
			<span style="display: inline-block; border: none; padding: 0cm"><font color="#000000"><img src='
"""
            try:
                reportHtmlTemplate += value["imagepath"]
            except:
                pass
            reportHtmlTemplate += (
                "' name='Image"
                + str(imageCounter)
                + "' alt='2' align='bottom' width='135' height='135' border='0'/>"
            )
            reportHtmlTemplate += """
</span></font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += manufLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["manufacturer"]
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += partNumberLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td colspan="2" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["partNumber"]
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += urlLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td colspan="2" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["url"]
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += shapeLabel
            reportHtmlTemplate += """</b></font></strong></p>
		<td colspan="2" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["shape"]
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += inspectionNotesLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td colspan="2" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["inspectionNotes"]
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
            reportHtmlTemplate += diameterLabel
            reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td colspan="2" style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += value["diameter"]
            imageCounter += 1
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>
</table>"""
            for o in value["ops"]:
                reportHtmlTemplate += """
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="262"/>

	<col width="262"/>

	<col width="262"/>

	<col width="262"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
                reportHtmlTemplate += opLabel
                reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
                reportHtmlTemplate += tcLabel
                reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
                reportHtmlTemplate += feedLabel
                reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
                reportHtmlTemplate += speedLabel
                reportHtmlTemplate += """</b></font></strong></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
                reportHtmlTemplate += o["Operation"]
                reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
                reportHtmlTemplate += o["ToolController"]
                reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
                reportHtmlTemplate += o["Feed"]
                reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
                reportHtmlTemplate += o["Speed"]
                reportHtmlTemplate += """</font></p>
		</td>
	</tr>
</table>"""
        d = data["outputData"]
        reportHtmlTemplate += """
<p><h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm"><a name="_output_gcode"></a>
<span style="display: inline-block; border-top: 1px solid #e7e7e9; border-bottom: none; border-left: none; border-right: none; padding-top: 0.05cm; padding-bottom: 0cm; padding-left: 0cm; padding-right: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += OutputHeading + " (Gcode)"
        reportHtmlTemplate += """
</span></font></font></font></h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="525"/>

	<col width="525"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += gcodeFileLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["lastgcodefile"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += lastpostLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["lastpostprocess"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += stopsLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["optionalstops"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += programmerLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["programmer"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += machineLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["machine"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += postLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["postprocessor"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += flagsLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["postprocessorFlags"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += fileSizeLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["filesize"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += lineCountLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["linecount"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
</table>"""
        d = data["fixtureData"]
        reportHtmlTemplate += """
<p><h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm"><a name="_fixtures_and_workholding"></a>
<span style="display: inline-block; border-top: 1px solid #e7e7e9; border-bottom: none; border-left: none; border-right: none; padding-top: 0.05cm; padding-bottom: 0cm; padding-left: 0cm; padding-right: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += FixturesHeading
        reportHtmlTemplate += """
</span></font></font></font></h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="525"/>

	<col width="525"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += offsetsLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["fixtures"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += orderByLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<font color="#000000">"""
        reportHtmlTemplate += d["orderBy"]
        reportHtmlTemplate += """</font></p>
		</td>
	</tr>
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000">"""
        reportHtmlTemplate += datumLabel
        reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left">
			<span style="display: inline-block; border: none; padding: 0cm"><font color="#000000"><img src='
"""
        reportHtmlTemplate += d["datumImage"] + "'" + "name='Image" + str(imageCounter)
        reportHtmlTemplate += "' alt='origin t' align='bottom'"
        reportHtmlTemplate += " width='320' height='320' border='0'/>"
        imageCounter += 1
        reportHtmlTemplate += """
</span></font></p>
		</td>
	</tr>
</table>"""
        d = data["squawkData"]
        TIPIcon = FreeCAD.getHomePath() + "Mod/Path/Path/Main/Gui/Sanity_Bulb.svg"
        NOTEIcon = FreeCAD.getHomePath() + "Mod/Path/Path/Main/Gui/Sanity_Note.svg"
        WARNINGIcon = (
            FreeCAD.getHomePath() + "Mod/Path/Path/Main/Gui/Sanity_Warning.svg"
        )
        CAUTIONIcon = FreeCAD.getHomePath() + "Mod/Path/Path/Main/Gui/Sanity_Stop.svg"

        reportHtmlTemplate += """
<p><h2 class="western" style="font-variant: normal; letter-spacing: normal; font-style: normal; font-weight: normal; line-height: 120%; orphans: 2; widows: 2; margin-top: 0cm; margin-bottom: 0cm; border: none; padding: 0cm"><a name="_squawks"></a>
<span style="display: inline-block; border-top: 1px solid #e7e7e9; border-bottom: none; border-left: none; border-right: none; padding-top: 0.05cm; padding-bottom: 0cm; padding-left: 0cm; padding-right: 0cm"><font face="Open Sans, DejaVu Sans, sans-serif"><font size="3" style="font-size: 20pt"><font color="#ba3925">
"""
        reportHtmlTemplate += SquawksHeading
        reportHtmlTemplate += """
</span></font></font></font></h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff">
	<col width="350"/>

	<col width="350"/>

	<col width="350"/>

	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += noteLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += operatorLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="border: none; padding: 0cm">
			<strong><font color="#000000"><b>"""
        reportHtmlTemplate += dateLabel
        reportHtmlTemplate += """</b></font></strong></p>
		</td>
	</tr>"""
        for i in d["items"]:
            reportHtmlTemplate += """
	<tr>
		<td style="border: 1px solid #dedede; padding: 0.05cm">
			<table cellpadding="2" cellspacing="2">
				<tr>
					<td style="border-top: none; border-bottom: none; border-left: 1px solid #dddddf; border-right: none; padding-top: 0cm; padding-bottom: 0cm; padding-left: 0.05cm; padding-right: 0cm"><p>
						<font color="#000000">"""
            if str(i["squawkType"]) == "TIP":
                reportHtmlTemplate += (
                    "<img src='"
                    + TIPIcon
                    + "' "
                    + " name='Image"
                    + str(imageCounter)
                    + "' alt='TIP' align='middle' width='32' height='32' border='0'/>"
                    + "<td style='border-top: none; border-bottom: none;"
                    + " border-left: 1px solid #dddddf;"
                    + " border-right: none; padding-top: 0cm;"
                    + " padding-bottom: 0cm;"
                    + " padding-left: 0.05cm;"
                    + " padding-right: 0cm'><p>"
                    + str(i["Note"])
                )
            if str(i["squawkType"]) == "NOTE":
                reportHtmlTemplate += (
                    "<img src='"
                    + NOTEIcon
                    + "' "
                    + " name='Image"
                    + str(imageCounter)
                    + "' alt='NOTE' align='middle' width='32' height='32' border='0'/>"
                    + "<td style='border-top: none; border-bottom: none;"
                    + " border-left: 1px solid #dddddf;"
                    + " border-right: none; padding-top: 0cm;"
                    + " padding-bottom: 0cm;"
                    + " padding-left: 0.05cm;"
                    + " padding-right: 0cm'><p>"
                    + str(i["Note"])
                )
            if str(i["squawkType"]) == "WARNING":
                reportHtmlTemplate += (
                    "<img src='"
                    + WARNINGIcon
                    + "' "
                    + " name='Image"
                    + str(imageCounter)
                    + "' alt='WARNING' align='middle' width='32' height='32' border='0'/>"
                    + "<td style='border-top: none; border-bottom: none;"
                    + " border-left: 1px solid #dddddf;"
                    + " border-right: none; padding-top: 0cm;"
                    + " padding-bottom: 0cm;"
                    + " padding-left: 0.05cm;"
                    + " padding-right: 0cm'><p>"
                    + str(i["Note"])
                )
            if str(i["squawkType"]) == "CAUTION":
                reportHtmlTemplate += (
                    "<img src='"
                    + CAUTIONIcon
                    + "' "
                    + " name='Image"
                    + str(imageCounter)
                    + "' alt='CAUTION' align='middle' width='32' height='32' border='0'/>"
                    + "<td style='border-top: none; border-bottom: none;"
                    + " border-left: 1px solid #dddddf;"
                    + " border-right: none; padding-top: 0cm;"
                    + " padding-bottom: 0cm;"
                    + " padding-left: 0.05cm;"
                    + " padding-right: 0cm'><p>"
                    + str(i["Note"])
                )
            imageCounter += 1
            reportHtmlTemplate += """</font></p>
					</td>
				</tr>
			</table>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["Operator"]
            reportHtmlTemplate += """</font></p>
		</td>
		<td style="border: 1px solid #dedede; padding: 0.05cm"><p align="left" style="line-height: 160%; border: none; padding: 0cm">
			<font color="#000000">"""
            reportHtmlTemplate += i["Date"]
            reportHtmlTemplate += """</font></p>
		</td>
	</tr>"""
        reportHtmlTemplate += """
</table>
<p style="line-height: 100%; margin-bottom: 0cm"><br/>

</p>
</body>
</html>
"""


        # Save the report
        subsLookup = os.path.splitext(os.path.basename(obj.PostProcessorOutputFile))[0]
        foundSub = False

        for elem in ["%D", "%d", "%M", "%j"]:
            if elem in subsLookup:
                foundSub = True
                break

        if foundSub:
            filepath = self.resolveOutputFile(obj)
            Path.Log.debug("filepath: {}".format(filepath))

            # Make sure the filepath is fully qualified
            if os.path.basename(filepath) == filepath:
                filepath = f"{os.path.dirname(FreeCAD.ActiveDocument.FileName)}/{filepath}"
            Path.Log.debug("filepath: {}".format(filepath))
            base_name = os.path.splitext(filepath)[0]
            reporthtml = base_name + ".html"
        else:
            reporthtml = self.outputpath + data["outputData"]["outputfilename"] + ".html"


        # Python 3.11 aware
        with codecs.open(reporthtml, encoding="utf-8", mode="w") as fd:
            fd.write(reportHtmlTemplate)
            fd.close()
            FreeCAD.Console.PrintMessage("html file written to {}\n".format(reporthtml))

        return reporthtml

    def __summarize(self, obj):
        """
        Top level function to summarize information for the report
        Returns a dictionary of sections
        """
        data = {}
        data["baseData"] = self.__baseObjectData(obj)
        data["designData"] = self.__designData(obj)
        data["toolData"] = self.__toolData(obj)
        data["runData"] = self.__runData(obj)
        data["outputData"] = self.__outputData(obj)
        data["fixtureData"] = self.__fixtureData(obj)
        data["stockData"] = self.__stockData(obj)
        data["squawkData"] = self.squawkData
        return data

    def squawk(self, operator, note, date=datetime.now(), squawkType="NOTE"):
        squawkType = (
            squawkType
            if squawkType in ["NOTE", "WARNING", "CAUTION", "TIP"]
            else "NOTE"
        )

        self.squawkData["items"].append(
            {
                "Date": str(date),
                "Operator": operator,
                "Note": note,
                "squawkType": squawkType,
            }
        )

    def __baseObjectData(self, obj):
        data = {"baseimage": "", "bases": ""}
        try:
            bases = {}
            for name, count in Counter(
                [obj.Proxy.baseObject(obj, o).Label for o in obj.Model.Group]
            ).items():
                bases[name] = str(count)

            data["baseimage"] = self.__makePicture(obj.Model, "baseimage")
            data["bases"] = bases

        except Exception as e:
            data["errors"] = e
            self.squawk("PathSanity(__baseObjectData)", e, squawkType="CAUTION")

        return data

    def __designData(self, obj):
        """
        Returns header information about the design document
        Returns information about issues and concerns (squawks)
        """

        data = {
            "FileName": "",
            "LastModifiedDate": "",
            "Customer": "",
            "Designer": "",
            "JobDescription": "",
            "JobLabel": "",
            "Sequence": "",
            "JobType": "",
        }
        try:
            data["FileName"] = obj.Document.FileName
            data["LastModifiedDate"] = str(obj.Document.LastModifiedDate)
            data["Customer"] = obj.Document.Company
            data["Designer"] = obj.Document.LastModifiedBy
            data["JobDescription"] = obj.Description
            data["JobLabel"] = obj.Label

            n = 0
            m = 0
            for i in obj.Document.Objects:
                if hasattr(i, "Proxy"):
                    if isinstance(i.Proxy, Path.Main.Job.ObjectJob):
                        m += 1
                        if i is obj:
                            n = m
            data["Sequence"] = "{} of {}".format(n, m)
            data["JobType"] = "2.5D Milling"  # improve after job types added

        except Exception as e:
            data["errors"] = e
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
            for TC in obj.Tools.Group:
                if not hasattr(TC.Tool, "BitBody"):
                    self.squawk(
                        "PathSanity",
                        translate(
                            "Path_Sanity",
                            "Tool number {} is a legacy tool. Legacy tools not \
                        supported by Path-Sanity",
                        ).format(TC.ToolNumber),
                        squawkType="WARNING",
                    )
                    continue  # skip old-style tools
                tooldata = data.setdefault(str(TC.ToolNumber), {})
                bitshape = tooldata.setdefault("BitShape", "")
                if bitshape not in ["", TC.Tool.BitShape]:
                    self.squawk(
                        "PathSanity",
                        translate(
                            "Path_Sanity", "Tool number {} used by multiple tools"
                        ).format(TC.ToolNumber),
                        squawkType="CAUTION",
                    )
                tooldata["bitShape"] = TC.Tool.BitShape
                tooldata["description"] = TC.Tool.Label
                tooldata["manufacturer"] = ""
                tooldata["url"] = ""
                tooldata["inspectionNotes"] = ""
                tooldata["diameter"] = str(TC.Tool.Diameter)
                tooldata["shape"] = TC.Tool.ShapeName

                tooldata["partNumber"] = ""
                imagedata = TC.Tool.Proxy.getBitThumbnail(TC.Tool)
                imagepath = "{}T{}.png".format(self.outputpath, TC.ToolNumber)
                tooldata["feedrate"] = str(TC.HorizFeed)
                if TC.HorizFeed.Value == 0.0:
                    self.squawk(
                        "PathSanity",
                        "Tool Controller '{}' has no feedrate".format(TC.Label),
                        squawkType="WARNING",
                    )

                tooldata["spindlespeed"] = str(TC.SpindleSpeed)
                if TC.SpindleSpeed == 0.0:
                    self.squawk(
                        "PathSanity",
                        translate(
                            "Path_Sanity", "Tool Controller '{}' has no spindlespeed"
                        ).format(TC.Label),
                        squawkType="WARNING",
                    )

                with open(imagepath, "wb") as fd:
                    fd.write(imagedata)
                    fd.close()
                tooldata["imagepath"] = imagepath

                used = False
                for op in obj.Operations.Group:
                    if hasattr(op, "ToolController") and op.ToolController is TC:
                        used = True
                        tooldata.setdefault("ops", []).append(
                            {
                                "Operation": op.Label,
                                "ToolController": TC.Label,
                                "Feed": str(TC.HorizFeed),
                                "Speed": str(TC.SpindleSpeed),
                            }
                        )

                if used is False:
                    tooldata.setdefault("ops", [])
                    self.squawk(
                        "PathSanity",
                        translate(
                            "Path_Sanity", "Tool Controller '{}' is not used"
                        ).format(TC.Label),
                        squawkType="WARNING",
                    )

        except Exception as e:
            data["errors"] = e
            self.squawk("PathSanity(__toolData)", e, squawkType="CAUTION")

        return data

    def __runData(self, obj):
        data = {
            "cycletotal": "",
            "jobMinZ": "",
            "jobMaxZ": "",
            "jobDescription": "",
            "items": [],
        }
        try:
            data["cycletotal"] = str(obj.CycleTime)
            data["jobMinZ"] = FreeCAD.Units.Quantity(
                obj.Path.BoundBox.ZMin, FreeCAD.Units.Length
            ).UserString
            data["jobMaxZ"] = FreeCAD.Units.Quantity(
                obj.Path.BoundBox.ZMax, FreeCAD.Units.Length
            ).UserString
            data["jobDescription"] = obj.Description

            data["items"] = []
            for op in obj.Operations.Group:
                oplabel = op.Label
                ctime = op.CycleTime if hasattr(op, "CycleTime") else 0.0
                cool = op.CoolantMode if hasattr(op, "CoolantMode") else "N/A"

                o = op
                while len(o.ViewObject.claimChildren()) != 0:  # dressup
                    oplabel = "{}:{}".format(oplabel, o.Base.Label)
                    o = o.Base
                    if hasattr(o, "CycleTime"):
                        ctime = o.CycleTime
                    cool = o.CoolantMode if hasattr(o, "CoolantMode") else cool

                if hasattr(op, "Active") and not op.Active:
                    oplabel = "{} (INACTIVE)".format(oplabel)
                    ctime = 0.0

                if op.Path.BoundBox.isValid():
                    zmin = FreeCAD.Units.Quantity(
                        op.Path.BoundBox.ZMin, FreeCAD.Units.Length
                    ).UserString
                    zmax = FreeCAD.Units.Quantity(
                        op.Path.BoundBox.ZMax, FreeCAD.Units.Length
                    ).UserString
                else:
                    zmin = ""
                    zmax = ""

                opdata = {
                    "opName": oplabel,
                    "minZ": zmin,
                    "maxZ": zmax,
                    "cycleTime": ctime,
                    "coolantMode": cool,
                }
                data["items"].append(opdata)

        except Exception as e:
            data["errors"] = e
            self.squawk("PathSanity(__runData)", e, squawkType="CAUTION")

        return data

    def __stockData(self, obj):
        data = {"xLen": "", "yLen": "", "zLen": "", "material": "", "stockImage": ""}

        try:
            bb = obj.Stock.Shape.BoundBox
            data["xLen"] = FreeCAD.Units.Quantity(
                bb.XLength, FreeCAD.Units.Length
            ).UserString
            data["yLen"] = FreeCAD.Units.Quantity(
                bb.YLength, FreeCAD.Units.Length
            ).UserString
            data["zLen"] = FreeCAD.Units.Quantity(
                bb.ZLength, FreeCAD.Units.Length
            ).UserString

            data["material"] = "Not Specified"
            if hasattr(obj.Stock, "Material"):
                if obj.Stock.Material is not None:
                    data["material"] = obj.Stock.Material.Material["Name"]

            if data["material"] == "Not Specified":
                self.squawk(
                    "PathSanity",
                    translate("Path_Sanity", "Consider Specifying the Stock Material"),
                    squawkType="TIP",
                )

            data["stockImage"] = self.__makePicture(obj.Stock, "stockImage")
        except Exception as e:
            data["errors"] = e
            self.squawk("PathSanity(__stockData)", e, squawkType="CAUTION")

        return data

    def __fixtureData(self, obj):
        data = {"fixtures": "", "orderBy": "", "datumImage": ""}
        try:
            data["fixtures"] = str(obj.Fixtures)
            data["orderBy"] = str(obj.OrderOutputBy)

            aview = FreeCADGui.activeDocument().activeView()
            aview.setAnimationEnabled(False)

            obj.Visibility = False
            obj.Operations.Visibility = False

            mw = FreeCADGui.getMainWindow()
            mdi = mw.findChild(QtGui.QMdiArea)
            view = mdi.activeSubWindow()
            view.showNormal()
            view.resize(320, 320)

            imagepath = "{}origin".format(self.outputpath)

            FreeCADGui.Selection.clearSelection()
            FreeCADGui.SendMsgToActiveView("PerspectiveCamera")
            aview.viewIsometric()
            for i in obj.Model.Group:
                FreeCADGui.Selection.addSelection(i)
            FreeCADGui.SendMsgToActiveView("ViewSelection")
            FreeCADGui.Selection.clearSelection()
            obj.ViewObject.Proxy.editObject(obj)
            Path.Log.debug(imagepath)
            aview.saveImage("{}.png".format(imagepath), 320, 320, "Current")
            aview.saveImage("{}_t.png".format(imagepath), 320, 320, "Transparent")
            obj.ViewObject.Proxy.uneditObject(obj)
            obj.Visibility = True
            obj.Operations.Visibility = True

            view.showMaximized()

            aview.setAnimationEnabled(True)
            data["datumImage"] = "{}_t.png".format(imagepath)

        except Exception as e:
            data["errors"] = e
            self.squawk("PathSanity(__fixtureData)", e, squawkType="CAUTION")

        return data

    def __outputData(self, obj):
        data = {
            "lastpostprocess": "",
            "lastgcodefile": "",
            "optionalstops": "",
            "programmer": "",
            "machine": "",
            "postprocessor": "",
            "postprocessorFlags": "",
            "filesize": "",
            "linecount": "",
            "outputfilename": "setupreport",
        }
        try:
            data["lastpostprocess"] = str(obj.LastPostProcessDate)
            data["lastgcodefile"] = str(obj.LastPostProcessOutput)
            data["optionalstops"] = "False"
            data["programmer"] = ""
            data["machine"] = ""
            data["postprocessor"] = str(obj.PostProcessor)
            data["postprocessorFlags"] = str(obj.PostProcessorArgs)

            if obj.PostProcessorOutputFile != "":
                fname = obj.PostProcessorOutputFile
                data["outputfilename"] = os.path.splitext(os.path.basename(fname))[0]
            for op in obj.Operations.Group:
                if isinstance(op.Proxy, Path.Op.Gui.Stop.Stop) and op.Stop is True:
                    data["optionalstops"] = "True"

            if obj.LastPostProcessOutput == "":
                data["filesize"] = str(0.0)
                data["linecount"] = str(0)
                self.squawk(
                    "PathSanity",
                    translate("Path_Sanity", "The Job has not been post-processed"),
                )
            else:
                data["filesize"] = str(
                    os.path.getsize(obj.LastPostProcessOutput) / 1000
                )
                data["linecount"] = str(
                    sum(1 for line in open(obj.LastPostProcessOutput))
                )

        except Exception as e:
            data["errors"] = e
            self.squawk("PathSanity(__outputData)", e, squawkType="CAUTION")

        return data


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_Sanity", CommandPathSanity())
