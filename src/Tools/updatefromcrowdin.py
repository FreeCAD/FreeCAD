#!/usr/bin/python

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *  
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
Usage:

    updatefromcrowdin.py [options] [LANGCODE] [LANGCODE LANGCODE...]

Example:

    ./updatefromcrowdin.py [-d <directory>] fr nl pt_BR

Options:

    -h or --help : prints this help text
    -d or --directory : specifies a directory containing unzipped translation folders
    -z or --zipfile : specifies a path to the freecad.zip file
    -m or --module : specifies a single module name to be updated, instead of all modules
    
This command must be run from its current source tree location (/src/Tools)
so it can find the correct places to put the translation files.  If run with
no arguments, the latest translations from crowdin will be downloaded, unzipped
and put to the correct locations. The necessary renaming of files and .qm generation
will be taken care of. The qrc files will also be updated when new
translations are added.

NOTE! The crowdin site only allows to download "builds" (zipped archives)
which must be built prior to downloading. This means a build might not
reflect the latest state of the translations. Better always make a build before
using this script!

You can specify a directory with the -d option if you already downloaded
and extracted the build, or you can specify a single module to update with -m.

You can also run the script without any language code, in which case all the
languages contained in the archive or directory will be added.
'''

import sys, os, shutil, tempfile, zipfile, getopt, StringIO, re

crowdinpath = "http://crowdin.net/download/project/freecad.zip"

# locations list contains Module name, relative path to translation folder, relative path to qrc file, and optionally
# a python rc file

locations = [["Arch","../Mod/Arch/Resources/translations","../Mod/Arch/Resources/Arch.qrc"],
             ["Assembly","../Mod/Assembly/Gui/Resources/translations","../Mod/Assembly/Gui/Resources/Assembly.qrc"],
             ["draft","../Mod/Draft/Resources/translations","../Mod/Draft/Resources/Draft.qrc"],
             ["Drawing","../Mod/Drawing/Gui/Resources/translations","../Mod/Drawing/Gui/Resources/Drawing.qrc"],
             ["Fem","../Mod/Fem/Gui/Resources/translations","../Mod/Fem/Gui/Resources/Fem.qrc"],
             ["FreeCAD","../Gui/Language","../Gui/Language/translation.qrc"],
             ["Image","../Mod/Image/Gui/Resources/translations","../Mod/Image/Gui/Resources/Image.qrc"],
             ["Mesh","../Mod/Mesh/Gui/Resources/translations","../Mod/Mesh/Gui/Resources/Mesh.qrc"],
             ["MeshPart","../Mod/MeshPart/Gui/Resources/translations","../Mod/MeshPart/Gui/Resources/MeshPart.qrc"],
             ["OpenSCAD","../Mod/OpenSCAD/Resources/translations","../Mod/OpenSCAD/Resources/OpenSCAD.qrc"],
             ["Part","../Mod/Part/Gui/Resources/translations","../Mod/Part/Gui/Resources/Part.qrc"],
             ["PartDesign","../Mod/PartDesign/Gui/Resources/translations","../Mod/PartDesign/Gui/Resources/PartDesign.qrc"],
             ["Points","../Mod/Points/Gui/Resources/translations","../Mod/Points/Gui/Resources/Points.qrc"],
             ["Raytracing","../Mod/Raytracing/Gui/Resources/translations","../Mod/Raytracing/Gui/Resources/Raytracing.qrc"],
             ["ReverseEngineering","../Mod/ReverseEngineering/Gui/Resources/translations","../Mod/ReverseEngineering/Gui/Resources/ReverseEngineering.qrc"],
             ["Robot","../Mod/Robot/Gui/Resources/translations","../Mod/Robot/Gui/Resources/Robot.qrc"],
             ["Sketcher","../Mod/Sketcher/Gui/Resources/translations","../Mod/Sketcher/Gui/Resources/Sketcher.qrc"],
             ["StartPage","../Mod/Start/Gui/Resources/translations","../Mod/Start/Gui/Resources/Start.qrc"],
             ["Test","../Mod/Test/Gui/Resources/translations","../Mod/Test/Gui/Resources/Test.qrc"],
             ["Ship","../Mod/Ship/resources/translations","../Mod/Ship/resources/Ship.qrc"],
             ["Plot","../Mod/Plot/resources/translations","../Mod/Plot/resources/Plot.qrc"],
             ["Web","../Mod/Web/Gui/Resources/translations","../Mod/Web/Gui/Resources/Web.qrc"],
             ["Spreadsheet","../Mod/Spreadsheet/Gui/Resources/translations","../Mod/Spreadsheet/Gui/Resources/Spreadsheet.qrc"],
             ["Path","../Mod/Path/Gui/Resources/translations","../Mod/Path/Gui/Resources/Path.qrc"],
             ["Tux","../Mod/Tux/Resources/translations","../Mod/Tux/Resources/Tux.qrc"],
             ["TechDraw","../Mod/TechDraw/Gui/Resources/translations","../Mod/TechDraw/Gui/Resources/TechDraw.qrc"],
             ]
             
default_languages = "af ar ca cs de el es-ES eu fi fil fr gl hr hu id it ja kab ko lt nl no pl pt-BR pt-PT ro ru sk sl sr sv-SE tr uk val-ES vi zh-CN zh-TW"

def updateqrc(qrcpath,lncode):
    "updates a qrc file with the given translation entry"

    print("opening " + qrcpath + "...")
    
    # getting qrc file contents
    if not os.path.exists(qrcpath):
        print("ERROR: Resource file " + qrcpath + " doesn't exist")
        sys.exit()
    f = open(qrcpath,"ro")
    resources = []
    for l in f.readlines():
        resources.append(l)
    f.close()

    # checking for existing entry
    name = "_" + lncode + ".qm"
    for r in resources:
        if name in r:
            print("language already exists in qrc file")
            return

    # find the latest qm line
    pos = None
    for i in range(len(resources)):
        if ".qm" in resources[i]:
            pos = i
    if pos == None:
        print("No existing .qm file in this resource. Appending to the end position")
        for i in range(len(resources)):
            if "</qresource>" in resources[i]:
                pos = i-1
    if pos == None:
        print("ERROR: couldn't add qm files to this resource: " + qrcpath)
        sys.exit()

    # inserting new entry just after the last one
    line = resources[pos]
    if ".qm" in line:
        line = re.sub("_.*\.qm","_"+lncode+".qm",line)
    else:
        modname = os.path.splitext(os.path.basename(qrcpath))[0]
        line = "        <file>translations/"+modname+"_"+lncode+".qm</file>\n"
        #print "ERROR: no existing qm entry in this resource: Please add one manually " + qrcpath
        #sys.exit()
    print("inserting line: ",line)
    resources.insert(pos+1,line)

    # writing the file
    f = open(qrcpath,"wb")
    for r in resources:
        f.write(r)
    f.close()
    print("successfully updated ",qrcpath)

def doFile(tsfilepath,targetpath,lncode,qrcpath):
    "updates a single ts file, and creates a corresponding qm file"
    basename = os.path.basename(tsfilepath)[:-3]
    # special fix of the draft filename...
    if basename == "draft": basename = "Draft"
    newname = basename + "_" + lncode + ".ts"
    newpath = targetpath + os.sep + newname
    shutil.copyfile(tsfilepath, newpath)
    os.system("lrelease " + newpath)
    newqm = targetpath + os.sep + basename + "_" + lncode + ".qm"
    if not os.path.exists(newqm):
        print("ERROR: impossible to create " + newqm + ", aborting")
        sys.exit()
    updateqrc(qrcpath,lncode)

def doLanguage(lncode,fmodule=""):
    " treats a single language"
    if lncode == "en":
        # never treat "english" translation... For now :)
        return
    mods = []
    if fmodule:
        for l in locations:
            if l[0].upper() == fmodule.upper():
                mods = [l]
    else:
        mods = locations
    if not mods:
        print("Error: Couldn't find module "+fmodule)
        sys.exit()
    for target in mods:
        basefilepath = tempfolder + os.sep + lncode + os.sep + target[0] + ".ts"
        targetpath = os.path.abspath(target[1])
        qrcpath = os.path.abspath(target[2])
        doFile(basefilepath,targetpath,lncode,qrcpath)
    print(lncode + " done!")

if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) < 1:
        print(__doc__)
        sys.exit()
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:z:m:", ["help", "directory=","zipfile=", "module="])
    except getopt.GetoptError:
        print(__doc__)
        sys.exit()
        
    # checking on the options
    inputdir = ""
    inputzip = ""
    fmodule = ""
    for o, a in opts:
        if o in ("-h", "--help"):
            print(__doc__)
            sys.exit()
        if o in ("-d", "--directory"):
            inputdir = a
        if o in ("-z", "--zipfile"):
            inputzip = a
        if o in ("-m", "--module"):
            fmodule = a

    currentfolder = os.getcwd()
    if inputdir:
        tempfolder = os.path.realpath(inputdir)
        if not os.path.exists(tempfolder):
            print("ERROR: " + tempfolder + " not found")
            sys.exit()
    elif inputzip:
        tempfolder = tempfile.mkdtemp()
        print("creating temp folder " + tempfolder)
        os.chdir(tempfolder)
        inputzip=os.path.realpath(inputzip)
        if not os.path.exists(inputzip):
            print("ERROR: " + inputzip + " not found")
            sys.exit()
        shutil.copy(inputzip,tempfolder)
        zfile=zipfile.ZipFile("freecad.zip")
        print("extracting freecad.zip...")
        zfile.extractall()
    else:
        tempfolder = tempfile.mkdtemp()
        print("creating temp folder " + tempfolder)
        os.chdir(tempfolder)
        os.system("wget "+crowdinpath)
        if not os.path.exists("freecad.zip"):
            print("download failed!")
            sys.exit()
        zfile=zipfile.ZipFile("freecad.zip")
        print("extracting freecad.zip...")
        zfile.extractall()
    os.chdir(currentfolder)
    if not args:
        #args = [o for o in os.listdir(tempfolder) if o != "freecad.zip"]
        # do not treat all languages in the zip file. Some are not translated enough.
        args = default_languages.split()
    for ln in args:
        if not os.path.exists(tempfolder + os.sep + ln):
            print("ERROR: language path for " + ln + " not found!")
        else:
            doLanguage(ln,fmodule)

