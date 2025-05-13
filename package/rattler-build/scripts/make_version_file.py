#!/usr/bin/env python

# call this file from within the FreeCAD git repo
# this script creates a file with the important version information
import os
import sys
import subprocess

sys.path.append(f"{os.getcwd()}/src/Tools")
import SubWCRev

gitInfo = SubWCRev.GitControl()
gitInfo.extractInfo("","")
gitDescription = os.environ['BUILD_TAG']

i = open("src/Build/Version.h.cmake")
content = []
for line in i.readlines():
	line = line.replace("-${PACKAGE_VERSION_SUFFIX}",gitDescription)
	line = line.replace("${PACKAGE_WCREF}",gitInfo.rev)
	line = line.replace("${PACKAGE_WCDATE}",gitInfo.date)
	line = line.replace("${PACKAGE_WCURL}",gitInfo.url)
	content.append(line)

with open("src/Build/Version.h.cmake", "w") as o:
	content.append('// Git relevant stuff\n')
	content.append('#define FCRepositoryHash   "%s"\n' % (gitInfo.hash))
	content.append('#define FCRepositoryBranch "%s"\n' % (gitInfo.branch))
	o.writelines(content)

with open(os.sys.argv[1], "w") as f:
	f.write(f"rev_number: {gitInfo.rev}\n")
	f.write(f"branch_name: {gitInfo.branch}\n")
	f.write(f"commit_date: {gitInfo.date}\n")
	f.write(f"commit_hash: {gitInfo.hash}\n")
	f.write(f"remote_url: {gitInfo.url}\n")

p = subprocess.Popen(["git", "-c", "user.name='github-actions[bot]'", "-c", "user.email='41898282+github-actions[bot]@users.noreply.github.com'",
		       "commit", "-a", "-m", "add git information"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

out, err = p.communicate()

print(out.decode())
print(err.decode())
