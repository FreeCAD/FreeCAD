#! python
# -*- coding: utf-8 -*-
# (c) 2006 Werner Mayer LGPL
#
# Python script to make source tarballs.
#

import sys, os, getopt, tarfile, gzip, time, StringIO, platform, shutil

def main():
    srcdir="."
    bindir="."
    dfsg=False
    check=False
    wta=""
    try:
        opts, args = getopt.getopt(sys.argv[1:], "sb:", ["srcdir=","bindir=","dfsg", "check"])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-s", "--srcdir"):
            srcdir = a
        if o in ("-b", "--bindir"):
            bindir = a
        if o in ("--dfsg"):
            dfsg = True
            wta = "--worktree-attributes"
        if o in ("--check"):
            check = True
            
    if dfsg:
        gitattr = open("src/.gitattributes","w")
        gitattr.write("zipios++    export-ignore\n")
        gitattr.write("Pivy-0.5    export-ignore\n")
        gitattr.write("Pivy    export-ignore\n")
        gitattr.write("3Dconnexion    export-ignore\n")
        gitattr.write("Kuka    export-ignore\n")
        gitattr.close()

    # revision number
    info=os.popen("git rev-list HEAD").read()
    revision='%04d' % (info.count('\n'))

    PACKAGE_NAME = 'freecad'
    version = "0.13.%s" % (revision)

    DIRNAME = "%(p)s-%(v)s" % {'p': PACKAGE_NAME, 'v': version}
    TARNAME = DIRNAME + '.tar'
    TGZNAME = DIRNAME + '.tar.gz'
    if dfsg:
        TGZNAME = DIRNAME + '-dfsg.tar.gz'

    verfile = open("%s/src/Build/Version.h" % (bindir), 'r')
    verstream = StringIO.StringIO(verfile.read())
    verfile.close()
    verinfo = tarfile.TarInfo(DIRNAME + "/src/Build/Version.h")
    verinfo.mode = 0660
    verinfo.size = len(verstream.getvalue())
    verinfo.mtime = time.time()

    print "git archive %s --prefix=%s/ HEAD" % (wta, DIRNAME)
    if platform.system() == 'Windows':
        os.popen("git archive %s --prefix=%s/ --output=%s HEAD"
                                % (wta, DIRNAME, TARNAME)).read()

        tar = tarfile.TarFile(mode="a", name=TARNAME)
        tar.addfile(verinfo, verstream)
        tar.close()

        out = gzip.open(TGZNAME, "wb")
        tardata = open(TARNAME, 'rb')
        out.write(tardata.read())
        out.close()
        tardata.close()
        os.remove(TARNAME)
    else:
        tardata = os.popen("git archive %s --prefix=%s/ HEAD"
                                % (wta, DIRNAME)).read()
        tarstream = StringIO.StringIO(tardata)

        tar = tarfile.TarFile(mode="a", fileobj=tarstream)
        tar.addfile(verinfo, verstream)
        tar.close()

        out = gzip.open(TGZNAME, "wb")
        out.write(tarstream.getvalue())
        out.close()
        
    if dfsg:
        os.remove("src/.gitattributes")
    print "Created " + TGZNAME
    # Unpack and build
    if check:
        archive=tarfile.open(mode='r:gz',name=TGZNAME)
        archive.extractall(bindir)
        builddir = os.path.join(bindir, DIRNAME)
        cwd = os.getcwd()
        os.chdir(builddir)
        os.system("cmake .")
        os.system("make")
        os.chdir(cwd)
        shutil.rmtree(builddir)

if __name__ == "__main__":
    main()
