#!/usr/bin/python3

# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

__title__ = "FreeCAD File info utility"
__author__ = "Yorik van Havre"
__url__ = ["https://www.freecad.org"]
__doc__ = """
This utility prints information about a given FreeCAD file (*.FCStd)
on screen, including document properties, number of included objects,
object sizes and properties and values. Its main use is to compare
two files and be able to see the differences in a text-based form.

If no option is used, fcinfo prints the document properties and a list
of properties of each object found in the given file.

Usage:

    fcinfo [options] myfile.FCStd

Options:

    -h, --help:      Prints this help text
    -s, --short:     Do not print object properties. Only one line
                     per object is printed, including its size and SHA1.
                     This is sufficient to see that an object has
                     changed, but not what exactly has changed.
    -vs --veryshort: Only prints the document info, not objects info.
                     This is sufficient to see if a file has changed, as
                     its SHA1 code and timestamp will show it. But won't
                     show details of what has changed.
    -p  --partinfo:  Print size and hash of internal BREP part files
    -g  --gui:       Adds visual properties too (if not using -s or -vs)

Git usage:

    This script can be used as a textconv tool for git diff by
    configuring your git folder as follows:

     1) add to .gitattributes (or ~/.gitattributes for user-wide):

         *.fcstd diff=fcinfo

     2) add to .git/config (or ~/.gitconfig for user-wide):

         [diff "fcinfo"]
             textconv = /path/to/fcinfo

    With this, when committing a .FCStd file with Git,
    'git diff' will show you the difference between the two
    texts obtained by fcinfo
"""


import sys
import zipfile
import xml.sax
import os
import hashlib
import re


