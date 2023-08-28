#! python
###
#  A convenience script to generate a deployment archive name of the form
#  FreeCAD_{Major Version Number}.{Minor Version Number}-{Git Revision Count}.{Git Short SHA}-{OS}-{Arch}
#
import sys, getopt, platform


def deserializeVersionHeader(path):
    version = {}
    try:
        dat = open(path, "r").readlines()
    except IOError:
        print("Unable to open ", path)
        raise

    for l in dat:
        tokens = l.split()
        if len(tokens) > 1 and tokens[0].lower() == "#define":
            version[tokens[1]] = tokens[2].replace('"', "")

    return version


def main():
    OSAbbrev = {"Windows": "WIN", "Darwin": "OSX"}
    SHA = None

    if len(sys.argv) < 2:
        sys.stderr.write("Usage:  archiveNameFromVersion <path to Version.h> [--git-SHA=]\n")

    try:
        opts, args = getopt.getopt(sys.argv[2:], "g:", ["git-SHA="])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-g", "--git-SHA"):
            SHA = a

    version = deserializeVersionHeader(sys.argv[1])
    if SHA:
        version["FCRepositoryHash"] = SHA

    print(
        "FreeCAD_{Major}.{Minor}-{RevCount}.{GitShortSHA}-{OS}-{Arch}".format(
            Major=version["FCVersionMajor"],
            Minor=version["FCVersionMinor"],
            RevCount=version["FCRevision"],
            GitShortSHA=version["FCRepositoryHash"][0:7],
            OS=OSAbbrev.get(platform.system(), "LIN"),
            Arch=platform.machine(),
        )
    )


if __name__ == "__main__":
    main()
