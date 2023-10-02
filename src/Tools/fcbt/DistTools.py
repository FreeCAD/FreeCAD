# shell and operating system
import os

verbose = 0
dcount = fcount = 0
maxfileload = 100000
blksize = 1024 * 8


def BuildDistName():
    # Building dist name
    # reading the last Version information
    [FCVersionMajor, FCVersionMinor, FCVersionBuild, FCVersionDisDa] = open(
        "../Version.h", "r"
    ).readlines()
    DistName = (
        "FreeCAD_V"
        + FCVersionMajor[23:-1]
        + "."
        + FCVersionMinor[23:-1]
        + "B"
        + FCVersionBuild[23:-1]
    )

    return DistName


def BuildSetupName():
    # Building dist name
    # reading the last Version information
    [FCVersionMajor, FCVersionMinor, FCVersionBuild, FCVersionDisDa] = open(
        "../Version.h", "r"
    ).readlines()
    DistName = "FreeCAD_V" + FCVersionMajor[23:-1] + "." + FCVersionMinor[23:-1]

    return DistName


def GetVersion():
    # Building dist name
    # reading the last Version information
    [FCVersionMajor, FCVersionMinor, FCVersionBuild, FCVersionDisDa] = open(
        "../Version.h", "r"
    ).readlines()
    return FCVersionMajor[23:-1] + "." + FCVersionMinor[23:-1]


def GetBuildNbr():
    # Building dist name
    # reading the last Version information
    [FCVersionMajor, FCVersionMinor, FCVersionBuild, FCVersionDisDa] = open(
        "../Version.h", "r"
    ).readlines()
    return FCVersionBuild[23:-1]


def GetBuildDate():
    # Building dist name
    # reading the last Version information
    [FCVersionMajor, FCVersionMinor, FCVersionBuild, FCVersionDisDa] = open(
        "../Version.h", "r"
    ).readlines()
    return FCVersionDisDa[23:-1]


def EnsureDir(name):
    if not os.path.isdir(name):
        os.mkdir(name)
        return 0
    else:
        return 1


SrcFilter = [
    "^.*\\.o$",
    "^Debug$",
    "^DebugCmd$",
    "^DebugPy$",
    "^Release$",
    "^ReleaseCmd$",
    "^ReleasePy$",
    "^Attic$",
    "^CVS$",
    "^moc_\\.*$",
    "^.*\\.opt$",
    "^.*\\.ilg$",
    "^.*\\.ps$",
    "^.*\\.ind$",
    "^.*\\.idx$",
    "^.*\\.doc$",
    "^.*\\.dvi$",
    "^.*\\.ncb$",
    "^.*\\.aux$",
    "^.*\\.pdf$",
    "^.*\\.toc$",
    "^.*\\.exe$",
    "^.*\\.png$",
    "^.*\\.bak$",
    "^.*\\.pyc$",
    "^.*\\.dep$",
    "^.*\\.log$",
    "^.*\\.pyd$",
    "^.*\\.ilk$",
    "^.*\\.lib$",
    "^.*\\.pdb$",
    "^.*\\.exp$",
    "^.*\\.bsc$",
    "^.*\\.plg$",
]

BinFilter = [
    "^Plugin\\.*$",
    "^Standard\\.*$",
    "^.*\\.xml$",
    "^.*\\.log$",
    "^.*\\.pdb$",
    "^.*\\.ilk$",
    "^.*\\.lib$",
    "^.*\\.exp$",
    "^.*\\.bsc$",
    "^.*CADD.exe$",
    "^.*CADAppD.dll$",
    "^.*CmdD.exe$",
    "^.*BaseD.dll$",
    "^.*CADDCmdPy.dll$",
    "^.*GuiD.dll$",
    "^.*\\.bsc$",
    "^.*\\.FCScript\\..*$",
    "^.*\\.FCParam$",
    "^.*\\.FCScript$",
]

LibFilter = [
    "^Plugin\\.*$",
    "^Standard\\.*$",
    "^.*\\.xml$",
    "^.*\\.log$",
    "^.*\\.pdb$",
    "^.*\\.ilk$",
    "^.*\\.exe$",
    "^.*\\.exp$",
    "^.*\\.bsc$",
    "^.*CADD.lib$",
    "^.*CADAppD.lib$",
    "^.*CmdD.lib$",
    "^.*BaseD.lib$",
    "^.*GuiD.lib$",
    "^.*\\.FCScript\\..*$",
    "^.*\\.FCParam$",
]

LibPackFilter = ["^.*\\.o$", "^Debug$"]

ModFilter = [
    "^.*\\.o$",
    "^Debug$",
    "^DebugCmd$",
    "^DebugPy$",
    "^Release$",
    "^ReleaseCmd$",
    "^App$",
    "^Gui$",
    "^CVS$",
    "^Attic$",
    "^.*\\.opt$",
    "^.*_d\.pyd$",
    "^.*\\.opt$",
    "^.*\\.ilg$",
    "^.*\\.ps$",
    "^.*\\.ind$",
    "^.*\\.idx$",
    "^.*\\.doc$",
    "^.*\\.dvi$",
    "^.*\\.ncb$",
    "^.*\\.aux$",
    "^.*\\.pdf$",
    "^.*\\.toc$",
    "^.*\\.bak$",
    "^.*\\.pyc$",
    "^.*\\.dep$",
    "^.*\\.log$",
    "^.*\\.ilk$",
    "^.*\\.pdb$",
    "^.*\\.exp$",
    "^.*\\.lib$",
    "^.*\\.ui$",
    "^.*Makefile$",
    "^.*\\.plg$",
]

DocFilter = ["^.*\\.o$", "^Debug$"]
