# SPDX-License-Identifier: LGPL-2.1-or-later

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
import ImportGui
import tempfile

___stpZversion___ = "1.4.0"
# support both gz and zipfile archives
# Catia seems to use gz, Inventor zipfile
# improved import, open and export


import gzip as gz
import builtins


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

    # Private directory avoids predictable-path link attacks (GHSA-78hj-4hh8-w6f9)
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as tempdir:
        tempfile_path = os.path.join(tempdir, fname + ".stp")
        with builtins.open(tempfile_path, "wb") as f:
            f.write(fc)
        if doc is None:
            ImportGui.open(tempfile_path)
        else:
            ImportGui.open(tempfile_path, doc.Name)
        FreeCADGui.ActiveDocument.ActiveView.sendMessage("ViewFit")


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
    outfpath = os.path.join(basepath, fname) + ".stpZ"

    # Private directory avoids predictable-path link attacks (GHSA-78hj-4hh8-w6f9)
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as tempdir:
        tempfile_path = os.path.join(tempdir, fname) + ".stp"
        ImportGui.export(objs, tempfile_path)
        with builtins.open(tempfile_path, "rb") as f_in:
            file_content = f_in.read()
        with gz.open(tempfile_path, "wb") as f_out:
            f_out.write(file_content)
        shutil.move(tempfile_path, outfpath)


####
