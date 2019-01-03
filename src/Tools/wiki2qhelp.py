#!/usr/bin/env python

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Library General Public License (LGPL)   *
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

__title__="wiki2qhelp"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script retrieves the contents of a wiki site and saves it locally,
then calls qt help compiler to produce a qhelp-assistant help file.
The script can be called without arguments, it will then use the default
url below, or by passing it an url and optionally a TOC name.
"""

import sys, os, re, tempfile, getopt
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

DEFAULTURL = "www.freecadweb.org/wiki" #default URL if no URL is passed
INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
NORETRIEVE = ['Manual','Developer_hub','Power_users_hub','Users_hub','Source_documentation', 'User_hub','Main_Page','About_this_site'] # pages that won't be fetched (kept online)
GETTRANSLATIONS = True # Set true if you want to get the translations too.
MAXFAIL = 3 # max number of retries if download fails
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.
COMPILE = True # Whether Qt assistant will be used to compile the final help file
OUTPUTPATH = os.path.expanduser("~")+os.sep+'.FreeCAD' # Where to store the qch file
QHELPCOMPILER = 'qhelpgenerator'
QCOLLECTIOMGENERATOR = 'qcollectiongenerator'
PDFOUTPUT = False # if true, a pdf file will be generated instead of qhelp.
REMOVE = True # if true, the temp html files are removed after successful operation
PDFCONVERTOR = 'pisa' # can be 'pisa' or 'htmldoc'

#    END CONFIGURATION      ##############################################

URL = DEFAULTURL
TMPFOLDER = tempfile.mkdtemp()
wikiindex = "/index.php?title="
processed = []
pisa = None
usage='''
    wiki2qhelp [options] [url] [index page]

    fetches wiki pages from the specified url, starting from specified
    index page, and outputs a .qch file in the specified output path.
    You must have qassistant installed.

    If no url, index page or output path is specified, the following
    default values will be used:
    url: '''+DEFAULTURL+'''
    index page: '''+INDEX+'''
    output path: '''+OUTPUTPATH+'''

    Options:

    -v: Verbose mode
    -c filename or --helpcompiler-exe filename: Uses filename as qt help compiler
    -g filename or --helpgenerator-exe filename: Uses filename as qt collection generator
    -o path or --out-path path: Specifies an output path
    -h or --help: Displays this help message
    -p [convertor] or --pdf [convertor]: Outputs a pdf file instead of qhelp. Convertor
                                         can be pisa (default) or htmldoc
    -t path or --tempfolder path: Uses path as temp folder for storing html files

    '''
css = """/* Basic CSS for offline wiki rendering */

body {
  font-family: Arial,Helvetica,sans-serif;
  font-size: 13px;
  text-align: justify;
  }

h1 {
  font-size: 2.2em;
  font-weight: bold;
  background: #46A4D0;
  color: white;
  padding: 5px;
  -moz-border-radius: 5px;
  -webkit-border-radius: 5px;
  }

pre {
  border: 1px dashed #333333;
  text-align: left;
  background: #EEEEEE;
  padding: 5px;
  }

a:link, a:visited {
  font-weight: bold;
  text-decoration: none;
  color: #0084FF;
  }

a:hover {
  text-decoration: underline;
  }

.printfooter {
  font-size: 0.8em;
  color: #333333;
  border-top: 1px solid #333333;
  }

.wikitable #toc {
  font-size: 0.8em;
  }

#toc,.docnav {
  display: none;
  }

