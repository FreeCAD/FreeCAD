# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

from . import DistTools, FileTools

DistName = DistTools.BuildDistName()

DistInst = DistName + "_installer.msi"
DistDir = "../../DistTemp/"

# ====================================================================
# copy installer file

FileTools.cpfile("../../Install/FreeCAD.msi", DistDir + DistInst)
