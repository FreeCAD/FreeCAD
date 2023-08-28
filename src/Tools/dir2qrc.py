#! python
# -*- coding: utf-8 -*-
# (c) 2007 Juergen Riegel GPL

Usage = """dir2qrc - merging all files in a directory in a qrc file

Usage:
   dir2qrc [Optionen]

Options:
 -v, --verbose            print out all files collected
 -o  --out-file=FILENAME  use this file name for output, default resources.qrc
 -d, --directory=DIRNAME  directory to search, default PWD
 -h, --help               print this help message

This program walks a directory (tree) and collects all supported files
and put them in a .qrc file, to compile in with the QT resource facility.

Examples:

   dir2qrc  -v -o resource.qrc -d "d:/Develop/Resources"

Author:
  (c) 2007 Juergen Riegel
  juergen.riegel@web.de
  Licence: GPL

Version:
  0.1
"""

import os, sys, getopt
from os.path import join

# Globals
Verbose = False
Automatic = False
ExtraDist = False
Dir = "."
Output = "resources.qrc"

hhcHeader = """<RCC>
    <qresource%s>
"""
hhcFooter = """    </qresource>
</RCC>
"""

EndingList = [".xpm", ".svg", ".qm", ".png", ".ui"]

locations = [
    ["../Gui/Language", "translation.qrc", ' prefix="/translations"'],
    ["../Gui/Icons", "resource.qrc", ' prefix="/icons"'],
    ["../Mod/Assembly/Gui/Resources", "Assembly.qrc"],
    ["../Mod/Complete/Gui/Resources", "Complete.qrc"],
    ["../Mod/Draft/Resources", "Draft.qrc"],
    ["../Mod/Drawing/Gui/Resources", "Drawing.qrc"],
    ["../Mod/Fem/Gui/Resources", "Fem.qrc"],
    ["../Mod/Image/Gui/Resources", "Image.qrc"],
    ["../Mod/Mesh/Gui/Resources", "Mesh.qrc"],
    ["../Mod/MeshPart/Gui/Resources", "MeshPart.qrc"],
    ["../Mod/Part/Gui/Resources", "Part.qrc"],
    ["../Mod/PartDesign/Gui/Resources", "PartDesign.qrc"],
    ["../Mod/Points/Gui/Resources", "Points.qrc"],
    ["../Mod/ReverseEngineering/Gui/Resources", "ReverseEngineering.qrc"],
    ["../Mod/Robot/Gui/Resources", "Robot.qrc"],
    ["../Mod/Sketcher/Gui/Resources", "Sketcher.qrc"],
]


def main():
    global Verbose, Automatic, ExtraDist, Dir, Output

    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            "hvd:o:",
            ["help", "verbose", "auto", "dist", "directory=", "out-file="],
        )
    except getopt.GetoptError:
        # print help information and exit:
        sys.stderr.write(Usage)
        sys.exit(2)

    # checking on the options
    for o, a in opts:
        if o == "-v":
            Verbose = True
        if o in ("-h", "--help"):
            sys.stderr.write(Usage)
            sys.exit()
        if o in ("-a", "--auto"):
            Automatic = True
        if o in ("--dist"):
            ExtraDist = True
        if o in ("-o", "--out-file"):
            Output = a
        if o in ("-d", "--directory"):
            print("Using path: " + a + "\n")
            Dir = a

    if Automatic:
        path = os.path.realpath(__file__)
        path = os.path.dirname(path)
        for i in locations:
            qrcDir = os.path.realpath(join(path, i[0]))
            if len(i) > 2:
                updateResourceFile(qrcDir, i[1], i[2])
            else:
                updateResourceFile(qrcDir, i[1])
            if ExtraDist:
                makeTargetExtraDist(qrcDir)
    else:
        updateResourceFile(Dir, Output)
        if ExtraDist:
            makeTargetExtraDist(Dir)


def updateResourceFile(Dir, Output, prefix=""):
    global Verbose
    Output = join(Dir, Output)
    file = open(Output, "w")
    file.write(hhcHeader % (prefix))
    DirPath = Dir + os.path.sep
    filelist = []
    for root, dirs, files in os.walk(Dir):
        for name in files:
            if (1 in [c in name for c in EndingList]) and not (".svn" in root):
                FilePathOrg = join(root, name)
                FilePath = FilePathOrg.replace(DirPath, "")
                FilePath = FilePath.replace(".\\", "")
                FilePath = FilePath.replace("\\", "/")
                if Verbose:
                    print(FilePathOrg + " -> " + FilePath)
                filelist.append(FilePath)

    filelist.sort()
    for i in filelist:
        file.write("        <file>" + i + "</file>\n")

    file.write(hhcFooter)
    file.close()


def makeTargetExtraDist(Dir):
    extensions = EndingList[:]
    extensions.append(".qrc")
    extensions.append(".bat")
    extensions.append(".ts")
    print("EXTRA_DIST = \\")
    DirPath = Dir + os.path.sep
    for root, dirs, files in os.walk(Dir):
        for name in files:
            if (1 in [c in name for c in extensions]) and not (".svn" in root):
                FilePathOrg = join(root, name)
                FilePath = FilePathOrg.replace(DirPath, "")
                FilePath = FilePath.replace(".\\", "")
                FilePath = FilePath.replace("\\", "/")
                print("\t\t%s \\" % (FilePath))
    print()


if __name__ == "__main__":
    main()
