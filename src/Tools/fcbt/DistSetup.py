
from . import DistTools,FileTools

DistName = DistTools.BuildDistName()

DistInst  = DistName + "_installer.msi"
DistDir  = "../../DistTemp/"

#====================================================================
# copy intaller file

FileTools.cpfile("../../Install/FreeCAD.msi",DistDir+DistInst)

