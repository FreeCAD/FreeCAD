#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

# (c) 2006 Werner Mayer LGPL
#
# Generate FreeCAD version information from Git.

import argparse
import datetime
import os
import re
import sys
import time
from urllib.parse import urlparse


class VersionControl:
    def __init__(self):
        self.rev = ""
        self.date = ""
        self.url = ""

    def extractInfo(self, srcdir, bindir):
        return False

    def writeVersion(self, lines):
        content = []
        for line in lines:
            line = line.replace("$WCREV$", self.rev)
            line = line.replace("$WCDATE$", self.date)
            line = line.replace("$WCURL$", self.url)
            content.append(line)
        return content


class UnknownControl(VersionControl):
    def extractInfo(self, srcdir, bindir):
        # Do not overwrite existing file with almost useless information
        if os.path.exists(bindir + "/src/Build/Version.h.out"):
            return False
        self.rev = "Unknown"
        self.date = "Unknown"
        self.url = "Unknown"
        return True

    def printInfo(self):
        print("Unknown version control")


class GitControl(VersionControl):
    # http://www.hermanradtke.com/blog/canonical-version-numbers-with-git/
    # http://blog.marcingil.com/2011/11/creating-build-numbers-using-git-commits/
    # http://gitref.org/remotes/#fetch
    # http://cworth.org/hgbook-git/tour/
    # http://git.or.cz/course/svn.html
    # git help log
    def getremotes(self):
        """return a mapping of remotes and their fetch urls"""
        rr = os.popen("git remote -v")
        rrstr = rr.read().strip()
        if rr.close() is None:  # exit code == 0
            self.remotes = dict(
                l[:-8].split("\t") for l in rrstr.splitlines() if l.endswith(" (fetch)")
            )
            self.branchlst = (
                os.popen("git show -s --pretty=%d HEAD").read().strip(" ()\n").split(", ")
            )  # used for possible remotes

    def geturl(self):
        urls = []
        for ref in self.branchlst:
            if "/" in ref:
                remote, branch = ref.split("/", 1)
                if remote in self.remotes:
                    url = self.remotes[remote]
                    # rewrite github to public url
                    match = re.match(r"git@github\.com:(\S+?)/(\S+\.git)", url) or re.match(
                        r"https://github\.com/(\S+)/(\S+\.git)", url
                    )
                    if match is not None:
                        url = "git://github.com/%s/%s" % match.groups()
                    match = re.match(r"ssh://\S+?@(\S+)", url)
                    if match is not None:
                        url = "git://%s" % match.group(1)
                    parsed_url = urlparse(url)
                    entryscore = (
                        url == "git://github.com/FreeCAD/FreeCAD.git",
                        parsed_url.netloc == "github.com",
                        branch == self.branch,
                        branch == "main",
                        "@" not in url,
                    )
                    # used for sorting the list
                    if branch == self.branch:  # add branch name
                        url = "%s %s" % (url, branch)
                    urls.append((entryscore, url))
        if len(urls) > 0:
            self.url = sorted(urls)[-1][1]
        else:
            self.url = "Unknown"

    def revisionNumber(self, srcdir, origin=None):
        """sets the revision number
        for main and release branches all commits are counted
        for other branches. The version number is split in to two parts:
        The first number reflects the number of commits in common with the
        blessed main repository. The second part (separated by " +") reflects
        the number of commits that are different from the main repository"""
        referencecommit = "7d8e53aaab17961d85c5009de34f69f2af084e8b"
        referencerevision = 14555

        result = None
        null_device = "nul" if os.name == "nt" else "/dev/null"
        countallfh = os.popen(f"git rev-list --count {referencecommit}..HEAD 2>{null_device}")
        countallstr = countallfh.read().strip()
        if countallfh.close() is not None:  # reference commit not present, use the date
            date_object = datetime.datetime.strptime(self.date, "%Y/%m/%d %H:%M:%S")
            formatted_date = date_object.strftime("%Y%m%d")
            self.rev = f"{formatted_date} (Git shallow)"
            return
        else:
            countall = int(countallstr)

        if (
            origin is not None
            and self.branch.lower() != "main"
            and "release" not in self.branch.lower()
        ):
            mbfh = os.popen("git merge-base %s/main HEAD" % origin)
            mergebase = mbfh.read().strip()
            if mbfh.close() is None:  # exit code == 0
                try:
                    countmergebase = int(
                        os.popen("git rev-list --count %s..%s" % (referencecommit, mergebase))
                        .read()
                        .strip()
                    )
                    if countall > countmergebase:
                        result = "%04d +%d (Git)" % (
                            countmergebase + referencerevision,
                            countall - countmergebase,
                        )
                except ValueError:
                    pass
        self.rev = result or ("%04d (Git)" % (countall + referencerevision))

    def namebranchbyparents(self):
        """name multiple branches in case that the last commit was a merge
        a merge is identified by having two or more parents
        if the describe does not return a ref name (the hash is added)
        if one parent is the main and the second one has no ref name, one branch was
        merged."""
        parents = os.popen("git log -n1 --pretty=%P").read().strip().split(" ")
        if len(parents) >= 2:  # merge commit
            parentrefs = []
            names = []
            hasnames = 0
            for p in parents:
                refs = os.popen("git show -s --pretty=%%d %s" % p).read().strip(" ()\n").split(", ")
                if refs[0] != "":  # has a ref name
                    parentrefs.append(refs)
                    names.append(refs[-1])
                    hasnames += 1
                else:
                    parentrefs.append(p)
                    names.append(p[:7])
            if hasnames >= 2:  # merging main into dev is not enough
                self.branch = ",".join(names)

    def extractInfo(self, srcdir, bindir):
        self.hash = os.popen("git log -1 --pretty=format:%H").read().strip()
        if self.hash == "":
            return False  # not a git repo
        # date/time
        info = os.popen("git log -1 --date=raw --pretty=format:%cd").read()
        # commit time is more meaningful than author time
        # use UTC
        self.date = time.strftime(
            "%Y/%m/%d %H:%M:%S", time.gmtime(float(info.strip().split(" ", 1)[0]))
        )
        for self.branch in os.popen("git branch --no-color").read().split("\n"):
            if re.match(r"\*", self.branch) is not None:
                break
        self.branch = self.branch[2:]
        self.getremotes()  # setup self.remotes and branchlst

        self.geturl()
        origin = None  # remote for the blessed main
        for fetchurl in (
            "git@github.com:FreeCAD/FreeCAD.git",
            "https://github.com/FreeCAD/FreeCAD.git",
        ):
            for key, url in self.remotes.items():
                if fetchurl in url:
                    origin = key
                    break
            if origin is not None:
                break

        self.revisionNumber(srcdir, origin)
        if self.branch.lower() != "main" and "release" not in self.branch.lower():
            self.namebranchbyparents()
        if self.branch == "(no branch)":  # check for remote branches
            if len(self.branchlst) >= 2:
                self.branch = self.branchlst[1]
            else:  # guess
                self.branch = "(%s)" % os.popen("git describe --all --dirty").read().strip()
        # if the branch name contained any slashes but was not a remote
        # there might be no result by now. Hence we assume origin
        if self.url == "Unknown":
            for i in info:
                r = re.match("origin\\W+(\\S+)", i)
                if r is not None:
                    self.url = r.groups()[0]
                    break
        return True

    def writeVersion(self, lines):
        content = VersionControl.writeVersion(self, lines)
        content.append("// Git relevant stuff\n")
        content.append('#define FCRepositoryHash   "%s"\n' % (self.hash))
        content.append('#define FCRepositoryBranch "%s"\n' % (self.branch))
        return content


def main():
    parser = argparse.ArgumentParser(
        description="Generate FreeCAD version information from Git.",
        allow_abbrev=False,
    )
    parser.add_argument("--srcdir", required=True, help="FreeCAD source directory")
    parser.add_argument("--bindir", required=True, help="FreeCAD build directory")
    args = parser.parse_args()
    srcdir = args.srcdir
    bindir = args.bindir

    vcs = [GitControl(), UnknownControl()]
    for i in vcs:
        if i.extractInfo(srcdir, bindir):
            # Open the template file and the version file
            inp = open("%s/src/Build/Version.h.in" % (bindir))
            lines = inp.readlines()
            inp.close()
            lines = i.writeVersion(lines)
            out = open("%s/src/Build/Version.h.out" % (bindir), "w")
            out.writelines(lines)
            out.write("\n")
            out.close()
            sys.stdout.write("%s/src/Build/Version.h.out written\n" % (bindir))
            break


if __name__ == "__main__":
    main()
