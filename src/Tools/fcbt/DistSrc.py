# shell and operating system
import os, sys

# sys.path.append( "E:\\Develop\\Projekte\\FreeCADWin\\src\\Tools" )

from . import DistTools, FileTools

# line separator
ls = os.linesep
# path separator
ps = os.pathsep
# dir separator
ds = os.sep

DistName = DistTools.BuildDistName()

DistSrc = DistName + "_src"
DistDir = "../../DistTemp/"

# ====================================================================
# script assumes to run in src/Tools

DistTools.EnsureDir(DistDir)
if DistTools.EnsureDir(DistDir + DistSrc) == 1:
    raise RuntimeError("Dist path already there!!")

# ====================================================================
# copy src
sys.stdout.write("Copy src Tree ...\n")
DistTools.EnsureDir(DistDir + DistSrc + "/src")
FileTools.cpallWithFilter(
    "../../src", DistDir + DistSrc + "/src", FileTools.SetUpFilter(DistTools.SrcFilter)
)

# ====================================================================
# copy top level files

# FileTools.cpfile("../Doc/README.html",DistDir+DistBin+"/README.html")
# FileTools.cpfile("../Doc/INSTALL.html",DistDir+DistBin+"/INSTALL.html")
# FileTools.cpfile("../Doc/LICENSE.GPL.html",DistDir+DistBin+"/LICENSE.GPL.html")
# FileTools.cpfile("../Doc/LICENSE.LGPL.html",DistDir+DistBin+"/LICENSE.LGPL.html")
# DistTools.cpfile("../Tools/BuildTool.py",DistDir+DistBin+"/BuildTool.py")

# ====================================================================
# zipping an archive
os.popen("7z a -tzip " + DistDir + DistSrc + ".zip " + DistDir + DistSrc + " -mx9")

FileTools.rmall(DistDir + DistSrc)
