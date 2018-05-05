

# shell and operating system
import os,sys
#sys.path.append( "E:\\Develop\\Projekte\\FreeCADWin\\src\\Tools" )

import DistTools,FileTools

# line separator 
ls = os.linesep
# path separator
ps = os.pathsep
# dir separator
ds = os.sep

DistName = DistTools.BuildDistName()

DistInst  = DistName + "_installer.msi"
DistDir  = "../../DistTemp/"

#====================================================================
# copy intaller file

FileTools.cpfile("../../Install/FreeCAD.msi",DistDir+DistInst)

