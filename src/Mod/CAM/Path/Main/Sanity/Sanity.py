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
CAM projects.  Ideally, the user could execute these utilities from an icon
to make sure tools are selected and configured and defaults have been revised
"""

from collections import Counter
from datetime import datetime
import FreeCAD
import Path
import Path.Log
import Path.Main.Sanity.ImageBuilder as ImageBuilder
import Path.Main.Sanity.ReportGenerator as ReportGenerator
import os
import tempfile
import Path.Dressup.Utils as PathDressup

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class CAMSanity:
    """
    This class has the functionality to harvest data from a CAM Job
    and export it in a format that is useful to the user.
    """

    def __init__(self, job, output_file):
        self.job = job
        self.output_file = output_file
        self.filelocation = os.path.dirname(output_file)

        # set the filelocation to the parent of the output filename
        if not os.path.isdir(self.filelocation):
            raise ValueError(
                translate(
                    "CAM_Sanity",
                    "output location {} doesn't exist".format(os.path.dirname(output_file)),
                )
            )

        self.image_builder = ImageBuilder.ImageBuilderFactory.get_image_builder(self.filelocation)
        self.data = self.summarize()

    def summarize(self):
        """
        Gather all the incremental parts of the analysis
        """

        data = {}
        data["baseData"] = self._baseObjectData()
        data["designData"] = self._designData()
        data["toolData"] = self._toolData()
        data["runData"] = self._runData()
        data["outputData"] = self._outputData()
        data["fixtureData"] = self._fixtureData()
        data["stockData"] = self._stockData()
        # data["squawkData"] = self._squawkData()

        return data

    def squawk(self, operator, note, date=datetime.now(), squawkType="NOTE"):
        squawkType = squawkType if squawkType in ("NOTE", "WARNING", "CAUTION", "TIP") else "NOTE"

        if squawkType == "TIP":
            squawk_icon = "Sanity_Bulb"
        elif squawkType == "NOTE":
            squawk_icon = "Sanity_Note"
        elif squawkType == "WARNING":
            squawk_icon = "Sanity_Warning"
        elif squawkType == "CAUTION":
            squawk_icon = "Sanity_Caution"

        path = f"{FreeCAD.getHomePath()}Mod/CAM/Path/Main/Sanity/{squawk_icon}.svg"

        squawk = {
            "Date": str(date),
            "Operator": operator,
            "Note": note,
            "squawkType": squawkType,
            "squawkIcon": path,
        }

        return squawk

    def _baseObjectData(self):
        data = {"baseimage": "", "bases": "", "squawkData": []}
        obj = self.job
        bases = {}
        for name, count in Counter(
            [obj.Proxy.baseObject(obj, o).Label for o in obj.Model.Group]
        ).items():
            bases[name] = str(count)
        data["baseimage"] = self.image_builder.build_image(obj.Model, "baseimage")
        data["bases"] = bases

        return data

    def _designData(self):
        """
        Returns header information about the design document
        Returns information about issues and concerns (squawks)
        """

        obj = self.job
        data = {
            "FileName": "",
            "LastModifiedDate": "",
            "Customer": "",
            "Designer": "",
            "JobDescription": "",
            "JobLabel": "",
            "Sequence": "",
            "JobType": "",
            "squawkData": [],
        }
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

        return data

    def _fixtureData(self):
        obj = self.job
        data = {"fixtures": "", "orderBy": "", "datumImage": "", "squawkData": []}

        data["fixtures"] = str(obj.Fixtures)
        data["orderBy"] = str(obj.OrderOutputBy)

        data["datumImage"] = self.image_builder.build_image(obj, "datumImage")

        return data

    def _outputData(self):
        obj = self.job
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
            "squawkData": [],
        }

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
            if "Stop" in op.Name and hasattr(op, "Stop") and op.Stop is True:
                data["optionalstops"] = "True"

        if obj.LastPostProcessOutput == "":
            data["filesize"] = str(0.0)
            data["linecount"] = str(0)
            data["squawkData"].append(
                self.squawk(
                    "CAMSanity",
                    translate("CAM_Sanity", "The Job has not been post-processed"),
                )
            )
        else:
            if os.path.isfile(obj.LastPostProcessOutput):
                data["filesize"] = str(os.path.getsize(obj.LastPostProcessOutput) / 1000)
                data["linecount"] = str(sum(1 for line in open(obj.LastPostProcessOutput)))
            else:
                data["filesize"] = str(0.0)
                data["linecount"] = str(0)
                data["squawkData"].append(
                    self.squawk(
                        "CAMSanity",
                        translate(
                            "CAM_Sanity",
                            "The Job's last post-processed file is missing",
                        ),
                    )
                )

        return data

    def _runData(self):
        obj = self.job
        data = {
            "cycletotal": "",
            "jobMinZ": "",
            "jobMaxZ": "",
            "jobDescription": "",
            "operations": [],
            "squawkData": [],
        }

        data["cycletotal"] = str(obj.CycleTime)
        data["jobMinZ"] = FreeCAD.Units.Quantity(
            obj.Path.BoundBox.ZMin, FreeCAD.Units.Length
        ).UserString
        data["jobMaxZ"] = FreeCAD.Units.Quantity(
            obj.Path.BoundBox.ZMax, FreeCAD.Units.Length
        ).UserString
        data["jobDescription"] = obj.Description

        data["operations"] = []
        for op in obj.Operations.Group:
            oplabel = op.Label
            Path.Log.debug(oplabel)
            ctime = op.CycleTime if hasattr(op, "CycleTime") else "00:00:00"
            cool = op.CoolantMode if hasattr(op, "CoolantMode") else "N/A"

            o = op
            while "Dressup" in o.Name:
                oplabel = "{}:{}".format(oplabel, o.Base.Label)
                o = o.Base
                if hasattr(o, "CycleTime"):
                    ctime = o.CycleTime
                cool = o.CoolantMode if hasattr(o, "CoolantMode") else cool

            if hasattr(op, "Active") and not op.Active:
                oplabel = "{} (INACTIVE)".format(oplabel)
                ctime = "00:00:00"

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
            data["operations"].append(opdata)

        return data

    def _stockData(self):
        obj = self.job
        data = {
            "xLen": "",
            "yLen": "",
            "zLen": "",
            "material": "",
            "surfaceSpeedCarbide": "",
            "surfaceSpeedHSS": "",
            "stockImage": "",
            "squawkData": [],
        }

        bb = obj.Stock.Shape.BoundBox
        data["xLen"] = FreeCAD.Units.Quantity(bb.XLength, FreeCAD.Units.Length).UserString
        data["yLen"] = FreeCAD.Units.Quantity(bb.YLength, FreeCAD.Units.Length).UserString
        data["zLen"] = FreeCAD.Units.Quantity(bb.ZLength, FreeCAD.Units.Length).UserString

        data["material"] = "Not Specified"
        if hasattr(obj.Stock, "ShapeMaterial"):
            if obj.Stock.ShapeMaterial is not None:
                data["material"] = obj.Stock.ShapeMaterial.Name

            props = obj.Stock.ShapeMaterial.PhysicalProperties
            if "SurfaceSpeedCarbide" in props:
                data["surfaceSpeedCarbide"] = FreeCAD.Units.Quantity(
                    props["SurfaceSpeedCarbide"]
                ).UserString
            if "SurfaceSpeedHSS" in props:
                data["surfaceSpeedHSS"] = FreeCAD.Units.Quantity(
                    props["SurfaceSpeedHSS"]
                ).UserString

        if data["material"] in ["Default", "Not Specified"]:
            data["squawkData"].append(
                self.squawk(
                    "CAMSanity",
                    translate("CAM_Sanity", "Consider Specifying the Stock Material"),
                    squawkType="TIP",
                )
            )

        data["stockImage"] = self.image_builder.build_image(obj.Stock, "stockImage")

        return data

    def _toolData(self):
        """
        Returns information about the tools used in the job, and associated
        toolcontrollers
        Returns information about issues and problems with the tools (squawks)
        """

        obj = self.job
        data = {"squawkData": []}

        for TC in obj.Tools.Group:
            if not hasattr(TC.Tool, "BitBody"):
                data["squawkData"].append(
                    data["squawkData"].append(
                        self.squawk(
                            "CAMSanity",
                            translate(
                                "CAM_Sanity",
                                "Tool number {} is a legacy tool. Legacy tools not \
                    supported by Path-Sanity",
                            ).format(TC.ToolNumber),
                            squawkType="WARNING",
                        )
                    )
                )
                continue  # skip old-style tools
            tooldata = data.setdefault(str(TC.ToolNumber), {})
            bitshape = tooldata.setdefault("ShapeType", "")
            if bitshape not in ["", TC.Tool.ShapeType]:
                data["squawkData"].append(
                    self.squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Tool number {} used by multiple tools").format(
                            TC.ToolNumber
                        ),
                        squawkType="CAUTION",
                    )
                )
            tooldata["bitShape"] = TC.Tool.ShapeType
            tooldata["description"] = TC.Tool.Label
            tooldata["manufacturer"] = ""
            tooldata["url"] = ""
            tooldata["inspectionNotes"] = ""
            tooldata["diameter"] = str(TC.Tool.Diameter)
            tooldata["shape"] = TC.Tool.ShapeType

            tooldata["partNumber"] = ""

            if os.path.isfile(TC.Tool.ShapeType):
                imagedata = TC.Tool.Proxy.get_thumbnail()
            else:
                imagedata = None
                data["squawkData"].append(
                    self.squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Toolbit Shape for TC: {} not found").format(
                            TC.ToolNumber
                        ),
                        squawkType="WARNING",
                    )
                )
            tooldata["image"] = ""
            imagepath = os.path.join(self.filelocation, f"T{TC.ToolNumber}.png")
            tooldata["imagepath"] = imagepath
            Path.Log.debug(imagepath)
            if imagedata is not None:
                with open(imagepath, "wb") as fd:
                    fd.write(imagedata)
                    fd.close()

            tooldata["feedrate"] = str(TC.HorizFeed)
            if TC.HorizFeed.Value == 0.0:
                data["squawkData"].append(
                    self.squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Tool Controller '{}' has no feedrate").format(
                            TC.Label
                        ),
                        squawkType="WARNING",
                    )
                )

            tooldata["spindlespeed"] = str(TC.SpindleSpeed)
            if TC.SpindleSpeed == 0.0:
                data["squawkData"].append(
                    self.squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Tool Controller '{}' has no spindlespeed").format(
                            TC.Label
                        ),
                        squawkType="WARNING",
                    )
                )

            used = False
            for op in obj.Operations.Group:
                base_op = PathDressup.baseOp(op)
                if hasattr(base_op, "ToolController") and base_op.ToolController is TC:
                    used = True
                    tooldata.setdefault("ops", []).append(
                        {
                            "Operation": base_op.Label,
                            "ToolController": TC.Label,
                            "Feed": str(TC.HorizFeed),
                            "Speed": str(TC.SpindleSpeed),
                        }
                    )

            if used is False:
                tooldata.setdefault("ops", [])
                data["squawkData"].append(
                    self.squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Tool Controller '{}' is not used").format(
                            TC.Label
                        ),
                        squawkType="WARNING",
                    )
                )

        return data

    def serialize(self, obj):
        """A function to serialize non-serializable objects."""
        if isinstance(obj, type(Exception)):
            # Convert an exception to its string representation
            return str(obj)
        # You might need to handle more types depending on your needs
        return str(obj)  # Fallback to convert any other non-serializable types to string

    def get_output_report(self):
        Path.Log.debug("get_output_url")

        generator = ReportGenerator.ReportGenerator(self.data, embed_images=True)
        html = generator.generate_html()
        generator = None
        return html
