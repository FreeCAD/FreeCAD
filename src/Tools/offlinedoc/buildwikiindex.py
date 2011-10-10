#!/usr/bin/env python

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@gmx.fr>                     *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

__title__="buildwikiindex.py"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://free-cad.sf.net"

"""
This script parses the contents of a wiki site and saves a file containing
names of pages and images to be downloaded.
"""

import sys, os, re, tempfile, getopt
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

URL = "http://sourceforge.net/apps/mediawiki/free-cad" #default URL if no URL is passed
INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
NORETRIEVE = ['Manual','Developer_hub','Power_users_hub','Users_hub','Source_documentation', 'User_hub','Main_Page','About_this_site'] # pages that won't be fetched (kept online)
GETTRANSLATIONS = False # Set true if you want to get the translations too.
MAXFAIL = 3 # max number of retries if download fails
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.

#    END CONFIGURATION      ##############################################

wikiindex = "/index.php?title="

def crawl():
    "downloads an entire wiki site"    
    todolist = []
    processed = []
    count = 1
    indexpages,imgs = get(INDEX)
    todolist.extend(indexpages)
    while todolist:
        targetpage = todolist.pop()
        if not targetpage in NORETRIEVE:
            if VERBOSE: print count, ": Scanning ", targetpage
            pages,images = get(targetpage)
            count += 1
            processed.append(targetpage)
            processed.extend(images)
            if VERBOSE: print "got",len(pages),"links"
            for p in pages:
                if (not (p in todolist)) and (not (p in processed)):
                    todolist.append(p)
    if VERBOSE: print "Fetched ", count, " pages"
    writeList(processed)
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

def writeList(pages):
    f = open("wikifiles.txt","wb")
    for p in pages:
        f.write(p+"\n")
    f.close()
    if VERBOSE: print "written wikifiles.txt"

if __name__ == "__main__":
	crawl()
      
