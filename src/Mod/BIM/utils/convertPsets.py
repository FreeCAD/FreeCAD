# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

"""This script converts a xml file containing pset definitions to a csv file.
Python3 only!! (py2 csv doesn't support utf8"""

import xml.sax, os


class PropertyDefHandler(xml.sax.ContentHandler):
    "A XML handler to process pset definitions"

    # this creates a dictionary where each key is a Pset name,
    # and each value is a list of [property,type] lists

    def __init__(self):
        super().__init__()
        self.psets = {}
        self.currentpset = None
        self.currentprop = None
        self.currenttype = None
        self.currentlist = []
        self.charbuffer = []
        self.writing = False

    # Call when raw text is read

    def characters(self, data):
        if self.writing:
            self.charbuffer.append(data)

    # Call when an element starts

    def startElement(self, tag, attributes):
        if tag == "Name":
            self.writing = True
        if tag == "DataType":
            self.currenttype = attributes["type"]

    # Call when an elements ends

    def endElement(self, tag):
        if tag == "Name":
            if not self.currentpset:
                self.currentpset = "".join(self.charbuffer)
            else:
                if not self.currentprop:
                    self.currentprop = "".join(self.charbuffer)
            self.writing = False
            self.charbuffer = []
        elif tag == "PropertyDef":
            if self.currentprop and self.currenttype:
                self.currentlist.append([self.currentprop, self.currenttype])
            self.currentprop = None
            self.currenttype = None
        elif tag == "PropertySetDef":
            if self.currentpset and self.currentlist:
                self.psets[self.currentpset] = self.currentlist
            self.currentpset = None
            self.currentlist = []


defpath = "pset_definitions.xml"
outpath = "pset_definitions.csv"

if os.path.exists(defpath):
    handler = PropertyDefHandler()
    parser = xml.sax.make_parser()
    # parser.setFeature(xml.sax.handler.feature_namespaces, 0)
    parser.setContentHandler(handler)
    parser.parse(defpath)
    psets = handler.psets

    import csv

    with open(outpath, "w", encoding="utf-8") as csvfile:
        csvfile = csv.writer(csvfile, delimiter=";")
        for key, values in psets.items():
            r = [key]
            for value in values:
                r.extend(value)
            csvfile.writerow(r)
    print("successfully exported ", outpath)
