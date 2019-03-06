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

from __future__ import print_function

__title__="downloadwiki"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script retrieves the contents of a wiki site from a pages list
"""

import sys, os, re, tempfile, getopt
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

DEFAULTURL = "https://www.freecadweb.org" #default URL if no URL is passed
INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
NORETRIEVE = ['Manual','Developer_hub','Power_users_hub','Users_hub','Source_documentation', 
              'User_hub','Main_Page','About_this_site','Interesting_links','Syndication_feeds',
              'FreeCAD:General_disclaimer','FreeCAD:About','FreeCAD:Privacy_policy','WikiPages'] # pages that won't be fetched (kept online)
NORETRIEVE += ['Constraint_Concentric','Constraint_EqualLength','Constraint_ExternalAngle',
               'Constraint_Horizontal','Constraint_HorizontalDistance','Constraint_Internal_Alignment',
               'Constraint_InternalAngle','Constraint_Length','Constraint_Lock','Constraint_Parallel',
               'Constraint_Perpendicular','Constraint_PointOnEnd','Constraint_PointOnMidPoint',
               'Constraint_PointOnObject','Constraint_PointOnPoint','Constraint_PointOnStart',
               'Constraint_PointToObject','Constraint_Radius','Constraint_SnellsLaw',
               'Constraint_Symmetric','Constraint_Tangent','Constraint_TangentToEnd',
               'Constraint_TangentToStart','Constraint_Vertical'] # pages that have been renamed but still dangle around...GETTRANSLATIONS = False # Set true if you want to get the translations too.
MAXFAIL = 3 # max number of retries if download fails
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.
GETTRANSLATIONS = False # Set true if you want to get the translations too.

#    END CONFIGURATION      ##############################################

FOLDER = "./localwiki"
LISTFILE = "wikifiles.txt"
URL = DEFAULTURL
wikiindex = "/wiki/index.php?title="
defaultfile = "<html><head><link type='text/css' href='wiki.css' rel='stylesheet'></head><body>&nbsp;</body></html>"
css = """/* Basic CSS for offline wiki rendering */

body {
  font-family: Fira Sans,Arial,Helvetica,sans-serif;
  font-size: 14px;
  text-align: justify;
  /*background: #fff;
  color: #000;*/
  max-width: 800px;
  }

h1 {
  font-size: 2.4em;
  font-weight: bold;
  padding: 5px;
  border-radius: 5px;
  }
  
h2 {
  font-weight: normal;
  font-size: 1.6em;
  border-bottom: 1px solid #ddd;
  }
  
h3 {
  padding-left: 20px;
  }
  
img {
  max-width: 100%;
  }
  
li {
  margin-top: 10px;
  }

pre, .mw-code {
  text-align: left;
  /*background: #eee;*/
  padding: 5px 5px 5px 20px;
  font-family: mono;
  border-radius: 2px;
  }

a:link, a:visited {
  font-weight: bold;
  text-decoration: none;
  color: #2969C4;
  }

a:hover {
  text-decoration: underline;
  }

.printfooter {
  font-size: 0.8em;
  color: #333333;
  border-top: 1px solid #333;
  margin-top: 20px;
  }

.wikitable #toc {
  font-size: 0.8em;
  }

.ct, .ctTitle, .ctOdd, .ctEven th {
  font-size: 1em;
  text-align: left;
  width: 190px;
  float: right;
  /*background: #eee;*/
  margin-top: 10px;
  border-radius: 2px;
  }
  
.ct {
  margin-left: 15px;
  padding: 10px;
  }
#mw-navigation {
  display:none; /*TODO remove on next build (included below)*/
 }
