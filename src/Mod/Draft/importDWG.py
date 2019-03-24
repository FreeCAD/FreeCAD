# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

"FreeCAD Draft Workbench - DWG importer/exporter"

## @package importDWG
#  \ingroup DRAFT
#  \brief DWG file importer & exporter
#
#  This module provides support for importing and exporting Autodesk DWG files.
#  This module is only a thin layer that uses the ODA (formerly Teigha) File
#  Converter application to convert to/from DXF. Then the real work is done by
#  importDXF

import six

if open.__module__ == '__builtin__':
    pythonopen = open # to distinguish python built-in open function from the one declared here

def open(filename):
    "called when freecad opens a file."
    dxf = convertToDxf(filename)
    if dxf:
        import importDXF
        doc = importDXF.open(dxf)
        return doc
    return

def insert(filename,docname):
    "called when freecad imports a file"
    dxf = convertToDxf(filename)
    if dxf:
        import importDXF
        doc = importDXF.insert(dxf,docname)
        return doc
    return

def export(objectslist,filename):
    "called when freecad exports a file"
    import importDXF,os,tempfile
    outdir = tempfile.mkdtemp()
    dxf = outdir + os.sep + os.path.splitext(os.path.basename(filename))[0] + ".dxf"
    importDXF.export(objectslist,dxf)
    convertToDwg(dxf,filename)
    return filename

def getTeighaConverter():
    import FreeCAD,os,platform
    "finds the Teigha Converter executable"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    p = p.GetString("TeighaFileConverter")
    if p:
        # path set manually
        teigha = p
    else:
        # try to find teigha
        teigha = None
        if platform.system() == "Linux":
            teigha = "/usr/bin/TeighaFileConverter"
        elif platform.system() == "Windows":
            odadir = os.path.expandvars("%ProgramFiles%\ODA")
            if os.path.exists(odadir):
                subdirs = os.walk(odadir).next()[1]
                for sub in subdirs:
                    t = odadir + os.sep + sub + os.sep + "TeighaFileConverter.exe"
                    if os.path.exists(t):
                        teigha = t
    if teigha:
        if os.path.exists(teigha):
            return teigha
    from DraftTools import translate
    FreeCAD.Console.PrintMessage(translate("draft","ODA (formerly Teigha) File Converter not found, DWG support is disabled")+"\n")
    return None

def convertToDxf(dwgfilename):
    "converts a DWG file to DXF"
    import os,tempfile,subprocess,sys     #import os,tempfile
    teigha = getTeighaConverter()
    if teigha:
        indir = os.path.dirname(dwgfilename)
        outdir = tempfile.mkdtemp()
        basename = os.path.basename(dwgfilename)
        cmdline = '"%s" "%s" "%s" "ACAD2000" "DXF" "0" "1" "%s"' % (teigha, indir, outdir, basename)
        print("Converting: " + cmdline)
        if six.PY2:
            if isinstance(cmdline,six.text_type):
                encoding = sys.getfilesystemencoding()
                cmdline = cmdline.encode(encoding)
        subprocess.call(cmdline,  shell=True)     #os.system(cmdline)
        result = outdir + os.sep + os.path.splitext(basename)[0] + ".dxf"
        if os.path.exists(result):
            print("Conversion successful")
            return result
        else:
            print("Error during DWG to DXF conversion. Try moving the DWG file to a directory path")
            print("without spaces and non-english characters, or try saving to a lower DWG version")
    return None

def convertToDwg(dxffilename,dwgfilename):
    "converts a DXF file to DWG"
    import os,subprocess     #import os
    teigha = getTeighaConverter()
    if teigha:
        indir = os.path.dirname(dxffilename)
        outdir = os.path.dirname(dwgfilename)
        basename = os.path.basename(dxffilename)
        cmdline = '"%s" "%s" "%s" "ACAD2000" "DWG" "0" "1" "%s"' % (teigha, indir, outdir, basename)
        print("converting " + cmdline)
        subprocess.call(cmdline,  shell=True)     #os.system(cmdline)
        return dwgfilename
    return None
