

# shell and operating system
import os

#import sys
#sys.path.append( "E:\\Develop\\Projekte\\FreeCADWin\\src\\Tools" )

from . import DistTools,FileTools

DistName = DistTools.BuildDistName()

DistInst  = DistName + "_installer.msi"
DistDir  = "../../DistTemp/"

#====================================================================
# copy intaller file

FileTools.cpfile("../../Install/FreeCAD.msi",DistDir+DistInst)

