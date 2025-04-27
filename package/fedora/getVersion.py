#!/usr/bin/env python

# call this file from within the FreeCAD git repo
# this script creates a file with the important version information
import os
import sys


sys.path.append(f"{os.getcwd()}/src/Tools")
import SubWCRev

gitInfo = SubWCRev.GitControl()
gitInfo.extractInfo("","")

print(gitInfo.rev, end="")
