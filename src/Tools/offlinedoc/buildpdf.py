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

import sys, os, re, tempfile, getopt, shutil, time
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
PDFCONVERTOR = 'wkhtmltopdf' # can be 'pisa', 'htmldoc', 'wkhtmltopdf' or 'firefox'
VERBOSE = True # set true to get output messages
INCLUDECOMMANDS = True # if true, the command pages of each workbench are included after each WB page
OVERWRITE = False # if true, pdf files are recreated even if already existing
FIREFOXPDFFOLDER = os.path.expanduser("~")+os.sep+"PDF" # if firefox is used, set this to where it places its pdf files by default

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
    if PDFCONVERTOR == 'wkhtmltopdf':
        makeStyleSheet()
    global fileslist
    fileslist = []
    for i in templist:
        if i[-5:] == '.html':
            fileslist.append(i)
    print "converting ",len(fileslist)," pages"
    i = 1
    for f in fileslist:
        print i," : ",f
        if PDFCONVERTOR == 'pisa':
            createpdf_pisa(f[:-5])
        elif PDFCONVERTOR == 'wkhtmltopdf': 
            createpdf_wkhtmltopdf(f[:-5])
        elif PDFCONVERTOR == 'firefox': 
            createpdf_firefox(f[:-5])
        else: 
            createpdf_htmldoc(f[:-5])
        i += 1

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
    if (not exists(pagename+".pdf",image=True)) or OVERWRTIE:
        infile = file(FOLDER + os.sep + pagename+'.html','ro')
        outfile = file(FOLDER + os.sep + pagename+'.pdf','wb')
        if VERBOSE: print "Converting " + pagename + " to pdf..."
        pdf = pisa.CreatePDF(infile,outfile,FOLDER,link_callback=fetch_resources)
        outfile.close()
        if pdf.err: 
            return pdf.err
        return 0

def createpdf_firefox(pagename):
    "creates a pdf file from a saved page using firefox (needs command line printing extension)"
    # the default printer will be used, so make sure it is set to pdf
    # command line printing extension http://forums.mozillazine.org/viewtopic.php?f=38&t=2729795
    if (not exists(pagename+".pdf",image=True)) or OVERWRITE:
        infile = FOLDER + os.sep + pagename+'.html'
        outfile = FOLDER + os.sep + pagename+'.pdf'
        return os.system('firefox -print ' + infile)
        time.sleep(6)
        if os.path.exists(FIREFOXPDFFOLDER + os.sep + pagename + ".pdf"):
            shutil.move(FIREFOXPDFFOLDER+os.sep+pagename+".pdf",outfile)
        else:
            print "-----------------------------------------> Couldn't find print output!"

def createpdf_htmldoc(pagename):
    "creates a pdf file from a saved page using htmldoc (external app, but supports images)"
    if (not exists(pagename+".pdf",image=True)) or OVERWRITE:
        infile = FOLDER + os.sep + pagename+'.html'
        outfile = FOLDER + os.sep + pagename+'.pdf'
        return os.system('htmldoc --webpage --textfont sans --browserwidth 840 -f '+outfile+' '+infile)

def createpdf_wkhtmltopdf(pagename):
    "creates a pdf file from a saved page using htmldoc (external app, but supports images)"
    if (not exists(pagename+".pdf",image=True)) or OVERWRITE:
        infile = FOLDER + os.sep + pagename+'.html'
        outfile = FOLDER + os.sep + pagename+'.pdf'
        return os.system('wkhtmltopdf --margin-top 5mm --user-style-sheet '+FOLDER+os.sep+'wkhtmltppdf.css '+infile+' '+outfile)
    else:
        print "skipping"

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
            if VERBOSE: print 'Appending',p[:-5]+'.pdf'
            try: 
                inputfile = PdfFileReader(file(FOLDER+os.sep+p[:-5]+'.pdf','rb'))
            except: 
                print 'Unable to append',p
            else:
                for i in range(inputfile.getNumPages()):
                    result.addPage(inputfile.getPage(i))
                if INCLUDECOMMANDS:
                    if ("_Workbench.html" in p) or ("_Module.html" in p):
                        mod = [p.split("_")[0]]
                        if mod[0] == "PartDesign":
                            mod.append("Constraint")
                        for m in mod:
                            for f in fileslist:
                                if f[:len(m)+1] == m+"_":
                                    if (not("Module" in f)) and (not("Workbench" in f)) and (not("Scripting" in f)) and (not("API" in f)):
                                        if VERBOSE: print '    Appending',f[:-5]+'.pdf'
                                        try:
                                            inputfile = PdfFileReader(file(FOLDER+os.sep+f[:-5]+'.pdf','rb'))
                                        except: 
                                            print 'Unable to append',f
                                        else:
                                            for i in range(inputfile.getNumPages()):
                                                result.addPage(inputfile.getPage(i))
    if VERBOSE: print "Writing..."
    outputfile = file(FOLDER+os.sep+"freecad.pdf",'wb')
    result.write(outputfile)
    outputfile.close()
    if VERBOSE: 
        print ' '
        print 'Successfully created '+FOLDER+os.sep+'freecad.pdf'

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

def makeStyleSheet():
    "Creates a stylesheet for wkhtmltopdf"
    outputfile = file(FOLDER+os.sep+"wkhtmltopdf.css",'wb')
    outputfile.write("""
html {
    margin: 50px 0;
}
""")
    outputfile.close()
    
if __name__ == "__main__":
	crawl()
      
