#! python
# -*- coding: utf-8 -*-
# (c) 2006 Werner Mayer LGPL
#
# Python script to make source tarballs.
#

import sys, os, getopt, tarfile, gzip, time, io, platform, shutil, subprocess


def main():
    bindir = "."
    major = "0"
    minor = "0"
    dfsg = False
    check = False
    wta = None
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            "sb:",
            ["srcdir=", "bindir=", "major=", "minor=", "dfsg", "check"],
        )
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-s", "--srcdir"):
            print("{} is deprecated -- ignoring".format(o))
        if o in ("-b", "--bindir"):
            bindir = a
        if o in ("--major"):
            major = a
        if o in ("--minor"):
            minor = a
        if o in ("--dfsg"):
            dfsg = True
            wta = "--worktree-attributes"
        if o in ("--check"):
            check = True

    if dfsg:
        gitattr = open("src/.gitattributes", "w")
        gitattr.write("zipios++    export-ignore\n")
        gitattr.write("Pivy-0.5    export-ignore\n")
        gitattr.write("Pivy    export-ignore\n")
        gitattr.write("3Dconnexion    export-ignore\n")
        gitattr.write("Kuka    export-ignore\n")
        gitattr.close()

    # revision number
    info = os.popen("git rev-list HEAD").read()
    revision = "%04d" % (info.count("\n"))

    verfile = open("{}/src/Build/Version.h".format(bindir), "rb")
    verstream = io.BytesIO(verfile.read())
    verfile.close()

    version_major = major
    version_minor = minor

    PACKAGE_NAME = "freecad"
    version = "{}.{}.{}".format(version_major, version_minor, revision)

    DIRNAME = "%(p)s-%(v)s" % {"p": PACKAGE_NAME, "v": version}
    TARNAME = DIRNAME + ".tar"
    TGZNAME = DIRNAME + ".tar.gz"
    if dfsg:
        TGZNAME = DIRNAME + "-dfsg.tar.gz"

    verinfo = tarfile.TarInfo(DIRNAME + "/src/Build/Version.h")
    verinfo.mode = 0o660
    verinfo.size = len(verstream.getvalue())
    verinfo.mtime = time.time()

    if wta is None:
        print(("git archive --prefix={}/ HEAD".format(DIRNAME)))
    else:
        print(("git archive {} --prefix={}/ HEAD".format(wta, DIRNAME)))

    if platform.system() == "Windows":
        os.popen(
            "git archive {} --prefix={}/ --output={} HEAD".format(wta, DIRNAME, TARNAME)
        ).read()

        tar = tarfile.TarFile(mode="a", name=TARNAME)
        tar.addfile(verinfo, verstream)
        tar.close()

        out = gzip.open(TGZNAME, "wb")
        tardata = open(TARNAME, "rb")
        out.write(tardata.read())
        out.close()
        tardata.close()
        os.remove(TARNAME)
    else:
        cmd_line = ["git", "archive"]
        if not wta is None:
            cmd_line.append(wta)
        cmd_line.append("--prefix={}/".format(DIRNAME))
        cmd_line.append("HEAD")

        tardata = subprocess.Popen(cmd_line, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = tardata.communicate()
        tarstream = io.BytesIO(out)

        tar = tarfile.TarFile(mode="a", fileobj=tarstream)
        tar.addfile(verinfo, verstream)
        tar.close()

        out = gzip.open(TGZNAME, "wb")
        out.write(tarstream.getvalue())
        out.close()

    if dfsg:
        os.remove("src/.gitattributes")
    print(("Created " + TGZNAME))
    # Unpack and build
    if check:
        archive = tarfile.open(mode="r:gz", name=TGZNAME)
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
