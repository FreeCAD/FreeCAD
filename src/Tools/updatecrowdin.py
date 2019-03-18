#!/usr/bin/python

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Library General Public License (LGPL)   *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

from __future__ import print_function

'''
This utility offers several commands to interact with the FreeCAD project on crowdin.
For it to work, you need a .crowdin-freecad file in your user's folder, that contains
the API key that gives access to the crowdin FreeCAD project.

Usage:

    updatecrowdin.py command

Available commands:

    status:   prints a status of the translations
    update:   updates crowdin the current version of .ts files found in the source code
    build:    builds a new downloadable package on crowdin with all translated strings
    download: downloads the latest build

Example:

    ./updatecrowdin.py update

'''

# See crowdin API docs at https://crowdin.com/page/api



import sys,os,xml.sax,pycurl,StringIO


files = [ ["Arch.ts",              "/Mod/Arch/Resources/translations/Arch.ts"],
          ["Assembly.ts",          "/Mod/Assembly/Gui/Resources/translations/Assembly.ts"],
          ["draft.ts",             "/Mod/Draft/Resources/translations/Draft.ts"],
          ["Drawing.ts",           "/Mod/Drawing/Gui/Resources/translations/Drawing.ts"],
          ["Fem.ts",               "/Mod/Fem/Gui/Resources/translations/Fem.ts"],
          ["FreeCAD.ts",           "/Gui/Language/FreeCAD.ts"],
          ["Image.ts",             "/Mod/Image/Gui/Resources/translations/Image.ts"],
          ["Mesh.ts",              "/Mod/Mesh/Gui/Resources/translations/Mesh.ts"],
          ["MeshPart.ts",          "/Mod/MeshPart/Gui/Resources/translations/MeshPart.ts"],
          ["OpenSCAD.ts",          "/Mod/OpenSCAD/Resources/translations/OpenSCAD.ts"],
          ["Part.ts",              "/Mod/Part/Gui/Resources/translations/Part.ts"],
          ["PartDesign.ts",        "/Mod/PartDesign/Gui/Resources/translations/PartDesign.ts"],
          ["Plot.ts",              "/Mod/Plot/resources/translations/Plot.ts"],
          ["Points.ts",            "/Mod/Points/Gui/Resources/translations/Points.ts"],
          ["Raytracing.ts",        "/Mod/Raytracing/Gui/Resources/translations/Raytracing.ts"],
          ["ReverseEngineering.ts","/Mod/ReverseEngineering/Gui/Resources/translations/ReverseEngineering.ts"],
          ["Robot.ts",             "/Mod/Robot/Gui/Resources/translations/Robot.ts"],
          ["Ship.ts",              "/Mod/Ship/resources/translations/Ship.ts"],
          ["Sketcher.ts",          "/Mod/Sketcher/Gui/Resources/translations/Sketcher.ts"],
          ["StartPage.ts",         "/Mod/Start/Gui/Resources/translations/StartPage.ts"],
          ["Test.ts",              "/Mod/Test/Gui/Resources/translations/Test.ts"],
          ["Web.ts",               "/Mod/Web/Gui/Resources/translations/Web.ts"],
          ["Spreadsheet.ts",       "/Mod/Spreadsheet/Gui/Resources/translations/Spreadsheet.ts"],
          ["Path.ts",              "/Mod/Path/Gui/Resources/translations/Path.ts"],
          ["Tux.ts",               "/Mod/Tux/Resources/translations/Tux.ts"],
          ["TechDraw.ts",          "/Mod/TechDraw/Gui/Resources/translations/TechDraw.ts"],
          ]


# handler for the command responses
class ResponseHandler( xml.sax.ContentHandler ):

    def __init__(self):
        self.current = ""
        self.data = ""
        self.translated = 1
        self.total = 1

    def startElement(self, tag, attributes):
        self.current = tag
        if tag == "file":
            self.data += attributes["status"]
        elif tag == "error":
            self.data == "Error: "

    def endElement(self, tag):
        if self.current in ["language","success","error"]:
            self.data = ""
            self.translated = 1
            self.total = 1
        self.current = ""

    def characters(self, content):
        if self.current == "name":
            self.data += content
        elif self.current == "phrases":
            self.total = int(content)
        elif self.current == "translated":
            self.translated = int(content)
            pc = int((float(self.translated)/self.total)*100)
            self.data += " : " + str(pc) + "%\n"
        elif self.current == "message":
            self.data += content



if __name__ == "__main__":

    # only one argument allowed
    arg = sys.argv[1:]
    if len(arg) != 1:
        print(__doc__)
        sys.exit()
    arg = arg[0]

    # getting API key stored in ~/.crowdin-freecad
    configfile = os.path.expanduser("~")+os.sep+".crowdin-freecad"
    if not os.path.exists(configfile):
        print("Config file not found!")
        sys.exit()
    f = open(configfile)
    url = "https://api.crowdin.com/api/project/freecad/"
    key = "?key="+f.read().strip()
    f.close()

    if arg == "status":
        c = pycurl.Curl()
        c.setopt(pycurl.URL, url+"status"+key+"&xml")
        b = StringIO.StringIO()
        c.setopt(pycurl.WRITEFUNCTION, b.write)
        c.perform()
        c.close()
        handler = ResponseHandler()
        xml.sax.parseString(b.getvalue(),handler)
        print(handler.data)

    elif arg == "build":
        print("Building (warning, this can be invoked only once per 30 minutes)...")
        c = pycurl.Curl()
        c.setopt(pycurl.URL, url+"export"+key)
        b = StringIO.StringIO()
        c.setopt(pycurl.WRITEFUNCTION, b.write)
        c.perform()
        c.close()
        handler = ResponseHandler()
        xml.sax.parseString(b.getvalue(),handler)
        print(handler.data)

    elif arg == "download":
        print("Downloading all.zip in current directory...")
        cmd = "wget -O freecad.zip "+url+"download/all.zip"+key
        os.system(cmd)

    elif arg == "update":
        basepath = os.path.dirname(os.path.abspath("."))
        for f in files:
            print("Sending ",f[0],"...")
            c = pycurl.Curl()
            fields = [('files['+f[0]+']', (c.FORM_FILE, basepath+f[1]))]
            c.setopt(pycurl.URL, url+"update-file"+key)
            c.setopt(pycurl.HTTPPOST, fields)
            b = StringIO.StringIO()
            c.setopt(pycurl.WRITEFUNCTION, b.write)
            c.perform()
            c.close()
            handler = ResponseHandler()
            xml.sax.parseString(b.getvalue(),handler)
            print(handler.data)

    else:
        print(__doc__)
