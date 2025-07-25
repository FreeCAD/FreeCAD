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
    except getopt.GetoptError as e:
        raise Exception(e)  # raise when error, instead of continue

    for o, a in opts:
        if o in ("-s", "--srcdir"):
            print(f"{o} is deprecated -- ignoring")
        if o in ("-b", "--bindir"):
            bindir = a
        if o in "--major":
            major = a
        if o in "--minor":
            minor = a
        if o in "--dfsg":
            dfsg = True
            wta = "--worktree-attributes"
        if o in "--check":
            check = True

    if dfsg:
        with open("src/.gitattributes", "w") as gitattr:
            for i in ["zipios++", "Pivy-0.5", "Pivy", "3Dconnexion", "Kuka"]:
                gitattr.write(f"{i}    export-ignore\n")

    # revision number
    info = os.popen("git rev-list HEAD").read()
    revision = "%04d" % info.count("\n")

    with open(f"{bindir}/src/Build/Version.h", "rb") as verfile:
        verstream = io.BytesIO(verfile.read())

    PACKAGE_NAME = "freecad"
    version = f"{major}.{minor}.{revision}"

    DIRNAME = f"{PACKAGE_NAME}-{version}"
    TARNAME = DIRNAME + ".tar"
    TGZNAME = DIRNAME + ".tar.gz"
    if dfsg:
        TGZNAME = DIRNAME + "-dfsg.tar.gz"

    verinfo = tarfile.TarInfo(DIRNAME + "/src/Build/Version.h")
    verinfo.mode = 0o660
    verinfo.size = len(verstream.getvalue())
    verinfo.mtime = int(time.time())  # should be int

    if wta is None:
        print(f"git archive --prefix={DIRNAME}/ HEAD")
    else:
        print(f"git archive {wta} --prefix={DIRNAME}/ HEAD")

    if platform.system() == "Windows":
        os.popen(f"git archive {wta} --prefix={DIRNAME}/ --output={TARNAME} HEAD").read()

        tar = tarfile.TarFile(mode="a", name=TARNAME)
        tar.addfile(verinfo, verstream)
        tar.close()

        out = gzip.open(TGZNAME, "wb")
        with open(TARNAME, "rb") as tardata:
            out.write(tardata.read())
        out.close()
        os.remove(TARNAME)
    else:
        cmd_line = ["git", "archive"]
        if wta:
            cmd_line.append(wta)
        cmd_line.append(f"--prefix={DIRNAME}/")
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
    print(f"Created {TGZNAME}")
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
