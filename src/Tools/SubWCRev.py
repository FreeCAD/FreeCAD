#! python
# -*- coding: utf-8 -*-
# (c) 2006 Werner Mayer LGPL
#
# FreeCAD RevInfo script to get the revision information from Subversion, Bazaar, and Git.
#
# Under Linux the Subversion tool SubWCRev shipped with TortoiseSVN isn't
# available which is provided by this script.
# 2012/02/01: The script was extended to support git
# 2011/02/05: The script was extended to support also Bazaar

import os, sys, re, time, getopt
import xml.sax
import xml.sax.handler
import xml.sax.xmlreader

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

# SAX handler to parse the subversion output
class SvnHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        super().__init__()
        self.inUrl = 0
        self.inDate = 0
        self.mapping = {}

    def startElement(self, name, attributes):
        if name == "entry":
            self.buffer = ""
            self.mapping["Rev"] = attributes["revision"]
        elif name == "url":
            self.inUrl = 1
        elif name == "date":
            self.inDate = 1

    def characters(self, data):
        if self.inUrl:
            self.buffer += data
        elif self.inDate:
            self.buffer += data

    def endElement(self, name):
        if name == "url":
            self.inUrl = 0
            self.mapping["Url"] = self.buffer
            self.buffer = ""
        elif name == "date":
            self.inDate = 0
            self.mapping["Date"] = self.buffer
            self.buffer = ""


class VersionControl:
    def __init__(self):
        self.rev = ""
        self.date = ""
        self.url = ""

    def extractInfo(self, srcdir, bindir):
        return False

    def printInfo(self):
        print("")

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


class DebianChangelog(VersionControl):
    def extractInfo(self, srcdir, bindir):
        # Do not overwrite existing file with almost useless information
        if os.path.exists(bindir + "/src/Build/Version.h.out"):
            return False
        try:
            f = open(srcdir + "/debian/changelog")
        except Exception:
            return False
        c = f.readline()
        f.close()
        r = re.search("bzr(\\d+)", c)
        if r is not None:
            self.rev = r.groups()[0] + " (Launchpad)"

        t = time.localtime()
        self.date = ("%d/%02d/%02d %02d:%02d:%02d") % (
            t.tm_year,
            t.tm_mon,
            t.tm_mday,
            t.tm_hour,
            t.tm_min,
            t.tm_sec,
        )
        self.url = "https://code.launchpad.net/~vcs-imports/freecad/trunk"
        return True

    def printInfo(self):
        print("debian/changelog")


class BazaarControl(VersionControl):
    def extractInfo(self, srcdir, bindir):
        info = os.popen("bzr log -l 1 %s" % (srcdir)).read()
        if len(info) == 0:
            return False
        lines = info.split("\n")
        for i in lines:
            r = re.match("^revno: (\\d+)$", i)
            if r is not None:
                self.rev = r.groups()[0]
                continue
            r = re.match("^timestamp: (\\w+ \\d+-\\d+-\\d+ \\d+:\\d+:\\d+)", i)
            if r is not None:
                self.date = r.groups()[0]
                continue
        return True

    def printInfo(self):
        print("bazaar")


