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

__title__="buildwikiindex.py"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script parses the contents of a wiki site and saves a file containing
names of pages and images to be downloaded.
"""

import sys, os, re, tempfile, getopt
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

URL = "https://www.freecadweb.org/wiki" #default URL if no URL is passed
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
               'Constraint_TangentToStart','Constraint_Vertical'] # pages that have been renamed but still dangle around...
GETTRANSLATIONS = False # Set true if you want to get the translations too.
MAXFAIL = 3 # max number of retries if download fails
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.
WRITETHROUGH = True # if true, fetched files are constantly written to disk, in case of failure.

#    END CONFIGURATION      ##############################################

wikiindex = "/index.php?title="

def crawl(pagename=[]):
    "downloads an entire wiki site"    
    todolist = []
    processed = []
    count = 1
    if pagename:
        if not isinstance(pagename,list):
            pagename = [pagename]
        todolist = pagename
    else:
        if os.path.exists("wikifiles.txt"):
            f = open("wikifiles.txt","r")
            if VERBOSE: print ("Reading existing list...")
            for l in f.readlines():
                if l.strip() != "":
                    if VERBOSE: print ("Adding ",l)
                    processed.append(l.strip())
            f.close()
        if os.path.exists("todolist.txt"):
            f = open("todolist.txt","r")
            if VERBOSE: print ("Reading existing todo list...")
            for l in f.readlines():
                if l.strip() != "":
                    todolist.append(l.strip())
            f.close()
        else:
            indexpages,imgs = get(INDEX)
            todolist.extend(indexpages)
    while todolist:
        targetpage = todolist.pop()
        if (not targetpage in NORETRIEVE):
            if VERBOSE: print (count, ": Scanning ", targetpage)
            pages,images = get(targetpage)
            count += 1
            processed.append(targetpage)
            processed.extend(images)
            if VERBOSE: print ("got",len(pages),"links")
            for p in pages:
                if (not (p in todolist)) and (not (p in processed)):
                    todolist.append(p)
            if WRITETHROUGH:
                writeList(processed)
                writeList(todolist,"todolist.txt")
    if VERBOSE: print ("Fetched ", count, " pages")
    if not WRITETHROUGH:
        writeList(processed)
    if pagename:
        return processed
    return 0

def get(page):
    "downloads a single page, returns the other pages it links to"
    html = fetchpage(page)
    html = cleanhtml(html)
    pages = getlinks(html)
    images = getimagelinks(html)
    return pages,images

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
    html = re.compile('<div class="mw-pt-translate-header.*?</div>').sub('',html) # removing translations links
    if not GETTRANSLATIONS:
        html = re.compile('<div class="languages.*?</div>').sub('',html) # removing translations links
        html = re.compile('<div class="mw-pt-languages.*?</div>').sub('',html) # removing translations links
    html = re.compile('Wlinebreak').sub('\n',html) # restoring original linebreaks
    return html
    
def getlinks(html):
    "returns a list of wikipage links in html file"
    global NORETRIEVE
    links = re.findall('<a[^>]*>.*?</a>',html)
    pages = []
    for l in links:
        # rg = re.findall('php\?title=(.*)\" title',l)
        rg = re.findall('href=.*?php\?title=(.*?)"',l)
        if not rg:
            rg = re.findall('href="\/wiki\/(.*?)"',l)
            if "images" in rg:
                rg = None
            if "mediawiki" in rg:
                rg = None
        if rg:
            rg = rg[0]
            if not "Command_Reference" in rg:
                if "#" in rg:
                    rg = rg.split('#')[0]
                if ":" in rg:
                    NORETRIEVE.append(rg)
                if "&" in rg:
                    NORETRIEVE.append(rg)
            if ";" in rg:
                    NORETRIEVE.append(rg)
            if "/" in rg:
                if not GETTRANSLATIONS:
                    NORETRIEVE.append(rg)
            if not rg in NORETRIEVE:
                pages.append(rg)
                print ("got link: ",rg)
    return pages

def getimagelinks(html):
    "returns a list of image links found in an html file"
    imlinks = re.findall('<img.*?src="(.*?)"',html)
    imlinks = [l for l in imlinks if not l.startswith("http")] # remove external images
    return imlinks

def fetchpage(page):
    "retrieves given page from the wiki"
    print ("fetching: ",page)
    failcount = 0
    while failcount < MAXFAIL:
        try:
            html = (urlopen(URL + wikiindex + page).read())
            return html
        except HTTPError:
            failcount += 1
    print ('Error: unable to fetch page ' + page)
    sys.exit()

def cleanList(pagelist):
    "cleans the list"
    npages = []
    for p in pagelist:
        if not p in npages:
            if not "redlink" in p:
                npages.append(p)
    return npages

def writeList(pages,filename="wikifiles.txt"):
    pages = cleanList(pages)
    f = open(filename,"wb")
    for p in pages:
        f.write(p+"\n")
    f.close()
    if VERBOSE: print ("written ",filename)

if __name__ == "__main__":
	crawl(sys.argv[1:])
      
