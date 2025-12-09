#! python
# SPDX-License-Identifier: LGPL-2.1-or-later

# (c) 2010 Werner Mayer LGPL
# FreeCAD Python script to work with the FCStd file format.

import os
from defusedxml import sax as defused_sax
from xml.sax.handler import ContentHandler
import zipfile


# SAX handler to parse the Document.xml
class DocumentHandler(ContentHandler):
    def __init__(self, dirname):
        super().__init__()
        self.files = []
        self.dirname = dirname

    def startElement(self, name, attributes):
        item = attributes.get("file")
        if item is not None:
            self.files.append(os.path.join(self.dirname, str(item)))

    def characters(self, data):
        return

    def endElement(self, name):
        return


def extractDocument(filename, outpath):
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
        output = open(outpath + "/" + i, "wb")
        output.write(data)
        output.close()


def createDocument(filename, outpath):
    files = getFilesList(filename)
    compress = zipfile.ZipFile(outpath, "w", zipfile.ZIP_DEFLATED)
    for i in files:
        dirs = os.path.split(i)
        # print i, dirs[-1]
        compress.write(i, dirs[-1], zipfile.ZIP_DEFLATED)
    compress.close()


def getFilesList(filename):
    dirname = os.path.dirname(filename)
    handler = DocumentHandler(dirname)
    parser = defused_sax.make_parser()
    parser.setContentHandler(handler)
    parser.parse(filename)

    files = []
    files.append(filename)
    files.extend(iter(handler.files))
    dirname = os.path.join(dirname, "GuiDocument.xml")
    if os.path.exists(dirname):
        files.append(dirname)
    return files
