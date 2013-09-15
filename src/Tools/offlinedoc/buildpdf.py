#!/usr/bin/env python

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

__title__="buildpdf"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script builds a pdf file from a local copy of the wiki
"""

import sys, os, re, tempfile, getopt
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
REMOVE = True # if true, the temp html files are removed after successful operation
PDFCONVERTOR = 'wkhtmltopdf' # can be 'pisa', 'htmldoc' or 'wkhtmltopdf'
VERBOSE = True

#    END CONFIGURATION      ##############################################


FOLDER = "./localwiki"

fcount = dcount = 0

def crawl():
    "downloads an entire wiki site"

    # tests ###############################################
    
    if PDFCONVERTOR == 'pisa':
        try:
            import ho.pisa as pisa
        except:
            "Error: Python-pisa not installed, exiting."
            return 1
    elif PDFCONVERTOR == 'htmldoc':
        if os.system('htmldoc --version'):
            print "Error: Htmldoc not found, exiting."
            return 1
    try:
        from pyPdf import PdfFileReader,PdfFileWriter
    except:
        print "Error: Python-pypdf not installed, exiting."

    # run ########################################################
    
    buildpdffiles()
    joinpdf()

    if VERBOSE: print "All done!"
    return 0

def buildpdffiles():
    "scans a folder for html files and converts them all to pdf"
    templist = os.listdir(FOLDER)
    fileslist = []
    for i in templist:
        if i[-5:] == '.html':
            fileslist.append(i)
    for f in fileslist:
        if PDFCONVERTOR == 'pisa': createpdf_pisa(f[:-5])
        elif PDFCONVERTOR == 'wkhtmltopdf': createpdf_wkhtmltopdf(f[:-5])
        else: createpdf_htmldoc(f[:-5])

def fetch_resources(uri, rel):
        """
        Callback to allow pisa/reportlab to retrieve Images,Stylesheets, etc.
        'uri' is the href attribute from the html link element.
        'rel' gives a relative path, but it's not used here.

        Note from Yorik: Not working!!
        """
        path = os.path.join(FOLDER,uri.replace("./", ""))
        return path

def createpdf_pisa(pagename):
    "creates a pdf file from a saved page using pisa (python module)"
    import ho.pisa as pisa
    if not exists(pagename+".pdf",image=True):
        infile = file(FOLDER + os.sep + pagename+'.html','ro')
        outfile = file(FOLDER + os.sep + pagename+'.pdf','wb')
        if VERBOSE: print "Converting " + pagename + " to pdf..."
        pdf = pisa.CreatePDF(infile,outfile,FOLDER,link_callback=fetch_resources)
        outfile.close()
        if pdf.err: return pdf.err
        return 0

def createpdf_htmldoc(pagename):
    "creates a pdf file from a saved page using htmldoc (external app, but supports images)"
    if not exists(pagename+".pdf",image=True):
        infile = FOLDER + os.sep + pagename+'.html'
        outfile = FOLDER + os.sep + pagename+'.pdf'
        return os.system('htmldoc --webpage --textfont sans --browserwidth 840 -f '+outfile+' '+infile)

def createpdf_wkhtmltopdf(pagename):
    "creates a pdf file from a saved page using htmldoc (external app, but supports images)"
    if not exists(pagename+".pdf",image=True):
        infile = FOLDER + os.sep + pagename+'.html'
        outfile = FOLDER + os.sep + pagename+'.pdf'
        return os.system('wkhtmltopdf '+infile+' '+outfile)

def joinpdf():
    "creates one pdf file from several others, following order from startpage"
    from pyPdf import PdfFileReader,PdfFileWriter
    if VERBOSE: print "Building table of contents..."
    f = open(FOLDER+os.sep+INDEX+'.html')
    html = ''
    for line in f: html += line
    f.close()
    html = html.replace("\n"," ")
    html = html.replace("> <","><")
    html = re.findall("<ul.*/ul>",html)[0]
    pages = re.findall('href="(.*?)"',html)
    pages.insert(1,INDEX+".html")
    result = PdfFileWriter()
    for p in pages:
        if exists(p[:-5]):
            if VERBOSE: print 'Appending',p
            try: inputfile = PdfFileReader(file(FOLDER+os.sep+p[:-5]+'.pdf','rb'))
            except: print 'Unable to append',p
            else:
                for i in range(inputfile.getNumPages()):
                    result.addPage(inputfile.getPage(i))
    outputfile = file("freecad.pdf",'wb')
    result.write(outputfile)
    outputfile.close()
    if VERBOSE: print 'Successfully created freecad.pdf'

def local(page,image=False):
    "returns a local path for a given page/image"
    if image:
        return FOLDER + os.sep + page
    else:
        return FOLDER + os.sep + page + '.html'

def exists(page,image=False):
    "checks if given page/image already exists"
    path = local(page,image)
    if os.path.exists(path): return True
    return False
    
if __name__ == "__main__":
	crawl()
      
