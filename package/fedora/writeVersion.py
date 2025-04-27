#!/usr/bin/env python

# call this file from within the FreeCAD git repo
# this script creates a file with the important version information
import os
import sys


sys.path.append(f"{os.getcwd()}/src/Tools")
import SubWCRev

gitInfo = SubWCRev.GitControl()
gitInfo.extractInfo("","")

i = open("src/Build/Version.h.cmake")
content = []
for line in i.readlines():
    line = line.replace("${PACKAGE_WCREF}",gitInfo.rev)
    line = line.replace("${PACKAGE_WCDATE}",gitInfo.date)
    line = line.replace("${PACKAGE_WCURL}",gitInfo.url)
    content.append(line)

with open("src/Build/Version.h.cmake", "w") as o:
	content.append('// Git relevant stuff\n')
	content.append('#define FCRepositoryHash   "%s"\n' % (gitInfo.hash))
	content.append('#define FCRepositoryBranch "%s"\n' % (gitInfo.branch))
	o.writelines(content)
