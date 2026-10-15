#! python
# SPDX-License-Identifier: LGPL-2.1-or-later

###
#  A convenience script to generate a deployment archive name of the form
#  FreeCAD_{Major Version Number}.{Minor Version Number}-{Git Revision Count}.{Git Short SHA}-{OS}-{Arch}
#
import sys, getopt, platform


def deserializeVersionFile(path):
    version = {}
    try:
        dat = open(path, "r").readlines()
    except IOError:
        print("Unable to open ", path)
        raise

    for line in dat:
        key, separator, value = line.rstrip("\r\n").partition("=")
        if separator:
            version[key] = value

    return version


def main():
    OSAbbrev = {"Windows": "WIN", "Darwin": "OSX"}
    SHA = None

    if len(sys.argv) < 2:
        sys.stderr.write(
            "Usage:  archiveNameFromVersion <path to FreeCADVersion.txt> [--git-SHA=]\n"
        )

    try:
        opts, args = getopt.getopt(sys.argv[2:], "g:", ["git-SHA="])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-g", "--git-SHA"):
            SHA = a

    version = deserializeVersionFile(sys.argv[1])
    if SHA:
        version["FCRepositoryHash"] = SHA

    print(
        "FreeCAD_{Major}.{Minor}-{RevCount}.{GitShortSHA}-{OS}-{Arch}".format(
            Major=version["FCVersionMajor"],
            Minor=version["FCVersionMinor"],
            RevCount=version["FCRevision"].split()[0],
            GitShortSHA=version["FCRepositoryHash"][0:7],
            OS=OSAbbrev.get(platform.system(), "LIN"),
            Arch=platform.machine(),
        )
    )


if __name__ == "__main__":
    main()
