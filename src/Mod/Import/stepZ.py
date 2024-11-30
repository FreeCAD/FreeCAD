# -*- coding: utf-8 -*-
# ****************************************************************************
# *  Copyright (c) 2018 Maurice <easyw@katamail.com>                         *
# *                                                                          *
# *  StepZ Import Export compressed STEP files for FreeCAD                   *
# *  License: LGPLv2+                                                        *
# *                                                                          *
# ****************************************************************************

# workaround for unicode in gzipping filename
# OCC7 doesn't support non-ASCII characters at the moment
# https://forum.freecad.org/viewtopic.php?t=20815


import FreeCAD
import FreeCADGui
import shutil
import os
import re
import ImportGui
import PySide
from PySide import QtCore
from PySide import QtGui
import tempfile

___stpZversion___ = "1.4.0"
# support both gz and zipfile archives
# Catia seems to use gz, Inventor zipfile
# improved import, open and export


import gzip as gz
import builtins
import importlib


import zipfile as zf

# import stepZ; import importlib; importlib.reload(stepZ); stepZ.open(u"C:/Temp/brick.stpz")


def mkz_string(input):
    if isinstance(input, str):
        return input
    else:
        input = input.encode("utf-8")
        return input


####
def mkz_unicode(input):
    if isinstance(input, str):
        return input
    else:
        input = input.decode("utf-8")
        return input


####
def sayz(msg):
    FreeCAD.Console.PrintMessage(msg)
    FreeCAD.Console.PrintMessage("\n")


####
def sayzw(msg):
    FreeCAD.Console.PrintWarning(msg)
    FreeCAD.Console.PrintWarning("\n")


####
def sayzerr(msg):
    FreeCAD.Console.PrintError(msg)
    FreeCAD.Console.PrintWarning("\n")


####


def import_stpz(fn, fc, doc):

    # sayz(fn)
    ext = os.path.splitext(os.path.basename(fn))[1]
    fname = os.path.splitext(os.path.basename(fn))[0]
    basepath = os.path.split(fn)[0]
    filepath = os.path.join(basepath, fname + ".stp")

    tempdir = tempfile.gettempdir()  # get the current temporary directory
    tempfilepath = os.path.join(tempdir, fname + ".stp")

    with builtins.open(tempfilepath, "wb") as f:  # py3
        f.write(fc)
    # ImportGui.insert(filepath)
    if doc is None:
        ImportGui.open(tempfilepath)
    else:
        ImportGui.open(tempfilepath, doc.Name)
    FreeCADGui.SendMsgToActiveView("ViewFit")
    try:
        os.remove(tempfilepath)
    except OSError:
        sayzerr("error on removing " + tempfilepath + " file")


###


def open(filename, doc=None):

    if zf.is_zipfile(filename):
        with zf.ZipFile(filename, "r") as fz:
            file_names = fz.namelist()
            for fn in file_names:
                sayz(fn)
                with fz.open(fn) as zfile:
                    file_content = zfile.read()
                    import_stpz(filename, file_content, doc)
    else:
        with gz.open(filename, "rb") as f:
            fnm = os.path.splitext(os.path.basename(filename))[0]
            sayz(fnm)
            file_content = f.read()
            import_stpz(filename, file_content, doc)


####


def insert(filename, doc):

    doc = FreeCAD.ActiveDocument
    open(filename, doc)


####


def export(objs, filename):
    """exporting to file folder"""

    # sayz(filename)
    sayz("stpZ version " + ___stpZversion___)
    ext = os.path.splitext(os.path.basename(filename))[1]
    fname = os.path.splitext(os.path.basename(filename))[0]
    basepath = os.path.split(filename)[0]
    tempdir = tempfile.gettempdir()  # get the current temporary directory

    filepath = os.path.join(basepath, fname) + ".stp"
    filepath_base = os.path.join(basepath, fname)

    namefpath = os.path.join(basepath, fname)

    outfpath = os.path.join(basepath, fname) + ".stpZ"
    outfpathT = os.path.join(tempdir, fname) + ".stpZ"
    outfpath_stp = os.path.join(basepath, fname) + ".stp"
    outfpathT_stp = os.path.join(tempdir, fname) + ".stp"

    outfpath_base = basepath
    # outfpath_str = mkz_string(os.path.join(basepath,fname))
    outfpath_str = os.path.join(basepath, fname) + ".stp"
    outfpathT_str = os.path.join(tempdir, fname) + ".stp"

    if os.path.exists(outfpathT_stp):
        os.remove(outfpathT_stp)
        sayzw("Old temp file with the same name removed '" + outfpathT_stp + "'")
    ImportGui.export(objs, outfpathT_stp)
    with builtins.open(outfpathT_stp, "rb") as f_in:
        file_content = f_in.read()
        new_f_content = file_content
        f_in.close()
    with gz.open(outfpathT_str, "wb") as f_out:
        f_out.write(new_f_content)
        f_out.close()
    if os.path.exists(outfpath):
        shutil.move(outfpathT_str, outfpath)
        # os.remove(outfpathT_stp)
    else:
        shutil.move(outfpathT_str, outfpath)
        # os.remove(outfpathT_stp)


####