class DebianGitHub(VersionControl):
    # https://gist.github.com/0penBrain/7be59a48aba778c955d992aa69e524c5
    # https://gist.github.com/yershalom/a7c08f9441d1aadb13777bce4c7cdc3b
    # https://github.community/t5/GitHub-API-Development-and/How-to-get-all-branches-which-contain-a-commit-from-SHA-using/td-p/25006
    def extractInfo(self, srcdir, bindir):
        try:
            f = open(srcdir + "/debian/git-build-recipe.manifest")
        except Exception:
            return False

        # Read the first two lines
        recipe = f.readline()
        commit = f.readline()
        f.close()

        base_url = "https://api.github.com"
        owner = "FreeCAD"
        repo = "FreeCAD"
        sha = commit[commit.rfind(":") + 1 : -1]
        self.hash = sha

        try:
            import requests

            request_url = "{}/repos/{}/{}/commits?per_page=1&sha={}".format(
                base_url, owner, repo, sha
            )
            commit_req = requests.get(request_url)
            if not commit_req.ok:
                return False

            commit_date = commit_req.headers.get("last-modified")

        except Exception:
            # if connection fails then use the date of the file git-build-recipe.manifest
            commit_date = recipe[recipe.rfind("~") + 1 : -1]

        try:
            # Try to convert into the same format as GitControl
            t = time.strptime(commit_date, "%a, %d %b %Y %H:%M:%S GMT")
            commit_date = ("%d/%02d/%02d %02d:%02d:%02d") % (
                t.tm_year,
                t.tm_mon,
                t.tm_mday,
                t.tm_hour,
                t.tm_min,
                t.tm_sec,
            )
        except Exception:
            t = time.strptime(commit_date, "%Y%m%d%H%M")
            commit_date = ("%d/%02d/%02d %02d:%02d:%02d") % (
                t.tm_year,
                t.tm_mon,
                t.tm_mday,
                t.tm_hour,
                t.tm_min,
                t.tm_sec,
            )

        self.date = commit_date
        self.branch = "unknown"

        try:
            # Try to determine the branch of the sha
            # There is no function of the rest API of GH but with the url below we get HTML code
            branch_url = "https://github.com/{}/{}/branch_commits/{}".format(owner, repo, sha)
            branch_req = requests.get(branch_url)
            if branch_req.ok:
                html = branch_req.text
                pattern = '<li class="branch"><a href='
                start = html.find(pattern) + len(pattern)
                end = html.find("\n", start)
                link = html[start:end]
                start = link.find(">") + 1
                end = link.find("<", start)
                self.branch = link[start:end]

            link = commit_req.headers.get("link")
            beg = link.rfind("&page=") + 6
            end = link.rfind(">")
            self.rev = link[beg:end] + " (GitHub)"
        except Exception:
            pass

        self.url = "git://github.com/{}/{}.git {}".format(owner, repo, self.branch)
        return True

    def writeVersion(self, lines):
        content = VersionControl.writeVersion(self, lines)
        content.append("// Git relevant stuff\n")
        content.append('#define FCRepositoryHash   "%s"\n' % (self.hash))
        content.append('#define FCRepositoryBranch "%s"\n' % (self.branch))
        return content

    def printInfo(self):
        print("Debian/GitHub")


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
                    match = re.match("git@github\.com:(\S+?)/(\S+\.git)", url) or re.match(
                        "https://github\.com/(\S+)/(\S+\.git)", url
                    )
                    if match is not None:
                        url = "git://github.com/%s/%s" % match.groups()
                    match = re.match("ssh://\S+?@(\S+)", url)
                    if match is not None:
                        url = "git://%s" % match.group(1)
                    entryscore = (
                        url == "git://github.com/FreeCAD/FreeCAD.git",
                        "github.com" in url,
                        branch == self.branch,
                        branch == "master",
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
        for master and release branches all commits are counted
        for other branches. The version number is split in to two parts:
        The first number reflects the number of commits in common with the
        blessed master repository. The second part (separated by " +") reflects
        the number of commits that are different from the master repository"""
        referencecommit = "7d8e53aaab17961d85c5009de34f69f2af084e8b"
        referencerevision = 14555

        result = None
        countallfh = os.popen("git rev-list --count %s..HEAD" % referencecommit)
        countallstr = countallfh.read().strip()
        if countallfh.close() is not None:  # reference commit not present
            self.rev = "%04d (Git shallow)" % referencerevision
            return
        else:
            countall = int(countallstr)

        if (
            origin is not None
            and self.branch.lower() != "master"
            and "release" not in self.branch.lower()
        ):
            mbfh = os.popen("git merge-base %s/master HEAD" % origin)
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
        if one parent is the master and the second one has no ref name, one branch was
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
            if hasnames >= 2:  # merging master into dev is not enough
                self.branch = ",".join(names)

    def extractInfo(self, srcdir, bindir):
        self.hash = os.popen("git log -1 --pretty=format:%H").read().strip()
        if self.hash == "":
            return False  # not a git repo
        # date/time
        import time

        info = os.popen("git log -1 --date=raw --pretty=format:%cd").read()
        # commit time is more meaningful than author time
        # use UTC
        self.date = time.strftime(
            "%Y/%m/%d %H:%M:%S", time.gmtime(float(info.strip().split(" ", 1)[0]))
        )
        for self.branch in os.popen("git branch --no-color").read().split("\n"):
            if re.match("\*", self.branch) is not None:
                break
        self.branch = self.branch[2:]
        self.getremotes()  # setup self.remotes and branchlst

        self.geturl()
        origin = None  # remote for the blessed master
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
        if self.branch.lower() != "master" and "release" not in self.branch.lower():
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

    def printInfo(self):
        print("git")

    def writeVersion(self, lines):
        content = VersionControl.writeVersion(self, lines)
        content.append("// Git relevant stuff\n")
        content.append('#define FCRepositoryHash   "%s"\n' % (self.hash))
        content.append('#define FCRepositoryBranch "%s"\n' % (self.branch))
        return content


class MercurialControl(VersionControl):
    def extractInfo(self, srcdir, bindir):
        return False

    def printInfo(self):
        print("mercurial")


class Subversion(VersionControl):
    def extractInfo(self, srcdir, bindir):
        parser = xml.sax.make_parser()
        handler = SvnHandler()
        parser.setContentHandler(handler)

        # Create an XML stream with the required information and read in with a SAX parser
        Ver = os.popen("svnversion %s -n" % (srcdir)).read()
        Info = os.popen("svn info %s --xml" % (srcdir)).read()
        try:
            inpsrc = xml.sax.InputSource()
            strio = StringIO.StringIO(Info)
            inpsrc.setByteStream(strio)
            parser.parse(inpsrc)
        except Exception:
            return False

        # Information of the Subversion stuff
        self.url = handler.mapping["Url"]
        self.rev = handler.mapping["Rev"]
        self.date = handler.mapping["Date"]
        self.date = self.date[:19]
        # Same format as SubWCRev does
        self.date = self.date.replace("T", " ")
        self.date = self.date.replace("-", "/")

        # Date is given as GMT. Now we must convert to local date.
        m = time.strptime(self.date, "%Y/%m/%d %H:%M:%S")
        # Copy the tuple and set tm_isdst to 0 because it's GMT
        l = (
            m.tm_year,
            m.tm_mon,
            m.tm_mday,
            m.tm_hour,
            m.tm_min,
            m.tm_sec,
            m.tm_wday,
            m.tm_yday,
            0,
        )
        # Take timezone into account
        t = time.mktime(l) - time.timezone
        self.date = time.strftime("%Y/%m/%d %H:%M:%S", time.localtime(t))

        # Get the current local date
        self.time = time.strftime("%Y/%m/%d %H:%M:%S")

        self.mods = "Src not modified"
        self.mixed = "Src not mixed"
        self.range = self.rev

        # if version string ends with an 'M'
        r = re.search("M$", Ver)
        if r is not None:
            self.mods = "Src modified"

        # if version string contains a range
        r = re.match("^\\d+\\:\\d+", Ver)
        if r is not None:
            self.mixed = "Src mixed"
            self.range = Ver[: r.end()]
        return True

    def printInfo(self):
        print("subversion")


def main():
    # if(len(sys.argv) != 2):
    #    sys.stderr.write("Usage:  SubWCRev \"`svn info .. --xml`\"\n")

    srcdir = "."
    bindir = "."
    try:
        opts, args = getopt.getopt(sys.argv[1:], "sb:", ["srcdir=", "bindir="])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-s", "--srcdir"):
            srcdir = a
        if o in ("-b", "--bindir"):
            bindir = a

    vcs = [
        GitControl(),
        DebianGitHub(),
        BazaarControl(),
        Subversion(),
        MercurialControl(),
        DebianChangelog(),
        UnknownControl(),
    ]
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
            i.printInfo()
            sys.stdout.write("%s/src/Build/Version.h.out written\n" % (bindir))
            break


if __name__ == "__main__":
    main()
