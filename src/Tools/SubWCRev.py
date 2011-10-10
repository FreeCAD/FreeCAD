# FreeCAD RevInfo script to get the revision information from Subversion.
# (c) 2006 Werner Mayer
#
# Under Linux the Subversion tool SubWCRev shipped with TortoiseSVN isn't 
# available which is provided by this script. 
# 2011/02/05: The script was extended to support also Bazaar

#***************************************************************************
#*   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

#!/usr/bin/python
import os,sys,string,re,time,getopt
import xml.sax
import xml.sax.handler
import xml.sax.xmlreader
import StringIO

# SAX handler to parse the subversion output
class SvnHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
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
        self.range = ""
        self.url = ""
        self.time = ""
        self.mods = "Src not modified"
        self.mixed = "Src not mixed"

    def extractInfo(self, srcdir):
        return False

    def printInfo(self):
        print ""

    def writeVersion(self, lines):
        content=[]
        for line in lines:
            line = string.replace(line,'$WCREV$',self.rev)
            line = string.replace(line,'$WCDATE$',self.date)
            line = string.replace(line,'$WCRANGE$',self.range)
            line = string.replace(line,'$WCURL$',self.url)
            line = string.replace(line,'$WCNOW$',self.time)
            line = string.replace(line,'$WCMODS?Src modified:Src not modified$',self.mods)
            line = string.replace(line,'$WCMIXED?Src mixed:Src not mixed$',self.mixed)
            content.append(line)
        return content

class UnknownControl(VersionControl):
    def extractInfo(self, srcdir):
        # Do not overwrite existing file with almost useless information
        if os.path.exists(srcdir+"/src/Build/Version.h"):
            return False
        self.rev = "Unknown"
        self.date = "Unknown"
        self.range = "Unknown"
        self.url = "Unknown"
        self.time = "Unknown"
        return True

    def printInfo(self):
        print "Unknown version control"

class DebianChangelog(VersionControl):
    def extractInfo(self, srcdir):
        # Do not overwrite existing file with almost useless information
        if os.path.exists(srcdir+"/src/Build/Version.h"):
            return False
        try:
            f = open(srcdir+"/debian/changelog")
        except:
            return False
        c = f.readline()
        f.close()
        r=re.search("bzr(\\d+)",c)
        if r != None:
            self.rev = r.groups()[0] + " (Launchpad)"
            self.range = self.rev
        
        t = time.localtime()
        self.url = "https://code.launchpad.net/~vcs-imports/freecad/trunk"
        #self.time = time.asctime()
        self.date = ("%d/%02d/%02d %02d:%02d:%02d") % (t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec)
        self.time = ("%d/%02d/%02d %02d:%02d:%02d") % (t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec)
        return True

    def printInfo(self):
        print "debian/changelog"

class BazaarControl(VersionControl):
    def extractInfo(self, srcdir):
        info=os.popen("bzr log -l 1 %s" % (srcdir)).read()
        if len(info) == 0:
            return False
        #Get the current local date
        self.time = time.strftime("%Y/%m/%d %H:%M:%S")
        lines=info.split("\n")
        for i in lines:
            r = re.match("^revno: (\\d+)$", i)
            if r != None:
                self.rev = r.groups()[0]
                continue
            r=re.match("^timestamp: (\\w+ \\d+-\\d+-\\d+ \\d+:\\d+:\\d+)",i)
            if r != None:
                self.date = r.groups()[0]
                continue
        return True

    def printInfo(self):
        print "bazaar"

class GitControl(VersionControl):
    def extractInfo(self, srcdir):
        return False

    def printInfo(self):
        print "git"

class MercurialControl(VersionControl):
    def extractInfo(self, srcdir):
        return False

    def printInfo(self):
        print "mercurial"

class Subversion(VersionControl):
    def extractInfo(self, srcdir):
        parser=xml.sax.make_parser()
        handler=SvnHandler()
        parser.setContentHandler(handler)

        #Create an XML stream with the required information and read in with a SAX parser
        Ver=os.popen("svnversion %s -n" % (srcdir)).read()
        Info=os.popen("svn info %s --xml" % (srcdir)).read()
        try:
            inpsrc = xml.sax.InputSource()
            strio=StringIO.StringIO(Info)
            inpsrc.setByteStream(strio)
            parser.parse(inpsrc)
        except:
            return False

        #Information of the Subversion stuff
        self.url = handler.mapping["Url"]
        self.rev = handler.mapping["Rev"]
        self.date = handler.mapping["Date"]
        self.date = self.date[:19]
        #Same format as SubWCRev does
        self.date = string.replace(self.date,'T',' ')
        self.date = string.replace(self.date,'-','/')

        #Date is given as GMT. Now we must convert to local date.
        m=time.strptime(self.date,"%Y/%m/%d %H:%M:%S")
        #Copy the tuple and set tm_isdst to 0 because it's GMT
        l=(m.tm_year,m.tm_mon,m.tm_mday,m.tm_hour,m.tm_min,m.tm_sec,m.tm_wday,m.tm_yday,0)
        #Take timezone into account
        t=time.mktime(l)-time.timezone
        self.date=time.strftime("%Y/%m/%d %H:%M:%S",time.localtime(t))

        #Get the current local date
        self.time = time.strftime("%Y/%m/%d %H:%M:%S")

        self.mods = 'Src not modified'
        self.mixed = 'Src not mixed'
        self.range = self.rev

        # if version string ends with an 'M'
        r=re.search("M$",Ver)
        if r != None:
            self.mods = 'Src modified'

        # if version string contains a range
        r=re.match("^\\d+\\:\\d+",Ver)
        if r != None:
            self.mixed = 'Src mixed'
            self.range = Ver[:r.end()]
        return True

    def printInfo(self):
        print "subversion"


def main():
    #if(len(sys.argv) != 2):
    #    sys.stderr.write("Usage:  SubWCRev \"`svn info .. --xml`\"\n")

    srcdir="."
    bindir="."
    try:
        opts, args = getopt.getopt(sys.argv[1:], "sb:", ["srcdir=","bindir="])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-s", "--srcdir"):
            srcdir = a
        if o in ("-b", "--bindir"):
            bindir = a

    vcs=[Subversion(), BazaarControl(), GitControl(), MercurialControl(), DebianChangelog(), UnknownControl()]
    for i in vcs:
        if i.extractInfo(srcdir):
            # Open the template file and the version file
            file = open("%s/src/Build/Version.h.in" % (srcdir))
            lines = file.readlines()
            file.close()
            lines = i.writeVersion(lines)
            out  = open("%s/src/Build/Version.h" % (bindir),"w");
            out.writelines(lines)
            out.write('\n')
            out.close()
            i.printInfo()
            sys.stdout.write("%s/src/Build/Version.h written\n" % (bindir))
            break

if __name__ == "__main__":
    main()

