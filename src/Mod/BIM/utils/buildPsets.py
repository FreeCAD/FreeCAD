# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""This script retrieves a list of standard property sets from the IFC4 official
   documentation website and stores them into 1) a pset_definitions.csv  and 2)
   a qto_definitions.csv files in the directory ../Presets."""

import os
from urllib.request import urlopen
import xml.sax
from zipfile import ZipFile


URL = "https://ifc43-docs.standards.buildingsmart.org/IFC/RELEASE/IFC4x3/HTML/annex-a-psd.zip"

QTO_TYPES = {
    "Q_AREA": "IfcQuantityArea",
    "Q_COUNT": "IfcQuantityCount",
    "Q_LENGTH": "IfcQuantityLength",
    "Q_NUMBER": "IfcQuantityNumber",
    "Q_TIME": "IfcQuantityTime",
    "Q_VOLUME": "IfcQuantityVolume",
    "Q_WEIGHT": "IfcQuantityWeight",
}

class PropertyDefHandler(xml.sax.ContentHandler):
    "A XML handler to process pset definitions"

    # this creates a dictionary where each key is a Pset name,
    # and each value is a list of [property,type] lists

    def __init__(self, pset):
        super().__init__()
        self.line = pset.strip(".xml") + ";"
        self.currentprop = None
        self.currenttype = None
        self.charbuffer = []
        self.writing = False
        self.prop = False
        self.qtotype = False

    # Call when raw text is read (the property name)

    def characters(self, data):
        if self.writing:
            self.charbuffer.append(data)

    # Call when an element starts

    def startElement(self, tag, attributes):
        if tag in ["PropertyDef", "QtoDef"]:
            self.prop = True
        elif tag == "Name":
            self.writing = True
        elif tag == "DataType":
            self.currenttype = attributes["type"]
        elif tag == "QtoType":
            self.qtotype = True
            self.writing = True

    # Call when an elements ends

    def endElement(self, tag):
        if tag in ["Name", "QtoType"]:
            if self.prop:
                self.currentprop = "".join(self.charbuffer)
            elif self.qtotype:
                self.currenttype = "".join(self.charbuffer)
            self.writing = False
            self.prop = False
            self.qtotype = False
            self.charbuffer = []
        elif tag in ["PropertyDef", "QtoDef"]:
            if self.currentprop and self.currenttype:
                if self.currenttype in QTO_TYPES:
                    self.currenttype = QTO_TYPES[self.currenttype]
                self.line += self.currentprop + ";" + self.currenttype + ";"
            self.currentprop = None
            self.currenttype = None



# MAIN


print("Getting psets xml definitions…")

with open("psd.zip","wb") as f:
    u = urlopen(URL)
    p = u.read()
    f.write(p)

print("Reading xml definitions…")

psets = []
qtos = []

with ZipFile("psd.zip", 'r') as z:
    for entry in z.namelist():
        print("Parsing",entry)
        xml_data = z.read(entry).decode(encoding="utf-8")
        handler = PropertyDefHandler(entry)
        xml.sax.parseString(xml_data, handler)
        if entry.startswith("Pset"):
            psets.append(handler.line)
        else:
            qtos.append(handler.line)

print("Saving files…")

with open("../Presets/pset_definitions.csv", "w") as f:
    for l in psets:
        f.write(l.strip(";") + "\n")

with open("../Presets/qto_definitions.csv", "w") as f:
    for l in qtos:
        f.write(l.strip(";") + "\n")

os.remove("psd.zip")
