#!/usr/bin/env python

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *  
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

__title__="update.py"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script needs to be run after the wiki has been fully downloaded. It has three usages:

1) If no revisions.txt file is found, it parses the contents of the wikifiles.txt file
   and, for each entry, it retrieves a corresponding revision ID, and creates a revisions.txt file
   
2) If a revisions.txt file exists but no update.txt file exists, it crawls through all entries of
   wikifiles.txt, and for each one, compares the current revision with the one stored in revisions.txt.
   An update.txt file is created with all pages that have different revision IDs
   
3) If update.txt exists, each entry of it will be scanned again for new links and all the needed 
   files downloaded. Revision.txt and wikifiles.txt get also updated.
"""

import sys, os, re, tempfile, getopt
from urllib.request import urlopen
from urllib.error import HTTPError

#    CONFIGURATION       #################################################

URL = "https://www.freecadweb.org/wiki" #default URL if no URL is passed
GETTRANSLATIONS = False # Set true if you want to get the translations too.
MAXFAIL = 3 # max number of retries if download fails
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.

#    END CONFIGURATION      ##############################################

wikiindex = "/index.php?title="

def update(pagename=None):

    if not os.path.exists("revisions.txt"):                                             # case 1)
        if not os.path.exists("wikifiles.txt"):
            print("No wikifiles.txt found. Aborting")
            sys.exit()
        pages = []
        f = open("wikifiles.txt","r")
        if VERBOSE: print("Reading existing list...")
        for l in f.readlines():
            if l.strip() != "":
                if not "/wiki/" in l:
                    if VERBOSE: print("Adding ",l.strip())
                    pages.append(l.strip())
        f.close()
        if VERBOSE: print("Added ",str(len(pages))," entries")
        i = 1
        revs = []
        for page in pages:
            rev = getRevision(page)
            if VERBOSE: print(str(i),"         revision: ",rev)
            revs.append(page+":"+rev)
            i += 1
        writeList(revs,"revisions.txt")
        print("All done. Successfully written revisions.txt with ",len(revs)," entries.")

    elif os.path.exists("revisions.txt") and (not os.path.exists("updates.txt")):        # case 2)
        f = open("revisions.txt","r")
        if VERBOSE: print("Reading revisions list...")
        revisions = {}
        for l in f.readlines():
            if l.strip() != "":
                r = l.strip().split(":")
                p = ":".join(r[:-1])
                if VERBOSE: print("Adding ",p)
                revisions[p] = r[1]
        f.close()
        if VERBOSE: print("Added ",str(len(list(revisions.keys())))," entries")
        updates = []
        i = 1
        for page in list(revisions.keys()):
            rev = getRevision(page)
            if rev != revisions[page]:
                if VERBOSE: print(str(i),page," has a new revision: ",rev)
                updates.append(page)
            else:
                if VERBOSE: print(str(i),page," is up to date ")
            i += 1
        if updates:
            writeList(updates,"updates.txt")
            print("All done. Successfully written updates.txt with ",len(updates)," entries.")
        else:
            print("Everything up to date. Nothing to be done.")
        
    elif os.path.exists("revisions.txt") and os.path.exists("updates.txt"):              # case 3)
        if not os.path.exists("wikifiles.txt"):
            print("No wikifiles.txt found. Aborting")
            sys.exit()
        wikifiles = []
        f = open("wikifiles.txt","r")
        if VERBOSE: print("Reading wikifiles list...")
        for l in f.readlines():
            if l.strip() != "":
                wikifiles.append(l.strip())
        f.close()
        if VERBOSE: print("Read ",str(len(wikifiles))," entries")
        f = open("revisions.txt","r")
        if VERBOSE: print("Reading revisions list...")
        revisions = {}
        for l in f.readlines():
            if l.strip() != "":
                r = l.strip().split(":")
                p = ":".join(r[:-1])
                revisions[p] = r[1]
        f.close()
        todo = []
        f = open("updates.txt","r")
        if VERBOSE: print("Reading updates list...")
        for l in f.readlines():
            if l.strip() != "":
                todo.append(l.strip())
        f.close()
        if VERBOSE: print(str(len(todo))," pages to scan...")
        import buildwikiindex
        buildwikiindex.WRITETHROUGH = False
        buildwikiindex.VERBOSE = VERBOSE
        updates = []
        for t in todo:
            if VERBOSE: print("Scanning ",t)
            updates.extend(buildwikiindex.crawl(t))
        updates = [u for u in updates if not u in wikifiles]
        if VERBOSE: print(str(len(updates))," files to download...")
        import downloadwiki
        i = 1
        for u in updates:
            if VERBOSE: print(i, ": Fetching ", u)
            downloadwiki.get(u)
            if not "/wiki/" in u:
                rev = getRevision(u)
                revisions[u] = rev
            if not u in wikifiles:
                wikifiles.append(u)
            i += 1
        if VERBOSE: print("Updating wikifiles and revisions...")
        writeList(wikifiles,"wikifiles.txt")
        updatedrevs = []
        for k in list(revisions.keys()):
            updatedrevs.append(k+":"+revisions[k])
        writeList(updatedrevs,"revisions.txt")
        os.remove("updates.txt")
        if VERBOSE: print("All done!")

def getRevision(page):
    html = fetchPage(page)
    revs = re.findall("wgCurRevisionId\"\:(.*?),",html)
    if len(revs) == 1:
        return revs[0]
    print('Error: unable to get revision ID of ' + page)
    sys.exit()

def fetchPage(page):
    "retrieves given page from the wiki"
    print("fetching: ",page)
    failcount = 0
    while failcount < MAXFAIL:
        try:
            html = (urlopen(URL + wikiindex + page).read())
            return html
        except HTTPError:
            failcount += 1
    print('Error: unable to fetch page ' + page)
    sys.exit()

def writeList(pages,filename):
    f = open(filename,"wb")
    for p in pages:
        f.write(p+"\n")
    f.close()
    if VERBOSE: print("written ",filename)

if __name__ == "__main__":
	update(sys.argv[1:])
