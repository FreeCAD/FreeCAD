# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   Copyright (c) 2022 FreeCAD Project Association                             #
#   Copyright (c) 2023 Werner Mayer                                            #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Document handling module.

Tools for extracting or creating project files.
"""


import os
import xml.sax
import xml.sax.handler
import zipfile

# SAX handler to parse the Document.xml
class DocumentHandler(xml.sax.handler.ContentHandler):
    """Parse content of Document.xml or GuiDocument.xml."""

    def __init__(self, dirname):
        """ Init parser """
        super().__init__()
        self.files = []
        self.dirname = dirname

    def startElement(self, name, attrs):
        item = attrs.get("file")
        if item is not None:
            self.files.append(os.path.join(self.dirname, str(item)))

    def characters(self, content):
        return

    def endElement(self, name):
        return

def extractDocument(filename, outpath):
    """Extract files from project archive."""

    zfile = zipfile.ZipFile(filename)
    files = zfile.namelist()

    for i in files:
        data = zfile.read(i)
        dirs = i.split("/")
        if len(dirs) > 1:
            dirs.pop()
            curpath = outpath
            for j in dirs:
                curpath = curpath + "/" + j
                os.mkdir(curpath)
        output = open(outpath + "/"+i, 'wb')
        output.write(data)
        output.close()

def createDocument(filename, outpath):
    """Create project archive."""
    files = getFilesList(filename)
    dirname = os.path.dirname(filename)
    guixml = os.path.join(dirname, "GuiDocument.xml")
    if os.path.exists(guixml):
        files.extend(getFilesList(guixml))
    compress = zipfile.ZipFile(outpath, 'w', zipfile.ZIP_DEFLATED)
    for file in files:
        if os.path.isfile(file):
            path_in_archive = os.path.relpath(path=file, start=dirname)
            compress.write(file, path_in_archive, zipfile.ZIP_DEFLATED)
    compress.close()

def getFilesList(filename):
    """Determine list of files referenced in a Document.xml or GuiDocument.xml."""
    dirname = os.path.dirname(filename)
    handler = DocumentHandler(dirname)
    parser = xml.sax.make_parser()
    parser.setContentHandler(handler)
    parser.parse(filename)

    files = []
    files.append(filename)
    files.extend(iter(handler.files))
    return files
