#!/usr/bin/python

'''
Usage:

    updateTranslations.py LANGCODE [LANGCODE LANGCODE...]

Example:

    ./updateTranslations.py [-d <directory>] fr nl pt_BR

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
'''

import sys, os, shutil, tempfile, zipfile, getopt, StringIO
import xml.sax, xml.sax.handler, xml.sax.xmlreader


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
             ["Sketcher","../Mod/Sketcher/Gui/Resources/translations"]]

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

def doLanguage(lncode):
    " treats a single language"
    for target in locations:
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
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory="])
    except getopt.GetoptError:
        print __doc__
        sys.exit()
        
    # checking on the options
    inputdir=""
    for o, a in opts:
        if o in ("-h", "--help"):
            print __doc__
            sys.exit()
        if o in ("-d", "--directory"):
            inputdir = a

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
    for ln in args:
        if not os.path.exists(tempfolder + os.sep + ln):
            print "language path for " + ln + " not found!"
        else:
            doLanguage(ln)

