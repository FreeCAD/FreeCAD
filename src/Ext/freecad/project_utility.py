# SPDX-License-Identifier: LGPL-2.1-or-later

# (c) 2023 Werner Mayer LGPL

__title__="Document handling module"
__author__ = "Werner Mayer"
__url__ = "https://www.freecad.org"
__doc__ = "Tools for extracting or creating project files"


import os
import xml.sax
import xml.sax.handler
import xml.sax.xmlreader
import zipfile
from defusedxml import sax as defused_sax

# SAX handler to parse the Document.xml
class DocumentHandler(xml.sax.handler.ContentHandler):
    """ Parse content of Document.xml or GuiDocument.xml """
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
    """ Extract files from project archive """
    outpath = os.path.abspath(outpath)
    with zipfile.ZipFile(filename) as zfile:
        for i in zfile.namelist():
            output_path = os.path.abspath(os.path.join(outpath, i))
            # Prevent Zip Slip path traversal from malicious project archive entries.
            if os.path.commonpath([outpath, output_path]) != outpath:
                raise ValueError("Archive entry outside extraction directory: {}".format(i))
            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            with open(output_path, 'wb') as output:
                output.write(zfile.read(i))

def createDocument(filename, outpath):
    """ Create project archive """
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
    """ Determine list of files referenced in a Document.xml or GuiDocument.xml """
    dirname = os.path.dirname(filename)
    handler = DocumentHandler(dirname)
    # Use defusedxml to block XML entity expansion and external entity attacks.
    parser = defused_sax.make_parser()
    parser.setContentHandler(handler)
    parser.parse(filename)

    files = []
    files.append(filename)
    files.extend(iter(handler.files))
    return files