"""

def crawl():
    "downloads an entire wiki site"
    global processed
    processed = []
    if VERBOSE: print ("crawling ", URL, ", saving in ", FOLDER)
    if not os.path.isdir(FOLDER): os.mkdir(FOLDER)
    file = open(FOLDER + os.sep + "wiki.css",'wb')
    file.write(css)
    file.close()
    dfile = open(FOLDER + os.sep + "default.html",'wb')
    dfile.write(defaultfile)
    dfile.close()
    lfile = open(LISTFILE)
    global locallist
    locallist = []
    for l in lfile: locallist.append(l.replace("\n",""))
    lfile.close()
    todolist = locallist[:]
    print ("getting",len(todolist),"files...")
    count = 1
    indexpages = get(INDEX)
    while todolist:
        targetpage = todolist.pop()
        if VERBOSE: print (count, ": Fetching ", targetpage)
        get(targetpage)
        count += 1
    if VERBOSE: print ("Fetched ", count, " pages")
    if VERBOSE: print ("All done!")
    return 0

def get(page):
    "downloads a single page, returns the other pages it links to"
    localpage = page
    if "Command_Reference" in localpage:
        localpage = localpage.replace("Category:","")
        localpage = localpage.replace("&pagefrom=","+")
        localpage = localpage.replace("#mw-pages","")
    if page[-4:] in [".png",".jpg",".svg",".gif","jpeg",".PNG",".JPG"]:
        fetchimage(page)
    elif not exists(localpage):
        html = fetchpage(page)
        html = cleanhtml(html)
        pages = getlinks(html)
        html = cleanlinks(html,pages)
        html = cleanimagelinks(html)
        output(html,page)
    else:
        if VERBOSE: print ("    skipping",page)

def getlinks(html):
    "returns a list of wikipage links in html file"
    links = re.findall('<a[^>]*>.*?</a>',html)
    pages = []
    for l in links:
        # rg = re.findall('php\?title=(.*)\" title',l)
        rg = re.findall('href=.*?php\?title=(.*?)"',l)
        if not rg:
            rg = re.findall('href="\/wiki\/(.*?)"',l)
        if rg:
            rg = rg[0]
            if not "Command_Reference" in rg:
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

def cleanhtml(html):
    "cleans given html code from dirty script stuff"
    html = html.replace('\n','Wlinebreak') # removing linebreaks for regex processing
    html = html.replace('\t','') # removing tab marks
    html = re.compile('(.*)<div id=\"content+[^>]+>').sub('',html) # stripping before content
    html = re.compile('<div id="mw-head+[^>]+>.*').sub('',html) # stripping after content
    html = re.compile('<!--[^>]+-->').sub('',html) # removing comment tags
    html = re.compile('<script[^>]*>.*?</script>').sub('',html) # removing script tags
    html = re.compile('<!--\[if[^>]*>.*?endif\]-->').sub('',html) # removing IE tags
    html = re.compile('<div id="jump-to-nav"[^>]*>.*?</div>').sub('',html) # removing nav div
    html = re.compile('<h3 id="siteSub"[^>]*>.*?</h3>').sub('',html) # removing print subtitle
    html = re.compile('Retrieved from').sub('Online version:',html) # changing online title
    html = re.compile('<div id="mw-normal-catlinks.*?</div>').sub('',html) # removing catlinks
    html = re.compile('<div class="NavHead.*?</div>').sub('',html) # removing nav stuff
    html = re.compile('<div class="NavContent.*?</div>').sub('',html) # removing nav stuff
    html = re.compile('<div class="NavEnd.*?</div>').sub('',html) # removing nav stuff
    html = re.compile('<div id="mw-navigation.*?</div>').sub('',html) # removing nav stuff
    html = re.compile('<table id="toc.*?</table>').sub('',html) # removing toc
    html = re.compile('width=\"100%\" style=\"float: right; width: 230px; margin-left: 1em\"').sub('',html) # removing command box styling
    html = re.compile('<div class="docnav.*?</div>Wlinebreak</div>').sub('',html) # removing docnav
    html = re.compile('<div class="mw-pt-translate-header.*?</div>').sub('',html) # removing translations links
    if not GETTRANSLATIONS:
        html = re.compile('<div class="languages.*?</div>').sub('',html) # removing translations links
        html = re.compile('<div class="mw-pt-languages.*?</div>').sub('',html) # removing translations links
    html = re.compile('Wlinebreak').sub('\n',html) # restoring original linebreaks
    return html
    

def cleanlinks(html, pages=None):
    "cleans page links found in html"
    if not pages: pages = getlinks(html)
    for page in pages:
        if  page in NORETRIEVE:
            output = 'href="' + URL + wikiindex + page + '"'
        else:
            output = 'href="' + page.replace("/","-") + '.html"'
        html = re.compile('href="[^"]+' + page + '"').sub(output,html)
        if "Command_Reference" in output:
            html = html.replace("Category:","")
            html = html.replace("&amp;pagefrom=","+")
            html = html.replace("#mw-pages",".html")
            html = html.replace("/wiki/index.php?title=Command_Reference","Command_Reference")
    return html

def cleanimagelinks(html,links=None):
    "cleans image links in given html"
    if not links: links = getimagelinks(html)
    if links:
        for l in links:
            nl = re.findall('.*/(.*)',l)
            if nl: html = html.replace(l,nl[0])
            # fetchimage(l)
    return html

def fetchpage(page):
    "retrieves given page from the wiki"
    print ("    fetching: ",page)
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
    if imagelink[0:5] == "File:":
        print ("Skipping file page link")
        return
    filename = re.findall('.*/(.*)',imagelink)[0]
    if not exists(filename,image=True):
        failcount = 0
        while failcount < MAXFAIL:
            try:
                if VERBOSE: print ("    fetching " + filename)
                data = (urlopen(URL + imagelink).read())
                path = local(filename,image=True)
                file = open(path,'wb')
                file.write(data)
                file.close()
            except:
                failcount += 1
            else:
                processed.append(filename)
                if VERBOSE: print ("    saving",local(filename,image=True))
                return
        print ('Error: unable to fetch file ' + filename)
    else:
        if VERBOSE: print ("    skipping",filename)

def local(page,image=False):
    "returns a local path for a given page/image"
    if image:
        return FOLDER + os.sep + page
    else:
        return FOLDER + os.sep + page + '.html'

def exists(page,image=False):
    "checks if given page/image already exists"
    path = local(page.replace("/","-"),image)
    if os.path.exists(path): return True
    return False

def webroot(url):
    return re.findall('(http://.*?)/',url)[0]

def output(html,page):
    "encapsulates raw html code into nice html body"
    title = page.replace("_"," ")
    header = "<html><head>"
    header += "<title>" + title + "</title>"
    header += '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">'
    header += "<link type='text/css' href='wiki.css' rel='stylesheet'>"
    header += "</head><body>"
    header += "<h1>" + title + "</h1>"
    footer = "</body></html>"
    html = header+html+footer
    filename = local(page.replace("/","-"))
    if "Command_Reference" in filename:
        filename = filename.replace("Category:","")
        filename = filename.replace("&pagefrom=","+")
        filename = filename.replace("#mw-pages","")
        filename = filename.replace(".html.html",".html")
    print ("    saving",filename)
    file = open(filename,'wb')
    file.write(html)
    file.close()

if __name__ == "__main__":
    crawl()

