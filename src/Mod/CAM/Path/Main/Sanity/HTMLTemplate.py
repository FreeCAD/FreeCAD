# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
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
        /* Reset margins and padding */
        div, dl, dt, dd, ul, ol, li, h1, h2, h3, #toctitle, .sidebarblock>.content>.title, h4, h5, h6, pre, form, p, blockquote, th, td {
            margin: 0;
            padding: 0;
        }

        /* Responsive image handling */
        img, object, embed {
            max-width: 100%;
            height: auto;
        }

        /* Base styling */
        body {
            background: #fff;
            color: rgba(0,0,0,.8);
            padding: 0;
            margin: 0;
            font-family: "Noto Serif", "DejaVu Serif", serif;
            line-height: 1;
            position: relative;
            cursor: auto;
            word-wrap: anywhere;
            -moz-osx-font-smoothing: grayscale;
            -webkit-font-smoothing: antialiased;
            max-width: 62.5em;
            margin-left: auto;
            margin-right: auto;
            padding: 20px;
        }

        /* Container styling */
        .center-container {
            max-width: 62.5em;
            margin-left: auto;
            margin-right: auto;
            padding: 20px;
        }

        /* Headings */
        h1, h2, h3, #toctitle, h4, h5, h6 {
            font-family: "Open Sans", "DejaVu Sans", sans-serif;
            font-weight: 300;
            font-style: normal;
            color: #ba3925;
            text-rendering: optimizeLegibility;
            margin-top: 1em;
            margin-bottom: .5em;
            line-height: 1.0125em;
        }

        h1 {
            font-size: 2.125em;
        }

        h2 {
            font-size: 1.6875em;
        }

        h3 {
            font-size: 1.375em;
        }

        h4, h5 {
            font-size: 1.125em;
        }

        h6 {
            font-size: 1em;
        }

        #toctitle {
            font-size: 1.2em;
        }

        @media screen and (min-width: 768px) {
            #toctitle {
                font-size: 1.375em;
            }
        }

        /* Media query for larger screens */
        @media screen and (min-width: 768px) {
            h1, h2, h3, h4, h5, h6 {
                line-height: 1.2;
            }

            h1 {
                font-size: 2.75em;
            }

            h2 {
                font-size: 2.3125em;
            }

            h3 {
                font-size: 1.6875em;
            }

            h4 {
                font-size: 1.4375em;
            }
        }

        /* Links */
        a {
            color: #2156a5;
            text-decoration: underline;
            line-height: inherit;
        }

        /* TOC styling */
        #toc {
            border-bottom: 1px solid #e7e7e9;
            padding-bottom: .5em;
        }

        #header>h1:first-child+#toc {
            margin-top: 8px;
            border-top: 1px solid #dddddf;
        }

        #toc>ul {
            margin-left: .125em;
        }

        #toc ul {
            font-family: "Open Sans", "DejaVu Sans", sans-serif;
            list-style-type: none;
        }

        #toc li {
            line-height: 1.3334;
            margin-top: .3334em;
        }

        #toc a {
            text-decoration: none;
        }

        a:hover, a:focus {
            color: #1d4b8f;
        }

        /* Lists */
        ul, ol, dl {
            line-height: 1.6;
            margin-bottom: 1.25em;
            list-style-position: outside;
            font-family: inherit;
            margin-left: 1.5em;
        }

        ul {
            list-style: disc;
        }

        ul li ul, ul li ol {
            margin-left: 1.25em;
            margin-bottom: 0;
        }

        /* TOC section levels */
        #toc ul.sectlevel0>li>a {
            font-style: italic;
        }

        #toc ul.sectlevel0 ul.sectlevel1 {
            margin: .5em 0;
        }

        @media screen and (min-width: 768px) {
            #toc.toc2 ul ul {
                margin-left: 0;
                padding-left: 1em;
            }

            #toc.toc2 ul.sectlevel0 ul.sectlevel1 {
                padding-left: 0;
                margin-top: .5em;
                margin-bottom: .5em;
            }
        }

        @media screen and (min-width: 1280px) {
            #toc.toc2 ul ul {
                padding-left: 1.25em;
            }
        }

        /* Responsive image handling at different screen sizes */
        @media screen and (max-width: 768px) {
            td.image-container {
                max-width: 100%;
                display: block;
                margin: 1em auto;
            }

            table td[rowspan] {
                display: table-cell;
            }

            img {
                max-width: 100%;
                height: auto;
            }
        }

        /* Tables - more detailed styling */
        table {
            background: #fff;
            margin-bottom: 1.25em;
            border: 1px solid #dedede;
            word-wrap: normal;
            border-collapse: collapse;
            border-spacing: 0;
            width: 100%;
        }

        table thead, table tfoot {
            background: #f7f8f7;
        }

        table thead tr th, table thead tr td, table tfoot tr th, table tfoot tr td {
            padding: .5em .625em .625em;
            font-size: inherit;
            color: rgba(0,0,0,.8);
            text-align: left;
        }

        table tr th, table tr td {
            padding: .5625em .625em;
            font-size: inherit;
            color: rgba(0,0,0,.8);
            line-height: 1.6;
            border: 1px solid #dedede;
        }

        /* Anchor links styling */
        #content h1>a.anchor,h2>a.anchor,h3>a.anchor,#toctitle>a.anchor,.sidebarblock>.content>.title>a.anchor,h4>a.anchor,h5>a.anchor,h6>a.anchor{
            position:absolute;
            z-index:1001;
            width:1.5ex;
            margin-left:-1.5ex;
            display:block;
            text-decoration:none!important;
            visibility:hidden;
            text-align:center;
            font-weight:400
        }

        table tr.even, table tr.alt {
            background: #f8f8f7;
        }

        /* Image container styling */
        td.image-container {
            vertical-align: middle;
            text-align: center;
            width: 40%;
            max-width: 300px;
        }

        td.image-container img {
            max-width: 100%;
            height: auto;
            object-fit: contain;
        }

        /* Text styling */
        p {
            line-height: 1.6;
            margin-bottom: 1.25em;
            text-rendering: optimizeLegibility;
        }

        strong, b {
            font-weight: bold;
            line-height: inherit;
        }

        em, i {
            font-style: italic;
            line-height: inherit;
        }

        /* Top navigation links - base style */
        .top-link {
            display: inline-block;
            float: right;
            font-size: 0.8em;
            font-weight: normal;
            text-transform: uppercase;
            color: #2156a5;
            text-decoration: none;
            position: relative;
        }

        /* Specific positioning for h2 headings */
        .heading-container h2 + .top-link {
            margin-top: 4.75em;
            bottom: 0.3em;
        }

        /* Specific positioning for h3 headings */
        .heading-container h3 + .top-link {
            margin-top: 3.5em;
            bottom: 0.3em;
        }

        /* Fallbacks in case the adjacent sibling selector doesn't work as expected */
        .heading-container:has(h2) .top-link {
            margin-top: 4.75em;
        }

        .heading-container:has(h3) .top-link {
            margin-top: 3.5em;
        }

        /* Clearfix for headings with top links */
        .heading-container {
            overflow: hidden;
            width: 100%;
            position: relative;
        }

        .heading-container h2,
        .heading-container h3 {
            float: left;
            margin-bottom: 0;
        }

        /* Hide top links when printing */
        @media print {
            .top-link {
                display: none;
            }
        }
    </style>
