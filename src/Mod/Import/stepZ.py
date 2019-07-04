# -*- coding: utf-8 -*-
#****************************************************************************
#*                                                                          *
#*  StepZ Import Export compressed STEP files for FreeCAD                   *
#*  Copyright (c) 2018                                                      *
#*  Maurice easyw@katamail.com                                              *
#*                                                                          *
#*  License: LGPLv2+                                                        *

# workaround for unicode in gzipping filename
# OCC7 doesn't support non-ASCII characters at the moment
# https://forum.freecadweb.org/viewtopic.php?t=20815

import six

import FreeCAD,FreeCADGui
import shutil
import os, re
import ImportGui
import PySide
from PySide import QtGui, QtCore
import tempfile

___stpZversion___ = "1.3.7"
# support both gz and zipfile archives
# Catia seems to use gz, Inventor zipfile
# improved import, open and export

if six.PY3:
    import gzip as gz
    import builtins as builtin  #py3
    import importlib
    
else:  # six.PY2
    import gzip_utf8 as gz
    import __builtin__ as builtin #py2

import zipfile  as zf

# import stepZ; import importlib; importlib.reload(stepZ); stepZ.open(u"C:/Temp/brick.stpz")
# import stepZ; import importlib; importlib.reload(stepZ); import gzip_utf8; importlib.reload(gzip_utf8)

def mkz_string(input):
    if six.PY3:
        if isinstance(input, str):
            return input
        else:
            input = input.encode('utf-8')
            return input
    else:  # six.PY2
        if isinstance(input, six.text_type):
            input = input.encode('utf-8')
            return input
        else:
            return input
####
def mkz_unicode(input):
    if six.PY3:
        if isinstance(input, str):
            return input
        else:
            input = input.decode('utf-8')
            return input
    else:  # six.PY2
        if isinstance(input, six.text_type):
            input = input.decode('utf-8')
            return input
        else:
            return input
####
def sayz(msg):
    FreeCAD.Console.PrintMessage(msg)
    FreeCAD.Console.PrintMessage('\n')
####
def sayzw(msg):
    FreeCAD.Console.PrintWarning(msg)
    FreeCAD.Console.PrintWarning('\n')
####
def sayzerr(msg):
    FreeCAD.Console.PrintError(msg)
    FreeCAD.Console.PrintWarning('\n')
####

def import_stpz(fn,fc,doc):

    # sayz(fn)
    ext = os.path.splitext(os.path.basename(fn))[1]
    fname=os.path.splitext(os.path.basename(fn))[0]
    basepath=os.path.split(fn)[0]
    filepath = os.path.join(basepath,fname + u'.stp')
    
    tempdir = tempfile.gettempdir() # get the current temporary directory
    tempfilepath = os.path.join(tempdir,fname + u'.stp')

    #with six.builtins.open(tempfilepath, 'wb') as f: #py3
    with builtin.open(tempfilepath, 'wb') as f: #py3
        f.write(fc)
    #ImportGui.insert(filepath)
    if doc is None:
        ImportGui.open(tempfilepath)
    else:
        ImportGui.open(tempfilepath,doc.Name)
    FreeCADGui.SendMsgToActiveView("ViewFit")
    try:
        os.remove(tempfilepath)
    except OSError:
        sayzerr("error on removing "+tempfilepath+" file")
        pass
###

def open(filename,doc=None):

    sayz("stpZ version "+___stpZversion___)
        
    if zf.is_zipfile(filename):
        with zf.ZipFile(filename, 'r') as fz:
            file_names = fz.namelist()
            for fn in file_names:
                sayz(fn)
                with fz.open(fn) as zfile:
                    file_content = zfile.read()
                    import_stpz(filename,file_content,doc)
    else:
        with gz.open(filename, 'rb') as f:
            fnm=os.path.splitext(os.path.basename(filename))[0]
            sayz(fnm)
            file_content = f.read()
            import_stpz(filename,file_content,doc)
####

def insert(filename,doc):

    doc = FreeCAD.ActiveDocument
    open(filename, doc)
    sayz("stpZ version "+___stpZversion___)
        
####

def export(objs,filename):
    """exporting to file folder"""
    
    #sayz(filename)
    sayz("stpZ version "+___stpZversion___)
    ext = os.path.splitext(os.path.basename(filename))[1]
    fname=os.path.splitext(os.path.basename(filename))[0]
    basepath=os.path.split(filename)[0]
    tempdir = tempfile.gettempdir() # get the current temporary directory
    
    filepath = os.path.join(basepath,fname) + u'.stp'
    filepath_base  = os.path.join(basepath,fname)
        
    namefpath = os.path.join(basepath,fname)
        
    outfpath = os.path.join(basepath,fname)+u'.stpZ'
    outfpath_stp = os.path.join(basepath,fname)+u'.stp'
    outfpath_base = basepath
    #outfpath_str = mkz_string(os.path.join(basepath,fname))
    outfpath_str = os.path.join(basepath,fname)+u'.stp'
    
        
    if os.path.exists(outfpath_stp):
        sayzw("File cannot be compressed because a file with the same name exists '"+ outfpath_stp +"'")
        QtGui.QApplication.restoreOverrideCursor()
        reply = QtGui.QMessageBox.information(None,"info", "File cannot be compressed because\na file with the same name exists\n'"+ outfpath_stp + "'")
    else:    
        ImportGui.export(objs,outfpath_stp)
        with builtin.open(outfpath_stp, 'rb') as f_in:
            file_content = f_in.read()
            new_f_content = file_content
            f_in.close()
        with gz.open(outfpath_str, 'wb') as f_out:
            f_out.write(new_f_content)
            f_out.close()    
        if os.path.exists(outfpath):
            os.remove(outfpath)
            os.rename(outfpath_str, outfpath)  
            #os.remove(outfpath_stp)
        else:
            os.rename(outfpath_str, outfpath)
            #os.remove(outfpath_stp)                

####