class FreeCADFileHandler(xml.sax.ContentHandler):
    def __init__(self, zfile, short=0):  # short: 0=normal, 1=short, 2=veryshort

        xml.sax.ContentHandler.__init__(self)
        self.zfile = zfile
        self.obj = None
        self.prop = None
        self.count = "0"
        self.contents = {}
        self.short = short

    def startElement(self, tag, attributes):

        if tag == "Document":
            self.obj = tag
            self.contents = {}
            self.contents["ProgramVersion"] = attributes["ProgramVersion"]
            self.contents["FileVersion"] = attributes["FileVersion"]

        elif tag == "Object":
            if "name" in attributes:
                name = self.clean(attributes["name"])
                self.obj = name
                if "type" in attributes:
                    self.contents[name] = attributes["type"]

        elif tag == "ViewProvider":
            if "name" in attributes:
                self.obj = self.clean(attributes["name"])

        elif tag == "Part":
            if self.obj and partinfo:
                file = self.clean(attributes["file"])
                r = self.zfile.read(attributes["file"])
                s = r.__sizeof__()
                if s < 1024:
                    s = f"{s:g}B"
                elif s > 1048576:
                    s = f"{s / 1048576:.3g}M"
                else:
                    s = f"{s / 1024:.3g}K"
                d = str(hashlib.sha1(r).hexdigest()[:12])
                self.contents[self.obj] += f" ({file},{s},{d})"

        elif tag == "Property":
            self.prop = None
            # skip "internal" properties, useless for a diff
            if attributes["name"] not in [
                "Symbol",
                "AttacherType",
                "MapMode",
                "MapPathParameter",
                "MapReversed",
                "AttachmentOffset",
                "SelectionStyle",
                "TightGrid",
                "GridSize",
                "GridSnap",
                "GridStyle",
                "Lighting",
                "Deviation",
                "AngularDeflection",
                "BoundingBox",
                "Selectable",
                "ShowGrid",
            ]:
                self.prop = attributes["name"]

        elif tag in ["String", "Uuid", "Float", "Integer", "Bool", "Link"]:
            if self.prop and ("value" in attributes):
                if self.obj == "Document":
                    self.contents[self.prop] = attributes["value"]
                elif self.short == 0:
                    if tag == "Float":
                        val = float(attributes["value"])
                        self.contents[self.obj + "00000000::" + self.prop] = f"{val:g}"
                    else:
                        self.contents[self.obj + "00000000::" + self.prop] = attributes["value"]

        elif tag in ["PropertyVector"]:
            if self.prop and self.obj and (self.short == 0):
                vx = float(attributes["valueX"])
                vy = float(attributes["valueY"])
                vz = float(attributes["valueZ"])
                val = f"({vx:g},{vy:g},{vz:g})"
                self.contents[self.obj + "00000000::" + self.prop] = val

        elif tag in ["PropertyPlacement"]:
            if self.prop and self.obj and (self.short == 0):
                px = float(attributes["Px"])
                py = float(attributes["Py"])
                pz = float(attributes["Pz"])

                q0 = float(attributes["Q0"])
                q1 = float(attributes["Q1"])
                q2 = float(attributes["Q2"])
                q3 = float(attributes["Q3"])

                val = f"({px:g},{py:g},{pz:g})"
                val += f"({q0:.3g},{q1:.3g},{q2:.3g},{q3:.3g})"
                self.contents[self.obj + "00000000::" + self.prop] = val

        elif tag in ["PropertyColor"]:
            if self.prop and self.obj and (self.short == 0):
                c = int(attributes["value"])
                r = (c >> 24) & 0xFF
                g = (c >> 16) & 0xFF
                b = (c >> 8) & 0xFF
                val = f"({r},{g},{b})"
                self.contents[self.obj + "00000000::" + self.prop] = val

        elif tag == "Objects":
            self.count = attributes["Count"]
            self.obj = None

            # Print all the contents of the document properties
            items = self.contents.items()
            items = sorted(items)
            for key, value in items:
                key = self.clean(key)
                value = self.clean(value)
                print(f"   {key} : {value}")
            print(f"   Objects: ({self.count})")
            self.contents = {}

    def endElement(self, tag):

        if (tag == "Document") and (self.short != 2):
            items = self.contents.items()
            items = sorted(items)
            for key, value in items:
                key = self.clean(key)
                if "00000000::" in key:
                    key = "   " + key.split("00000000::")[1]
                value = self.clean(value)
                if value:
                    print(f"        {key} : {value}")

    def clean(self, value):

        value = value.strip()
        return value


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit()

    if ("-h" in sys.argv[1:]) or ("--help" in sys.argv[1:]):
        print(__doc__)
        sys.exit()

    ext = sys.argv[-1].rsplit(".")[-1].lower()
    if not ext.startswith("fcstd") and not ext.startswith("fcbak"):
        print(__doc__)
        sys.exit()

    if ("-vs" in sys.argv[1:]) or ("--veryshort" in sys.argv[1:]):
        short = 2
    elif ("-s" in sys.argv[1:]) or ("--short" in sys.argv[1:]):
        short = 1
    else:
        short = 0

    if ("-p" in sys.argv[1:]) or ("--partinfo" in sys.argv[1:]):
        partinfo = True
    else:
        partinfo = False

    if ("-g" in sys.argv[1:]) or ("--gui" in sys.argv[1:]):
        gui = True
    else:
        gui = False

    zfile = zipfile.ZipFile(sys.argv[-1])

    if not "Document.xml" in zfile.namelist():
        sys.exit(1)
    doc = zfile.read("Document.xml")
    if gui and "GuiDocument.xml" in zfile.namelist():
        guidoc = zfile.read("GuiDocument.xml")
        guidoc = re.sub(b"<\\?xml.*?-->", b" ", guidoc, flags=re.MULTILINE | re.DOTALL)
        # a valid xml doc can have only one root element. So we need to insert
        # all the contents of the GUiDocument <document> tag into the main one
        doc = re.sub(b"<\\/Document>", b"", doc, flags=re.MULTILINE | re.DOTALL)
        guidoc = re.sub(b"<Document.*?>", b" ", guidoc, flags=re.MULTILINE | re.DOTALL)
        doc += guidoc
    s = os.path.getsize(sys.argv[-1])
    if s < 1024:
        s = str(s) + "B"
    elif s > 1048576:
        s = str(s / 1048576) + "M"
    else:
        s = str(s / 1024) + "K"
    print("Document: " + sys.argv[-1] + " (" + s + ")")
    print("   SHA1: " + str(hashlib.sha1(open(sys.argv[-1], "rb").read()).hexdigest()))
    xml.sax.parseString(doc, FreeCADFileHandler(zfile, short))