</head>
<body>
    <div id="header" class="center-container">
    <h1>${headingLabel}: ${JobLabel}</h1>
    <div id="toc" class="toc">
        <div id="toctitle">${tableOfContentsLabel}</div>
        <ul class="sectlevel1">
            <li><a href="#_part_information">${partInformationLabel}</a></li>
            <li><a href="#_run_summary">${runSummaryLabel}</a></li>
            <li><a href="#_rough_stock">${roughStockLabel}</a></li>
            <li><a href="#_tool_data">${toolDataLabel}</a>
                <ul class="sectlevel2">
                    ${tool_list}
                </ul>
            </li>
            <li><a href="#_output">${outputLabel}</a></li>
            <li><a href="#_fixtures_and_workholding">${fixturesLabel}</a></li>
            <li><a href="#_squawks">${squawksLabel}</a></li>
        </ul>
    </div>


<div class="heading-container">
    <h2 id="_part_information"><a name="_part_information"></a>${partInformationLabel}</h2><a href="#header" class="top-link">Top</a>
</div>
<table>
    <colgroup>
        <col width="20%"/>
        <col width="50%"/>
        <col width="30%"/>
    </colgroup>
    <tr>
        <td>
            <strong>${PartLabel}</strong>
        </td>
        <td>
            <table>
                <colgroup>
                    <col width="50%"/>
                    <col width="50%"/>
                </colgroup>
                ${bases}
            </table>
        </td>
        <td rowspan="7" class="image-container">
            ${baseimage}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${SequenceLabel}</strong>
        </td>
        <td>
            ${Sequence}
        </td>
    </tr>

    <tr>
        <td>
            <strong>${JobTypeLabel}</strong>
        </td>
        <td>
            ${JobType}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${CADLabel}</strong>
        </td>
        <td>
            ${FileName}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${LastSaveLabel}</strong>
        </td>
        <td>
            ${LastModifiedDate}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${CustomerLabel}</strong>
        </td>
        <td>
            ${Customer}
        </td>
    </tr>
