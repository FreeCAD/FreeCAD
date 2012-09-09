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


'''
Usage:

    updateTranslations.py [options] [LANGCODE] [LANGCODE LANGCODE...]

Example:

    ./updateTranslations.py [-d <directory>] fr nl pt_BR

Options:

    -h : prints this help text
    -d : specifies a directory containing the freecad.zip file.
    -m : specifies a single module name to be updated, instead of all modules
    
This command must be run from its current source tree location (/src/Tools)
so it can find the correct places to put the translation files. The
latest translations from crowdin will be downloaded, unzipped and put to
the correct locations. The necessary renaming of files and .qm generation
will be taken care of, but no resource file or other stuff will be done,
only the correct copying of .ts and .qm files.

NOTE! The crowdin site only allows to download "builds" (zipped archives)
which must be built prior to downloading. This means a build might not
reflect the latest state of the translations. Better make a build before
using this script!

You can specify a directory with the -d option if you already downloaded
and the build, or you can specify a single module to update with -m.

You can also run the script without any language code, in which case the
default ones specified inside this script will be used.
'''

import sys, os, shutil, tempfile, zipfile, getopt, StringIO
import xml.sax, xml.sax.handler, xml.sax.xmlreader

defaultLanguages = ["af","de","es-ES","fi","fr","hr","hu","it","ja","nl",
                    "no","pl","pt-BR","ru","sv-SE","uk","zh-CN"]

crowdinpath = "http://crowdin.net/download/project/freecad.zip"

locations = [["Assembly","../Mod/Assembly/Gui/Resources/translations"],
             ["Complete","../Mod/Complete/Gui/Resources/translations"],
             ["draft","../Mod/Draft/Resources/translations"],
             ["Drawing","../Mod/Drawing/Gui/Resources/translations"],
             ["Fem","../Mod/Fem/Gui/Resources/translations"],
             ["FreeCAD","../Gui/Language"],
             ["Image","../Mod/Image/Gui/Resources/translations"],
             ["Mesh","../Mod/Mesh/Gui/Resources/translations"],
             ["MeshPart","../Mod/MeshPart/Gui/Resources/translations"],
             ["Part","../Mod/Part/Gui/Resources/translations"],
             ["PartDesign","../Mod/PartDesign/Gui/Resources/translations"],
             ["Points","../Mod/Points/Gui/Resources/translations"],
             ["Raytracing","../Mod/Raytracing/Gui/Resources/translations"],
             ["ReverseEngineering","../Mod/ReverseEngineering/Gui/Resources/translations"],
             ["Robot","../Mod/Robot/Gui/Resources/translations"],
             ["Sketcher","../Mod/Sketcher/Gui/Resources/translations"],
             ["Arch","../Mod/Arch/Resources/translations"],
             ["StartPage","../Mod/Start/Gui/Resources/translations"],
             ["Test","../Mod/Test/Gui/Resources/translations"]]

tweaks = [["pt-BR","pt"],["es-ES","es"],["sv-SE","se"],["zh-CN","zh"]]

# SAX handler to parse the subversion output
class SvnHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.is_versioned = True

    def startElement(self, name, attributes):
        if name == "wc-status":
            if attributes["item"] == "unversioned":
                self.is_versioned = False

def doFile(tsfilepath,targetpath,lncode):
    "treats a single file"
    basename = os.path.basename(tsfilepath)[:-3]
    # special fix of the draft filename...
    if basename == "draft": basename = "Draft"
    # tweak of the final name...
    for t in tweaks:
        if lncode == t[0]:
            lncode = t[1]
    newname = basename + "_" + lncode + ".ts"
    newpath = targetpath + os.sep + newname
    shutil.copyfile(tsfilepath, newpath)
    os.system("lrelease " + newpath)
    newqm = targetpath + os.sep + basename + "_" + lncode + ".qm"
    if not os.path.exists(newqm):
        print "ERROR: " + newqm + "not released"

    # handle .ts file
    parser=xml.sax.make_parser()
    handler=SvnHandler()
    parser.setContentHandler(handler)
    info=os.popen("svn status %s --xml" % (newpath)).read()
    try:
        inpsrc = xml.sax.InputSource()
        strio=StringIO.StringIO(info)
        inpsrc.setByteStream(strio)
        parser.parse(inpsrc)
        if not handler.is_versioned:
            print "Add file %s to subversion" % (newpath)
            os.system("svn add %s" % (newpath))
    except:
        pass

    # handle .qm file
    parser=xml.sax.make_parser()
    handler=SvnHandler()
    parser.setContentHandler(handler)
    info=os.popen("svn status %s --xml" % (newqm)).read()
    try:
        inpsrc = xml.sax.InputSource()
        strio=StringIO.StringIO(info)
        inpsrc.setByteStream(strio)
        parser.parse(inpsrc)
        if not handler.is_versioned:
            print "Add file %s to subversion" % (newqm)
            os.system("svn add %s" % (newqm))
    except:
        pass

def doLanguage(lncode,fmodule=""):
    " treats a single language"
    mods = []
    if fmodule:
        for l in locations:
            if l[0].upper() == fmodule.upper():
                mods = [l]
    else:
        mods = locations
    if not mods:
        print "Error: Couldn't find module "+fmodule
        sys.exit()
    for target in mods:
        basefilepath = tempfolder + os.sep + lncode + os.sep + target[0] + ".ts"
        targetpath = os.path.abspath(target[1])
        doFile(basefilepath,targetpath,lncode)
    print lncode + " done!"

if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) < 1:
        print __doc__
        sys.exit()
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:m:", ["help", "directory=","module="])
    except getopt.GetoptError:
        print __doc__
        sys.exit()
        
    # checking on the options
    inputdir = ""
    fmodule = ""
    for o, a in opts:
        if o in ("-h", "--help"):
            print __doc__
            sys.exit()
        if o in ("-d", "--directory"):
            inputdir = a
        if o in ("-m", "--module"):
            fmodule = a

    tempfolder = tempfile.mkdtemp()
    print "creating temp folder " + tempfolder
    currentfolder = os.getcwd()
    os.chdir(tempfolder)
    if len(inputdir) > 0:
        inputdir=os.path.realpath(inputdir)
        inputdir=os.sep.join((inputdir,"freecad.zip"))
        if not os.path.exists(inputdir):
            print "freecad.zip does not exist!"
            sys.exit()
        shutil.copy(inputdir,tempfolder)
    else:
        os.system("wget "+crowdinpath)
        if not os.path.exists("freecad.zip"):
            print "download failed!"
            sys.exit()
    zfile=zipfile.ZipFile("freecad.zip")
    print "extracting freecad.zip..."
    zfile.extractall()
    os.chdir(currentfolder)
    if not args:
        args = defaultLanguages
    for ln in args:
        if not os.path.exists(tempfolder + os.sep + ln):
            print "language path for " + ln + " not found!"
        else:
            doLanguage(ln,fmodule)

