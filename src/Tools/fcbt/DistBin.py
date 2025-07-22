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

DistBin = DistName + "_binary_WinX86"
DistDir = "../../DistTemp/"

# ====================================================================
# script assumes to run in src/Tools

DistTools.EnsureDir(DistDir)
if DistTools.EnsureDir(DistDir + DistBin) == 1:
    raise RuntimeError("Dist path already there!!")

# ====================================================================
# copy src
sys.stdout.write("Copy src Tree ...\n")
DistTools.EnsureDir(DistDir + DistBin + "/src")
FileTools.cpallWithFilter(
    "../../src", DistDir + DistBin + "/src", FileTools.SetUpFilter(DistTools.SrcFilter)
)

# ====================================================================
# copy bin and lib
sys.stdout.write("Copy bin and lib Tree ...\n")
DistTools.EnsureDir(DistDir + DistBin + "/bin")
FileTools.cpallWithFilter(
    "../../bin", DistDir + DistBin + "/bin", FileTools.SetUpFilter(DistTools.BinFilter)
)
DistTools.EnsureDir(DistDir + DistBin + "/lib")
FileTools.cpallWithFilter(
    "../../lib", DistDir + DistBin + "/lib", FileTools.SetUpFilter(DistTools.LibFilter)
)

# ====================================================================
# copy Modules
sys.stdout.write("Copy module Tree ...\n")
DistTools.EnsureDir(DistDir + DistBin + "/Mod")
FileTools.cpallWithFilter(
    "../../src/Mod",
    DistDir + DistBin + "/Mod",
    FileTools.SetUpFilter(DistTools.ModFilter),
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
# os.popen("rar.exe a "+DistDir+DistBin+".rar "+ DistDir+DistBin)
os.popen("7z a -tzip " + DistDir + DistBin + ".zip " + DistDir + DistBin + " -mx9")

FileTools.rmall(DistDir + DistBin)