</table>

<div class="heading-container">
    <h2 id="_run_summary"><a name="_run_summary"></a>${runSummaryLabel}</h2><a href="#header" class="top-link">Top</a>
</div>
<table>
    <colgroup>
        <col width="20%"/>
        <col width="20%"/>
        <col width="20%"/>
        <col width="20%"/>
        <col width="20%"/>
    </colgroup>
    <thead>
        <tr>
            <th><strong>${opLabel}</strong></th>
            <th><strong>${jobMinZLabel}</strong></th>
            <th><strong>${jobMaxZLabel}</strong></th>
            <th><strong>${coolantLabel}</strong></th>
            <th><strong>${cycleTimeLabel}</strong></th>
        </tr>
    </thead>
    <tbody>
        ${run_summary_ops}
    </tbody>
</table>

<div class="heading-container">
    <h2 id="_rough_stock"><a name="_rough_stock"></a>${roughStockLabel}</h2><a href="#header" class="top-link">Top</a>
</div>

<table>
    <colgroup>
        <col width="30%"/>
        <col width="30%"/>
        <col width="40%"/>
    </colgroup>
    <tbody>
        <tr>
            <td><strong>${materialLabel}</strong></td>
            <td>${material}</td>
            <td rowspan="7" class="image-container">
                ${stockImage}
            </td>
        </tr>
        <tr>
            <td><strong>${sSpeedHSSLabel}</strong></td>
            <td>${surfaceSpeedHSS}</td>
        </tr>
        <tr>
            <td><strong>${sSpeedCarbideLabel}</strong></td>
        <td>${surfaceSpeedCarbide}</td>
    </tr>
    <tr>
        <td><strong>${xDimLabel}</strong></td>
        <td>${xLen}</td>
    </tr>
    <tr>
        <td><strong>${yDimLabel}</strong></td>
        <td>${yLen}</td>
    </tr>
    <tr>
        <td><strong>${zDimLabel}</strong></td>
        <td>${zLen}</td>
    </tr>
</table>


<div class="heading-container">
    <h2 id="_tool_data"><a name="_tool_data"></a>${toolDataLabel}</h2><a href="#header" class="top-link">Top</a>
</div>
${tool_data}


<div class="heading-container">
    <h2 id="_output"><a name="_output"></a>${outputLabel}</h2><a href="#header" class="top-link">Top</a>
</div>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="525"/>
        <col width="525"/>
    </colgroup>
    <tr>
        <td>
            <strong>${gcodeFileLabel}</strong>
        </td>
        <td>
            ${lastgcodefile}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${lastpostLabel}</strong>
        </td>
        <td>
            ${lastpostprocess}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${stopsLabel}</strong>
        </td>
        <td>
            ${optionalstops}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${programmerLabel}</strong>
        </td>
        <td>
            ${programmer}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${machineLabel}</strong>
        </td>
        <td>
            ${machine}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${postLabel}</strong>
        </td>
        <td>
            ${postprocessor}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${flagsLabel}</strong>
        </td>
        <td>
            ${postprocessorFlags}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${fileSizeLabel}</strong>
        </td>
        <td>
            ${filesize}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${lineCountLabel}</strong>
        </td>
        <td>
            ${linecount}
        </td>
    </tr>