"""
fcount = dcount = 0

def rmall(dirPath):                             # delete dirPath and below
    global fcount, dcount
    namesHere = os.listdir(dirPath)
    for name in namesHere:                      # remove all contents first
        path = os.path.join(dirPath, name)
        if not os.path.isdir(path):             # remove simple files
            os.remove(path)
            fcount = fcount + 1
        else:                                   # recur to remove subdirs
            rmall(path)
    os.rmdir(dirPath)                           # remove now-empty dirPath
    dcount = dcount + 1

def crawl(site=DEFAULTURL):
    "downloads an entire wiki site"

    # tests ###############################################
    
    if COMPILE and os.system(QHELPCOMPILER +' -v'):
        print ("Error: QAssistant not fully installed, exiting.")
        print (QHELPCOMPILER)
        return 1
    if COMPILE and os.system(QCOLLECTIOMGENERATOR +' -v'):
        print ("Error: QAssistant not fully installed, exiting.")
        return 1
    if PDFOUTPUT:
        if PDFCONVERTOR == 'pisa':
            try:
                import ho.pisa as pisa
            except: ("Error: Python-pisa not installed, exiting.")
            return 1
        else:
            if os.system('htmldoc --version'):
                print ("Error: Htmldoc not found, exiting.")
                return 1
        try:
            from pyPdf import PdfFileReader,PdfFileWriter
        except:
            print ("Error: Python-pypdf not installed, exiting.")

    # run ########################################################
    
    URL = site
    if VERBOSE: print ("crawling "), URL, ", saving in ", TMPFOLDER
    if not os.path.isdir(TMPFOLDER): os.mkdir(TMPFOLDER)
    file = open(TMPFOLDER + os.sep + "wiki.css",'wb')
    file.write(css)
    file.close()
    todolist = []
    count = 1
    indexpages = get(INDEX)
    todolist.extend(indexpages)
    while todolist:
        targetpage = todolist.pop()
        if not targetpage in NORETRIEVE:
            if VERBOSE: print (count, ": Fetching ", targetpage)
            pages = get(targetpage)
            count += 1
            processed.append(targetpage)
            for p in pages:
                if (not (p in todolist)) and (not (p in processed)):
                    todolist.append(p)
    if VERBOSE: print ("Fetched ", count, " pages")
    if PDFOUTPUT:
        buildpdffiles()
        joinpdf()
        if REMOVE:
            if VERBOSE: print ("Deleting temp files...")
            rmall(TMPFOLDER)
    if COMPILE:
        qhp = buildtoc()
        qhcp = createCollProjectFile()
        if generate(qhcp) or compile(qhp):
            print ("Temp Folder ",TMPFOLDER," has not been deleted.")
            return 1
        else:
            if REMOVE:
                if VERBOSE: print ("Deleting temp files...")
                rmall(TMPFOLDER)
    if VERBOSE: print ("All done!")
    return 0

def buildpdffiles(folder=TMPFOLDER,convertor=PDFCONVERTOR):
    "scans a folder for html files and converts them all to pdf"
    templist = os.listdir(folder)
    fileslist = []
    for i in templist:
        if i[-5:] == '.html':
            fileslist.append(i)
    for f in fileslist:
        if convertor == 'pisa': createpdf_pisa(f[:-5],folder)
        else: createpdf_htmldoc(f[:-5],folder)

def fetch_resources(uri, rel):
        """
        Callback to allow pisa/reportlab to retrieve Images,Stylesheets, etc.
        'uri' is the href attribute from the html link element.
        'rel' gives a relative path, but it's not used here.

        Note from Yorik: Not working!!
        """
        path = os.path.join(TMPFOLDER,uri.replace("./", ""))
        return path

def createpdf_pisa(pagename,folder=TMPFOLDER):
    "creates a pdf file from a saved page using pisa (python module)"
    infile = file(folder + os.sep + pagename+'.html','ro')
    outfile = file(folder + os.sep + pagename+'.pdf','wb')
    if VERBOSE: print ("Converting " + pagename + " to pdf...")
    pdf = pisa.CreatePDF(infile,outfile,folder,link_callback=fetch_resources)
    outfile.close()
    if pdf.err: return pdf.err
    return 0

def createpdf_htmldoc(pagename,folder=TMPFOLDER):
    "creates a pdf file from a saved page using htmldoc (external app, but supports images)"
    infile = folder + os.sep + pagename+'.html'
    outfile = folder + os.sep + pagename+'.pdf'
    return os.system('htmldoc --webpage -f '+outfile+' '+infile)

def joinpdf(folder=TMPFOLDER,startpage=INDEX,outputname='freecad.pdf'):
    "creates one pdf file from several others, following order from startpage"
    if VERBOSE: print ("Building table of contents...")
    f = open(folder+os.sep+startpage+'.html')
    html = ''
    for line in f: html += line
    f.close()
    html = html.replace("\n"," ")
    html = html.replace("> <","><")
    html = re.findall("<ul.*/ul>",html)[0]
    pages = re.findall('href="(.*?)"',html)
    pages.insert(1,startpage+".html")
    result = PdfFileWriter()
    for p in pages:
        if exists(p[:-5]):
            if VERBOSE: print ('Appending',p)
            try: inputfile = PdfFileReader(file(folder+os.sep+p[:-5]+'.pdf','rb'))
            except: print ('Unable to append',p)
            else:
                for i in range(inputfile.getNumPages()):
                    result.addPage(inputfile.getPage(i))
    outputfile = file(OUTPUTPATH + os.sep + outputname,'wb')
    result.write(outputfile)
    outputfile.close()
    if VERBOSE: print ('Successfully created',OUTPUTPATH,os.sep,outputname)
    
def compile(qhpfile,outputname='freecad.qch'):
    "compiles the whole html doc with qassistant"
    qchfile = OUTPUTPATH + os.sep + outputname
    if not os.system(QHELPCOMPILER + ' '+qhpfile+' -o '+qchfile):
        if VERBOSE: print ("Successfully created",qchfile)
        return 0

def generate(qhcpfile):
    "generates qassistant-specific settings like icon, title, ..."
    txt="""
