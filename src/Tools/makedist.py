#! python
# -*- coding: utf-8 -*-
# (c) 2006 Werner Mayer LGPL
#
# Python script to make source tarballs.
#

import sys, os, getopt, tarfile, gzip, time, StringIO

def main():
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

    # revision number
    info=os.popen("git rev-list HEAD").read()
    revision='%04d' % (info.count('\n'))

    PACKAGE_NAME = 'freecad'
    version = "0.13.%s" % (revision)

    DIRNAME = "%(p)s-%(v)s" % {'p': PACKAGE_NAME, 'v': version}
    TARNAME = DIRNAME + '.tar.gz'

    verfile = open("%s/src/Build/Version.h" % (bindir), 'r')
    verstream = StringIO.StringIO(verfile.read())
    verfile.close()
    verinfo = tarfile.TarInfo(DIRNAME + "/src/Build/Version.h")
    verinfo.mode = 0660
    verinfo.size = len(verstream.getvalue())
    verinfo.mtime = time.time()

    print "git archive --worktree-attributes --prefix=%s/ HEAD" % (DIRNAME)
    tardata = os.popen("git archive --worktree-attributes --prefix=%s/ HEAD"
                            % (DIRNAME)).read()
    tarstream = StringIO.StringIO(tardata)

    tar = tarfile.TarFile(mode="a", fileobj=tarstream)
    tar.addfile(verinfo, verstream)
    tar.close()

    out = gzip.open(TARNAME, "wb")
    out.write(tarstream.getvalue())
    out.close()
    print "Created " + TARNAME

if __name__ == "__main__":
    main()