</table>

<div class="heading-container">
    <h2 id="_fixtures_and_workholding"><a name="_fixtures_and_workholding"></a>${fixturesLabel}</h2><a href="#header" class="top-link">Top</a>
</div>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background: #ffffff;">
    <colgroup>
        <col width="525"/>
        <col width="525"/>
    </colgroup>
    <tr>
        <td>
            <strong>${offsetsLabel}</strong>
        </td>
        <td>
            ${fixtures}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${orderByLabel}</strong>
        </td>
        <td>
            ${orderBy}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${datumLabel}</strong>
        </td>
        <td class="image-container">
            ${datumImage}
        </td>
    </tr>
</table>

<div class="heading-container">
    <h2 id="_squawks"><a name="_squawks"></a>${squawksLabel}</h2><a href="#header" class="top-link">Top</a>
</div>
<table cellpadding="2" cellspacing="2" bgcolor="#ffffff" style="background-color: #ffffff;">
    <colgroup>
        <col width="100"/>
        <col width="250"/>
        <col width="250"/>
        <col width="550"/>
    </colgroup>
    <tr>
        <td>
            <strong>${noteLabel}</strong>
        </td>
        <td>
            <strong>${operatorLabel}</strong>
        </td>
        <td>
            <strong>${dateLabel}</strong>
        </td>
        <td>
            <strong>${noteLabel}</strong>
        </td>
        ${squawks}
    </tr>
</table>

<p style="line-height: 100%; margin-bottom: 0cm"><br/>
</p>
</div>
</body>
</html>
"""
)

base_template = Template(
    """
<tr>
    <td>
        ${key}
    </td>
    <td>
        ${val}
    </td>
</tr>
        """
)

squawk_template = Template(
    """
<tr>
    <td>
        ${squawkIcon}
    </td>
    <td>
        ${Operator}
    </td>
    <td>
        ${Date}
    </td>
    <td colspan="3">
        ${Note}
    </td>
</tr>
        """
)

tool_template = Template(
    """
<div class="heading-container">
    <h3 id="_tool_data_T${toolNumber}">Tool Number: T${toolNumber}</h3><a href="#header" class="top-link">Top</a>
</div>
<table>
    <colgroup>
        <col width="30%"/>
        <col width="30%"/>
        <col width="40%"/>
    </colgroup>
    <tr>
        <td>
            <strong>${descriptionLabel}</strong>
        </td>
        <td>
            ${description}
        </td>
        <td class="image-container">
            ${imagepath}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${manufLabel}</strong>
        </td>
        <td colspan="2">
            ${manufacturer}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${partNumberLabel}</strong>
        </td>
        <td colspan="2">
            ${partNumber}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${urlLabel}</strong>
        </td>
        <td colspan="2">
            ${url}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${shapeLabel}</strong>
        </td>
        <td colspan="2">
            ${shape}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${inspectionNotesLabel}</strong>
        </td>
        <td colspan="2">
            ${inspectionNotes}
        </td>
    </tr>
    <tr>
        <td>
            <strong>${diameterLabel}</strong>
        </td>
        <td colspan="2">
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
        <td>
            <strong>${opLabel}</strong>
        </td>
        <td>
            <strong>${tcLabel}</strong>
        </td>
        <td>
            <strong>${feedLabel}</strong>
        </td>
        <td>
            <strong>${speedLabel}</strong>
        </td>
    </tr>
    <tr>
        <td>
            ${Operation}
        </td>
        <td>
            ${ToolController}
        </td>
        <td>
            ${Feed}
        </td>
        <td>
            ${Speed}
        </td>
    </tr>
</table>
        """
)

op_run_template = Template(
    """
<tr>
    <td>
        ${opName}
    </td>
    <td>
        ${minZ}
    </td>
    <td>
        ${maxZ}
    </td>
    <td>
        ${coolantMode}
    </td>
    <td>
        ${cycleTime}
    </td>
</tr>
        """
)

tool_item_template = Template(
    """
<li><a href="#_tool_data_T${toolNumber}">Tool Number: T${toolNumber}</a></li>
    """
)
