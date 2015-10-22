

# shell and operating system
import os,sys
#sys.path.append( "E:\\Develop\\Projekte\\FreeCADWin\\src\\Tools" )

import DistTools,FileTools

# line seperator 
ls = os.linesep
# path seperator
ps = os.pathsep
# dir seperator
ds = os.sep

DistName = DistTools.BuildDistName()

DistInst  = DistName + "_installer.msi"
DistDir  = "../../DistTemp/"

#====================================================================
# copy intaller file

FileTools.cpfile("../../Install/FreeCAD.msi",DistDir+DistInst)

