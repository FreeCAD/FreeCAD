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

__title__="downloadwiki"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script retrieves the contents of a wiki site from a pages list
"""

import sys, os, re, tempfile, getopt
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

DEFAULTURL = "http://www.freecadweb.org/wiki" #default URL if no URL is passed
INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
NORETRIEVE = ['Manual','Developer_hub','Power_users_hub','Users_hub','Source_documentation', 'User_hub','Main_Page','About_this_site'] # pages that won't be fetched (kept online)
GETTRANSLATIONS = False # Set true if you want to get the translations too.
MAXFAIL = 3 # max number of retries if download fails
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.

#    END CONFIGURATION      ##############################################

FOLDER = "./localwiki"
LISTFILE = "wikifiles.txt"
URL = DEFAULTURL
wikiindex = "/index.php?title="
defaultfile = "<html><head><link type='text/css' href='wiki.css' rel='stylesheet'></head><body>&nbsp;</body></html>"
css = """
/* Basic CSS for offline wiki rendering */

body {
  font-family: Arial,Helvetica,sans-serif;
  font-size: 13px;
  text-align: justify;
  background: #ffffff;
  color: #000000;
  }

h1 {
  font-size: 2.2em;
  font-weight: bold;
  background: #46A4D0;
  color: white;
  padding: 5px;
  border-radius: 5px;
  }

pre {
  border: 1px solid #888888;
  text-align: left;
  background: #EEEEEE;
  padding: 5px;
  border-radius: 5px;
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

.ct, .ctTitle, .ctOdd, .ctEven th {
  text-align: left;
  width: 200px;
  float: right;
  background: #eeeeee;
  }
"""

def crawl():
    "downloads an entire wiki site"
    processed = []
    if VERBOSE: print "crawling ", URL, ", saving in ", FOLDER
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
    print "getting",len(todolist),"files..."
    count = 1
    indexpages = get(INDEX)
    while todolist:
        targetpage = todolist.pop()
        if VERBOSE: print count, ": Fetching ", targetpage
        get(targetpage)
        count += 1
    if VERBOSE: print "Fetched ", count, " pages"
    if VERBOSE: print "All done!"
    return 0

def get(page):
    "downloads a single page, returns the other pages it links to"
    if page[-4:] in [".png",".jpg",".svg",".gif","jpeg"]:
        print "getting image",page
        fetchimage(page)
    elif not exists(page):
        html = fetchpage(page)
        html = cleanhtml(html)
        pages = getlinks(html)
        html = cleanlinks(html,pages)
        html = cleanimagelinks(html)
        output(html,page)
    else:
        if VERBOSE: print "skipping",page

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
    html = re.compile('<div class="docnav.*?</div></div>').sub('',html) # removing docnav
    if not GETTRANSLATIONS:
        html = re.compile('<div class="languages.*?</div>').sub('',html) # removing translations links
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
    failcount = 0
    while failcount < MAXFAIL:
        try:
            html = (urlopen(URL + wikiindex + page).read())
            return html
        except HTTPError:
            failcount += 1
    print 'Error: unable to fetch page ' + page

def fetchimage(imagelink):
    "retrieves given image from the wiki and saves it"
    if imagelink[0:5] == "File:":
        print "Skipping file page link"
        return
    filename = re.findall('.*/(.*)',imagelink)[0]
    print "saving",filename
    if not exists(filename,image=True):
        failcount = 0
        while failcount < MAXFAIL:
            try:
                if VERBOSE: print "Fetching " + filename
                data = (urlopen(webroot(URL) + imagelink).read())
                path = local(filename,image=True)
                file = open(path,'wb')
                file.write(data)
                file.close()
                processed.append(filename)
                return
            except:
                failcount += 1
        print 'Error: unable to fetch file ' + filename

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
    header = "<html><head>"
    header += "<title>"
    header += page
    header += "</title>"
    header += "<link type='text/css' href='wiki.css' rel='stylesheet'>"
    header += "</head><body>"
    footer = "</body></html>"
    html = header+html+footer
    filename = local(page.replace("/","-"))
    print "saving",filename
    file = open(filename,'wb')
    file.write(html)
    file.close()

if __name__ == "__main__":
	crawl()
      
