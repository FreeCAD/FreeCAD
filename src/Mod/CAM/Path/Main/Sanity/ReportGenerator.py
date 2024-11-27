# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
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

from string import Template
import FreeCAD
import Path.Log
import base64
import os

from Path.Main.Sanity.HTMLTemplate import (
    html_template,
    base_template,
    squawk_template,
    tool_template,
    op_run_template,
    op_tool_template,
    tool_item_template,
)

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ReportGenerator:
    def __init__(self, data, embed_images=False):
        self.embed_images = embed_images
        self.squawks = ""
        self.tools = ""
        self.run_summary_ops = ""
        self.formatted_data = {}
        self.translated_labels = {
            "dateLabel": translate("CAM_Sanity", "Date"),
            "datumLabel": translate("CAM_Sanity", "Part Datum"),
            "descriptionLabel": translate("CAM_Sanity", "Description"),
            "diameterLabel": translate("CAM_Sanity", "Tool Diameter"),
            "feedLabel": translate("CAM_Sanity", "Feed Rate"),
            "fileSizeLabel": translate("CAM_Sanity", "File Size (kB)"),
            "fixturesLabel": translate("CAM_Sanity", "Fixtures"),
            "flagsLabel": translate("CAM_Sanity", "Post Processor Flags"),
            "gcodeFileLabel": translate("CAM_Sanity", "G-code File"),
            "headingLabel": translate("CAM_Sanity", "Setup Report for CAM Job"),
            "inspectionNotesLabel": translate("CAM_Sanity", "Inspection Notes"),
            "lastpostLabel": translate("CAM_Sanity", "Last Post Process Date"),
            "lineCountLabel": translate("CAM_Sanity", "Line Count"),
            "machineLabel": translate("CAM_Sanity", "Machine"),
            "manufLabel": translate("CAM_Sanity", "Manufacturer"),
            "materialLabel": translate("CAM_Sanity", "Material"),
            "noteLabel": translate("CAM_Sanity", "Note"),
            "offsetsLabel": translate("CAM_Sanity", "Work Offsets"),
            "opLabel": translate("CAM_Sanity", "Operation"),
            "operatorLabel": translate("CAM_Sanity", "Operator"),
            "orderByLabel": translate("CAM_Sanity", "Order By"),
            "outputLabel": translate("CAM_Sanity", "Output (G-code)"),
            "partInformationLabel": translate("CAM_Sanity", "Part Information"),
            "partNumberLabel": translate("CAM_Sanity", "Part Number"),
            "postLabel": translate("CAM_Sanity", "Postprocessor"),
            "programmerLabel": translate("CAM_Sanity", "Programmer"),
            "roughStockLabel": translate("CAM_Sanity", "Rough Stock"),
            "runSummaryLabel": translate("CAM_Sanity", "Run Summary"),
            "shapeLabel": translate("CAM_Sanity", "Tool Shape"),
            "speedLabel": translate("CAM_Sanity", "Spindle Speed"),
            "squawksLabel": translate("CAM_Sanity", "Squawks"),
            "stopsLabel": translate("CAM_Sanity", "Stops"),
            "sSpeedCarbideLabel": translate("CAM_Sanity", "Surface Speed Carbide"),
            "sSpeedHSSLabel": translate("CAM_Sanity", "Surface Speed HSS"),
            "tableOfContentsLabel": translate("CAM_Sanity", "Table of Contents"),
            "tcLabel": translate("CAM_Sanity", "Tool Controller"),
            "toolDataLabel": translate("CAM_Sanity", "Tool Data"),
            "toolNumberLabel": translate("CAM_Sanity", "Tool Number"),
            "urlLabel": translate("CAM_Sanity", "URL"),
            "xDimLabel": translate("CAM_Sanity", "X Size"),
            "yDimLabel": translate("CAM_Sanity", "Y Size"),
            "zDimLabel": translate("CAM_Sanity", "Z Size"),
            "jobMinZLabel": translate("CAM_Sanity", "Minimum Z"),
            "jobMaxZLabel": translate("CAM_Sanity", "Maximum Z"),
            "coolantLabel": translate("CAM_Sanity", "Coolant Mode"),
            "cycleTimeLabel": translate("CAM_Sanity", "Cycle Time"),
            "PartLabel": translate("CAM_Sanity", "Part"),
            "SequenceLabel": translate("CAM_Sanity", "Sequence"),
            "JobTypeLabel": translate("CAM_Sanity", "Job Type"),
            "CADLabel": translate("CAM_Sanity", "CAD File"),
            "LastSaveLabel": translate("CAM_Sanity", "Last Save"),
            "CustomerLabel": translate("CAM_Sanity", "Customer"),
        }

        # format the data for all blocks except tool
        for block in [
            "baseData",
            "designData",
            "runData",
            "outputData",
            "fixtureData",
            "stockData",
        ]:
            for key, val in data[block].items():
                Path.Log.debug(f"key: {key} val: {val}")
                if key == "squawkData":
                    self._format_squawks(val)
                elif key == "bases":
                    self._format_bases(val)
                elif key == "operations":
                    self._format_run_summary_ops(val)
                elif key in ["baseimage", "imagepath", "datumImage", "stockImage"]:
                    Path.Log.debug(f"key: {key} val: {val}")
                    if self.embed_images:
                        Path.Log.debug("Embedding images")
                        encoded_image, tag = self.file_to_base64_with_tag(val)
                    else:
                        Path.Log.debug("Not Embedding images")
                        tag = f"<img src={val} name='Image' alt={key} />"
                    self.formatted_data[key] = tag
                else:
                    self.formatted_data[key] = val

        # format the data for the tool block
        for key, val in data["toolData"].items():
            if key == "squawkData":
                self._format_squawks(val)
            # else:
            #     self._format_tool(key, val)

            else:
                toolNumber = key
                toolAttributes = val
                if "imagepath" in toolAttributes and toolAttributes["imagepath"] != "":
                    if self.embed_images:
                        encoded_image, tag = self.file_to_base64_with_tag(
                            toolAttributes["imagepath"]
                        )
                    else:
                        tag = f"<img src={toolAttributes['imagepath']} name='Image' alt={key} />"
                    toolAttributes["imagepath"] = tag

                self._format_tool(key, val)

        self.formatted_data["squawks"] = self.squawks
        self.formatted_data["run_summary_ops"] = self.run_summary_ops
        self.formatted_data["tool_data"] = self.tools
        self.formatted_data["tool_list"] = self._format_tool_list(data["toolData"])

        # Path.Log.debug(self.formatted_data)

    def _format_tool_list(self, tool_data):

        tool_list = ""
        for key, val in tool_data.items():
            if key == "squawkData":
                continue
            else:
                val["toolNumber"] = key
                tool_list += tool_item_template.substitute(val)
        return tool_list

    def _format_run_summary_ops(self, op_data):
        for op in op_data:
            self.run_summary_ops += op_run_template.substitute(op)

    def _format_tool(self, tool_number, tool_data):
        td = {}
        for key, val in tool_data.items():
            if key == "squawkData":
                self._format_squawks(val)
            if key == "ops":
                opslist = ""
                for op in val:
                    op.update(self.translated_labels)
                    opslist += op_tool_template.substitute(op)
                td[key] = opslist
            else:
                td[key] = val

        td.update(self.translated_labels)
        Path.Log.debug(f"Tool data: {td}")

        self.tools += tool_template.substitute(td)

    def _format_bases(self, base_data):
        bases = ""
        for base in base_data:
            bases += base_template.substitute(base)
        self.formatted_data["bases"] = bases

    def _format_squawks(self, squawk_data):
        for squawk in squawk_data:
            if self.embed_images:
                data, tag = self.file_to_base64_with_tag(squawk["squawkIcon"])
            else:
                tag = f"<img src={squawk['squawkIcon']} name='Image' alt='TIP' />"

            squawk["squawkIcon"] = tag
            self.squawks += squawk_template.substitute(squawk)

    def generate_html(self):
        self.formatted_data.update(self.translated_labels)
        html_content = html_template.substitute(self.formatted_data)
        return html_content

    def encode_gcode_to_base64(filepath):
        with open(filepath, "rb") as file:
            encoded_string = base64.b64encode(file.read()).decode("utf-8")
        return encoded_string

    def file_to_base64_with_tag(self, file_path):
        # Determine MIME type based on the file extension
        mime_types = {
            ".jpg": "image/jpeg",
            ".jpeg": "image/jpeg",
            ".png": "image/png",
            ".svg": "image/svg+xml",
            ".gcode": "application/octet-stream",  # MIME type for G-code files
        }
        extension = os.path.splitext(file_path)[1]
        mime_type = mime_types.get(
            extension, "application/octet-stream"
        )  # Default to binary data type if unknown

        if not os.path.exists(file_path):
            Path.Log.error(f"File not found: {file_path}")
            return "", ""

        try:
            # Encode file to base64
            with open(file_path, "rb") as file:
                encoded_string = base64.b64encode(file.read()).decode()

            # Generate HTML tag based on file type
            if extension in [".jpg", ".jpeg", ".png", ".svg"]:
                html_tag = f'<img src="data:{mime_type};base64,{encoded_string}">'
            elif extension in [".gcode", ".nc", ".tap", ".cnc"]:
                html_tag = f'<a href="data:{mime_type};base64,{encoded_string}" download="{os.path.basename(file_path)}">Download G-code File</a>'
            else:
                html_tag = f'<a href="data:{mime_type};base64,{encoded_string}" download="{os.path.basename(file_path)}">Download File</a>'

            return encoded_string, html_tag
        except FileNotFoundError:
            Path.Log.error(f"File not found: {file_path}")
            return "", ""