The help files for FreeCAD.
    """
    about=open(TMPFOLDER + os.sep + "about.txt","w")
    about.write(txt)
    about.close()
    qhcfile = OUTPUTPATH + os.sep + "freecad.qhc"
    if not os.system(QCOLLECTIOMGENERATOR+' '+qhcpfile+' -o '+qhcfile):
        if VERBOSE: print ("Successfully created ",qhcfile)
        return 0

def createCollProjectFile(folder=TMPFOLDER):
    qprojectfile = '''<?xml version="1.0" encoding="UTF-8"?>
<QHelpCollectionProject version="1.0">
    <assistant>
        <title>FreeCAD User Manual</title>
        <applicationIcon>Crystal_Clear_app_tutorials.png</applicationIcon>
        <cacheDirectory>freecad/freecad</cacheDirectory>
        <startPage>qthelp://org.freecad.usermanual_0.9/doc/Online_Help_Startpage.html</startPage>
        <aboutMenuText>
            <text>About FreeCAD</text>
        </aboutMenuText>
        <aboutDialog>
            <file>about.txt</file>
            <!--
            <icon>images/icon.png</icon>
            -->
            <icon>Crystal_Clear_app_tutorials.png</icon>
        </aboutDialog>
        <enableDocumentationManager>true</enableDocumentationManager>
        <enableAddressBar>true</enableAddressBar>
        <enableFilterFunctionality>true</enableFilterFunctionality>
    </assistant>
    <docFiles>
        <generate>
            <file>
                <input>freecad.qhp</input>
                <output>freecad.qch</output>
                </file>
            </generate>
        <register>
            <file>freecad.qch</file>
        </register>
    </docFiles>
</QHelpCollectionProject>
'''
    if VERBOSE: print ("Building project file...")
    qfilename = folder + os.sep + "freecad.qhcp"
    f = open(qfilename,'w')
    f.write(qprojectfile)
    f.close()
    if VERBOSE: print ("Done writing qhcp file.")
    return qfilename

def buildtoc(folder=TMPFOLDER,page=INDEX):
    "gets the table of contents page and parses its contents into a clean lists structure"
    
    qhelpfile = '''<?xml version="1.0" encoding="UTF-8"?>
<QtHelpProject version="1.0">
    <namespace>org.freecad.usermanual_0.9</namespace>
    <virtualFolder>doc</virtualFolder>
    <!--
    <customFilter name="FreeCAD 0.10">
        <filterAttribute>FreeCAD</filterAttribute>
        <filterAttribute>0.10</filterAttribute>
    </customFilter>
    -->
    <filterSection>
        <!--
        <filterAttribute>FreeCAD</filterAttribute>
        <filterAttribute>0.10</filterAttribute>
        -->
        <toc>
            <inserttoc>
        </toc>
        <keywords>
            <insertkeywords>
        </keywords>
        <insertfiles>
    </filterSection>
</QtHelpProject>
'''
    
    def getname(line):
        line = re.compile('<li>').sub('',line)
        line = re.compile('</li>').sub('',line)
        title = line.strip()
        link = ''
        if "<a" in line:
            title = re.findall('<a[^>]*>(.*?)</a>',line)[0].strip()
            link = re.findall('href="(.*?)"',line)[0].strip()
        return title,link

    if VERBOSE: print ("Building table of contents...")
    f = open(folder+os.sep+page+'.html')
    html = ''
    for line in f: html += line
    f.close()
    html = html.replace("\n"," ")
    html = html.replace("> <","><")
    html = re.findall("<ul.*/ul>",html)[0]
    items = re.findall('<li[^>]*>.*?</li>|</ul></li>',html)
    inserttoc = '<section title="Table of Contents">\n'
    insertkeywords = ''
    for item in items:
        if not ("<ul>" in item):
            if ("</ul>" in item):
                inserttoc += '</section>\n'
            else:
                link = ''
                title,link=getname(item)
                if link:
                    link='" ref="'+link
                    insertkeywords += ('<keyword name="'+title+link+'"/>\n')
                inserttoc += ('<section title="'+title+link+'"></section>\n')
        else:
            subitems = item.split("<ul>")
            for i in range(len(subitems)):
                link = ''
                title,link=getname(subitems[i])
                if link:
                    link='" ref="'+link
                    insertkeywords += ('<keyword name="'+title+link+'"/>\n')
                trail = ''
                if i == len(subitems)-1: trail = '</section>'
                inserttoc += ('<section title="'+title+link+'">'+trail+'\n')
    inserttoc += '</section>\n'

    insertfiles = "<files>\n"
    for fil in os.listdir(folder):
        insertfiles += ("<file>"+fil+"</file>\n")
    insertfiles += "</files>\n"

    qhelpfile = re.compile('<insertkeywords>').sub(insertkeywords,qhelpfile)
    qhelpfile = re.compile('<inserttoc>').sub(inserttoc,qhelpfile)
    qhelpfile = re.compile('<insertfiles>').sub(insertfiles,qhelpfile)
    qfilename = folder + os.sep + "freecad.qhp"
    f = open(qfilename,'wb')
    f.write(qhelpfile)
    f.close()
    if VERBOSE: print ("Done writing qhp file.")
    return qfilename

def get(page):
    "downloads a single page, returns the other pages it links to"
    html = fetchpage(page)
    html = cleanhtml(html)
    pages = getlinks(html)
    html = cleanlinks(html,pages)
    html = cleanimagelinks(html)
    output(html,page)
    return pages

def cleanhtml(html):
    "cleans given html code from dirty script stuff"
    html = html.replace('\n','Wlinebreak') # removing linebreaks for regex processing
    html = re.compile('(.*)<div[^>]+column-content+[^>]+>').sub('',html) # stripping before content
    html = re.compile('<div[^>]+column-one+[^>]+>.*').sub('',html) # stripping after content
    html = re.compile('<!--[^>]+-->').sub('',html) # removing comment tags
    html = re.compile('<script[^>]*>.*?</script>').sub('',html) # removing script tags
    html = re.compile('<!--\[if[^>]*>.*?endif\]-->').sub('',html) # removing IE tags
    html = re.compile('<div id="jump-to-nav"[^>]*>.*?</div>').sub('',html) # removing nav div
    html = re.compile('<h3 id="siteSub"[^>]*>.*?</h3>').sub('',html) # removing print subtitle
    html = re.compile('Retrieved from').sub('Online version:',html) # changing online title
    html = re.compile('<div id="mw-normal-catlinks[^>]>.*?</div>').sub('',html) # removing catlinks
    html = re.compile('<div class="NavHead.*?</div>').sub('',html) # removing nav stuff
    html = re.compile('<div class="NavContent.*?</div>').sub('',html) # removing nav stuff
    html = re.compile('<div class="NavEnd.*?</div>').sub('',html) # removing nav stuff
    if not GETTRANSLATIONS:
        html = re.compile('<div class="languages.*?</div>').sub('',html) # removing translations links
    html = re.compile('Wlinebreak').sub('\n',html) # restoring original linebreaks
    return html
    
def getlinks(html):
    "returns a list of wikipage links in html file"
    links = re.findall('<a[^>]*>.*?</a>',html)
    pages = []
    for l in links:
        # rg = re.findall('php\?title=(.*)\" title',l)
        rg = re.findall('href=.*?php\?title=(.*?)"',l)
        if rg:
            rg = rg[0]
            if "#" in rg:
                rg = rg.split('#')[0]
            if ":" in rg:
                NORETRIEVE.append(rg)
            if ";" in rg:
                NORETRIEVE.append(rg)
            if "&" in rg:
                NORETRIEVE.append(rg)
            if "/" in rg:
                if not GETTRANSLATIONS:
                    NORETRIEVE.append(rg)
            pages.append(rg)
    return pages

def getimagelinks(html):
    "returns a list of image links found in an html file"
    return re.findall('<img.*?src="(.*?)"',html)

def cleanlinks(html, pages=None):
    "cleans page links found in html"
    if not pages: pages = getlinks(html)
    for page in pages:
        if  page in NORETRIEVE:
            output = 'href="' + URL + wikiindex + page + '"'
        else:
            output = 'href="' + page.replace("/","-") + '.html"'
        html = re.compile('href="[^"]+' + page + '"').sub(output,html)
    return html

def cleanimagelinks(html,links=None):
    "cleans image links in given html"
    if not links: links = getimagelinks(html)
    if links:
        for l in links:
            nl = re.findall('.*/(.*)',l)
            if nl: html = html.replace(l,nl[0])
            fetchimage(l)
    return html

def fetchpage(page):
    "retrieves given page from the wiki"
    failcount = 0
    while failcount < MAXFAIL:
        try:
            html = (urlopen(URL + wikiindex + page).read())
            return html
        except HTTPError:
            failcount += 1
    print ('Error: unable to fetch page ' + page)

def fetchimage(imagelink):
    "retrieves given image from the wiki and saves it"
    filename = re.findall('.*/(.*)',imagelink)[0]
    if not (filename in processed):
        failcount = 0
        while failcount < MAXFAIL:
            try:
                if VERBOSE: print ("Fetching " + filename)
                data = (urlopen(webroot(URL) + imagelink).read())
                path = local(filename,image=True)
                file = open(path,'wb')
                file.write(data)
                file.close()
                processed.append(filename)
                return
            except:
                failcount += 1
        print ('Error: unable to fetch file ' + filename)

def local(page,image=False):
    "returns a local path for a given page/image"
    if image:
        return TMPFOLDER + os.sep + page
    else:
        return TMPFOLDER + os.sep + page + '.html'

def exists(page,image=False):
    "checks if given page/image already exists"
    path = local(page,image)
    if os.path.exists(path): return True
    return False

def webroot(url):
    return re.findall('(http://.*?)/',url)[0]

def output(html,page):
    "encapsulates raw html code into nice html body"
    header = "<html><head>"
    header += "<title>"
    header += page
    header += "</title>"
    header += "<link type='text/css' href='wiki.css' rel='stylesheet'>"
    header += "</head><body>"
    footer = "</body></html>"
    html = header+html+footer
    filename = local(page.replace("/","-"))
    file = open(filename,'wb')
    file.write(html)
    file.close()

def main(arg):
	global QHELPCOMPILER,QCOLLECTIOMGENERATOR,OUTPUTPATH,PDFOUTPUT,PDFCONVERTOR,TMPFOLDER
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hp:t:c:g:o:", ["help", "pdf=", "noremove", "tempfolder=", "helpcompiler-exe=", "out-path=", "helpgenerator-exe="])
	except getopt.GetoptError:
		# print help information and exit:
		sys.stderr.write(usage)
		sys.exit(2)

	# checking on the options
	for o, a in opts:
		if o == "-v":
			VERBOSE = True
		if o in ("-p","--pdf"):
			PDFOUTPUT = True
			if a in ['pisa','htmldoc']:
				print ("using pdf converter:",a)
				PDFCONVERTOR = a
		if o in ("-t","--tempfolder"):
			print ("using tempfolder:",a)
			TMPFOLDER = a
		if o in ("-h", "--help"):
			sys.stderr.write(usage)
			sys.exit()
		if o in ("-c", "--helpcompiler-exe"):
			QHELPCOMPILER = a
			print ('Using: ',QHELPCOMPILER)
		if o in ("-g", "--helpgenerator-exe"):
			QCOLLECTIOMGENERATOR = a
		if o in ("-o", "--out-path"):
			print ("Using output path:",a)
			OUTPUTPATH = a
#    if arg:
#        if (arg[0] == '-h') or (arg[0] == '--help'):
#            print usage
#        else:
#            URL = arg[0]
#            if len(arg) > 1: INDEX = arg[1]
#            if len(arg) > 2: OUTPUTPATH = arg[2]
#            crawl()
#    else:
		crawl()

if __name__ == "__main__":
    # main(sys.argv[1:])
    print ("Warning! This script is obsolete. Use the scripts in the offlinedocs folder...")
      
