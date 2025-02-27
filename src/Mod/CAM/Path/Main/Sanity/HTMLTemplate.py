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

"""
This module contains the HTML template for the CAM Sanity report.
"""

html_template = Template(
    """
<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
    <title>Setup Report for FreeCAD Job: Path Special</title>
    <style type="text/css">
        body {
            background-color: #FFFFFF;
            color: #000000;
            font-family: "Open Sans, DejaVu Sans, sans-serif";
        }
        h2.western, .ToC {
            font-size: 20pt;
            color: #ba3925;
            margin-bottom: 0.5cm;
        }
        a.customLink {
            color: #2156a5;
            text-decoration: none;
            font-size: 12pt;
        }
        ul {
            padding-left: 0;
            list-style: none;
        }
        ul.subList {
            padding-left: 20px;
        }
        li.subItem {
            padding-top: 5px;
        }
    </style>
</head>
<body>
    <h1>${headingLabel}: ${JobLabel}</h1>
    <div id="toc">
        <h2 class="ToC">${tableOfContentsLabel}</h2>
        <ul>
            <li><a class="customLink" href="#_part_information">${partInformationLabel}</a></li>
            <li><a class="customLink" href="#_run_summary">${runSummaryLabel}</a></li>
            <li><a class="customLink" href="#_rough_stock">${roughStockLabel}</a></li>
            <li><a class="customLink" href="#_tool_data">${toolDataLabel}</a>
                <ul class="subList">
                    ${tool_list}
                </ul>
            </li>
            <li><a class="customLink" href="#_output">${outputLabel}</a></li>
            <li><a class="customLink" href="#_fixtures_and_workholding">${fixturesLabel}</a></li>
            <li><a class="customLink" href="#_squawks">${squawksLabel}</a></li>
        </ul>
    </div>


<h2 class="western"><a name="_part_information"></a>${partInformationLabel}</h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="200"/>
        <col width="525"/>
        <col width="250"/>
    </colgroup>
    <tr valign="top">
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            <strong>${PartLabel}</strong>
        </td>
        <td>
            <table style="background-color: #ffffff;">
                <colgroup>
                    <col width="175"/>
                    <col width="175"/>
                </colgroup>
                ${bases}
            </table>
        </td>
        <td rowspan="7" style="border: 1px solid #dedede; padding: 0.05cm;">
            ${baseimage}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            <strong>${SequenceLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            ${Sequence}
        </td>
    </tr>

    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            <strong>${JobTypeLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            ${JobType}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            <strong>${CADLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            ${FileName}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            <strong>${LastSaveLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            ${LastModifiedDate}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            <strong>${CustomerLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm;">
            ${Customer}
        </td>
    </tr>
</table>

<h2 class="western"><a name="_run_summary"></a>${runSummaryLabel}</h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="210"/>
        <col width="210"/>
        <col width="210"/>
        <col width="210"/>
        <col width="210"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${opLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${jobMinZLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${jobMaxZLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${coolantLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${cycleTimeLabel}</strong>
        </td>
    </tr>
    ${run_summary_ops}
</table>

<h2 class="western"><a name="_rough_stock"></a>${roughStockLabel}</h2>

<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="350"/>
        <col width="350"/>
        <col width="350"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm"><strong>${materialLabel}</strong></td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">${material}</td>
        <td rowspan="7" style="border: 1px solid #dedede; padding: 0.05cm">
            ${stockImage}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm"><strong>${sSpeedHSSLabel}</strong></td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">${surfaceSpeedHSS}</td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm"><strong>${sSpeedCarbideLabel}</strong></td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">${surfaceSpeedCarbide}</td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm"><strong>${xDimLabel}</strong></td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">${xLen}</td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm"><strong>${yDimLabel}</strong></td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">${yLen}</td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm"><strong>${zDimLabel}</strong></td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">${zLen}</td>
    </tr>
</table>


<h2 class="western"><a name="_tool_data"></a>${toolDataLabel}</h2>
${tool_data}


<h2 class="western"><a name="_output"></a>${outputLabel}</h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="525"/>
        <col width="525"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${gcodeFileLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${lastgcodefile}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${lastpostLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${lastpostprocess}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${stopsLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${optionalstops}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${programmerLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${programmer}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${machineLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${machine}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${postLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${postprocessor}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${flagsLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${postprocessorFlags}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${fileSizeLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${filesize}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${lineCountLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${linecount}
        </td>
    </tr>
</table>

<h2 class="western"><a name="_fixtures_and_workholding"></a>${fixturesLabel}</h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="525"/>
        <col width="525"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${offsetsLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${fixtures}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${orderByLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${orderBy}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${datumLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${datumImage}
        </td>
    </tr>
</table>

<h2 class="western"><a name="_squawks"></a>${squawksLabel}</h2>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background-color: #ffffff;">
    <colgroup>
        <col width="100"/>
        <col width="250"/>
        <col width="250"/>
        <col width="550"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${noteLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${operatorLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${dateLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${noteLabel}</strong>
        </td>
        ${squawks}
    </tr>
</table>

<p style="line-height: 100%; margin-bottom: 0cm"><br/>

</p>
</body>
</html>
"""
)

base_template = Template(
    """
<tr>
    <td style='border: 1px solid #dedede; padding: 0.05cm;'>
        %{key}
    </td>
    <td style='border: 1px solid #dedede; padding: 0.05cm;'>
        %{val}
    </td>
</tr>
        """
)

squawk_template = Template(
    """
<tr>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${squawkIcon}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${Operator}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${Date}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="3">
        ${Note}
    </td>
</tr>
        """
)

tool_template = Template(
    """
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="350"/>
        <col width="350"/>
        <col width="350"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${descriptionLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${description}
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${imagepath}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${manufLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="2">
            ${manufacturer}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${partNumberLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="2">
            ${partNumber}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${urlLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="2">
            ${url}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${shapeLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="2">
            ${shape}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${inspectionNotesLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="2">
            ${inspectionNotes}
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${diameterLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm" colspan="2">
            ${diameter}
        </td>
    </tr>
</table>
${ops}
        """
)

op_tool_template = Template(
    """
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="262"/>
        <col width="262"/>
        <col width="262"/>
        <col width="262"/>
    </colgroup>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${opLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${tcLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${feedLabel}</strong>
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            <strong>${speedLabel}</strong>
        </td>
    </tr>
    <tr>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${Operation}
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${ToolController}
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${Feed}
        </td>
        <td style="border: 1px solid #dedede; padding: 0.05cm">
            ${Speed}
        </td>
    </tr>
</table>
        """
)

op_run_template = Template(
    """
<tr>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${opName}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${minZ}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${maxZ}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${coolantMode}
    </td>
    <td style="border: 1px solid #dedede; padding: 0.05cm">
        ${cycleTime}
    </td>
</tr>
        """
)

tool_item_template = Template(
    """
<li class="subItem"><a class="customLink" href="#_tool_data_T${toolNumber}">T${toolNumber}-${description}</a></li>
    """
)
