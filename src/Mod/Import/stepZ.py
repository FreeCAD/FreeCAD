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

import FreeCAD,FreeCADGui
import gzip_utf8, shutil
import sys, os, re
import ImportGui
import PySide
from PySide import QtGui, QtCore
import tempfile

___stpZversion___ = "1.3.1"

try:
    import __builtin__ as builtin #py2
except:
    import builtins as builtin  #py3

# import stepZ; reload(stepZ); import gzip_utf8; reload(gzip_utf8)

def mkz_string(input):
    if (sys.version_info > (3, 0)):  #py3
        if isinstance(input, str):
            return input
        else:
            input =  input.encode('utf-8')
            return input
    else:  #py2
        if type(input) == unicode:
            input =  input.encode('utf-8')
            return input
        else:
            return input
####
def mkz_unicode(input):
    if (sys.version_info > (3, 0)):  #py3
        if isinstance(input, str):
            return input
        else:
            input =  input.decode('utf-8')
            return input
    else: #py2
        if type(input) != unicode:
            input =  input.decode('utf-8')
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
def open(filename):

    sayz("stpZ version "+___stpZversion___)
    with gzip_utf8.open(filename, 'rb') as f:
        file_content = f.read()

    ext = os.path.splitext(os.path.basename(filename))[1]
    fname=os.path.splitext(os.path.basename(filename))[0]
    basepath=os.path.split(filename)[0]
    filepath = os.path.join(basepath,fname + u'.stp')

    tempdir = tempfile.gettempdir() # get the current temporary directory
    tempfilepath = os.path.join(tempdir,fname + u'.stp')

    with builtin.open(tempfilepath, 'w') as f: #py3
        f.write(file_content)
    #ImportGui.insert(filepath)
    ImportGui.open(tempfilepath)
    try:
        os.remove(tempfilepath)
    except OSError:
        sayzerr("error on removing "+tempfilepath+" file")
        pass
####

def insert(filename,doc):

    sayz("stpZ version "+___stpZversion___)
    with gzip_utf8.open(filename, 'rb') as f:
        file_content = f.read()

    ext = os.path.splitext(os.path.basename(filename))[1]
    fname=os.path.splitext(os.path.basename(filename))[0]
    basepath=os.path.split(filename)[0]
    filepath = os.path.join(basepath,fname + u'.stp')

    tempdir = tempfile.gettempdir() # get the current temporary directory
    tempfilepath = os.path.join(tempdir,fname + u'.stp')
    
    with builtin.open(tempfilepath, 'w') as f: #py3
        f.write(file_content)
    ImportGui.insert(tempfilepath, doc)
    #ImportGui.open(tempfilepath)
    try:
        os.remove(tempfilepath)
    except OSError:
        sayzerr("error on removing "+tempfilepath+" file")
        pass
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
    outfpath_str = os.path.join(basepath,fname)
    
        
    if os.path.exists(outfpath_stp):
        sayzw("File cannot be compressed because a file with the same name exists '"+ outfpath_stp +"'")
        QtGui.qApp.restoreOverrideCursor()
        reply = QtGui.QMessageBox.information(None,"info", "File cannot be compressed because\na file with the same name exists\n'"+ outfpath_stp + "'")
    else:    
        ImportGui.export(objs,outfpath_stp)
        if 0: #os.path.exists(namefpath):
            sayzw("File cannot be compressed because a file with the same name exists '" + namefpath + "'")
            QtGui.qApp.restoreOverrideCursor()
            reply = QtGui.QMessageBox.information(None,"info", "File cannot be compressed because\na file with the same name exists\n'"+ namefpath+ "'")
        else:
            with builtin.open(outfpath_stp, 'rb') as f_in:
                file_content = f_in.read()
                new_f_content = file_content
                f_in.close()
            with gzip_utf8.open(outfpath_str, 'wb') as f_out:
                f_out.write(new_f_content)
                f_out.close()    
            if os.path.exists(outfpath):
                os.remove(outfpath)
                os.rename(outfpath_str, outfpath)  
                os.remove(outfpath_stp)
            else:
                os.rename(outfpath_str, outfpath)
                os.remove(outfpath_stp)                

####

