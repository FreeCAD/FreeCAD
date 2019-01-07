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

__title__="wiki2qhelp"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"

"""
This script builds qhrlp files from a local copy of the wiki
"""

import sys, os, re, tempfile, getopt, shutil
from urllib2 import urlopen, HTTPError

#    CONFIGURATION       #################################################

FOLDER = "./localwiki"
INDEX = "Online_Help_Toc" # the start page from where to crawl the wiki
VERBOSE = True # to display what's going on. Otherwise, runs totally silent.
QHELPCOMPILER = 'qhelpgenerator'
QCOLLECTIOMGENERATOR = 'qcollectiongenerator'
RELEASE = '0.17'

#    END CONFIGURATION      ##############################################

fcount = dcount = 0

def crawl():
    "downloads an entire wiki site"

    # tests ###############################################
    
    if os.system(QHELPCOMPILER +' -v'):
        print ("Error: QAssistant not fully installed, exiting.")
        return 1
    if os.system(QCOLLECTIOMGENERATOR +' -v'):
        print ("Error: QAssistant not fully installed, exiting.")
        return 1

    # run ########################################################

    qhp = buildtoc()
    qhcp = createCollProjectFile()
    shutil.copy("../../Gui/Icons/freecad-icon-64.png","localwiki/freecad-icon-64.png")
    if generate(qhcp) or compile(qhp):
        print ("Error at compiling")
        return 1
    if VERBOSE: print ("All done!")
    i=raw_input("Copy the files to their correct location in the source tree? y/n (default=no) ")
    if i.upper() in ["Y","YES"]:
        shutil.copy("localwiki/freecad.qch","../../Doc/freecad.qch")
        shutil.copy("localwiki/freecad.qhc","../../Doc/freecad.qhc")
    else:
        print ('Files are in localwiki. Test with "assistant -collectionFile localwiki/freecad.qhc"')
    return 0
    
def compile(qhpfile):
    "compiles the whole html doc with qassistant"
    qchfile = FOLDER + os.sep + "freecad.qch"
    if not os.system(QHELPCOMPILER + ' '+qhpfile+' -o '+qchfile):
        if VERBOSE: print ("Successfully created",qchfile)
        return 0

def generate(qhcpfile):
    "generates qassistant-specific settings like icon, title, ..."
    txt="""
<center>FreeCAD """+RELEASE+""" help files<br/>
<a href="http://www.freecadweb.org">http://www.freecadweb.org</a></center>
    """
    about=open(FOLDER + os.sep + "about.txt","w")
    about.write(txt)
    about.close()
    qhcfile = FOLDER + os.sep + "freecad.qhc"
    if not os.system(QCOLLECTIOMGENERATOR+' '+qhcpfile+' -o '+qhcfile):
        if VERBOSE: print ("Successfully created ",qhcfile)
        return 0

def createCollProjectFile():
    qprojectfile = '''<?xml version="1.0" encoding="UTF-8"?>
<QHelpCollectionProject version="1.0">
    <assistant>
        <title>FreeCAD User Manual</title>
        <applicationIcon>freecad-icon-64.png</applicationIcon>
        <cacheDirectory>freecad/freecad</cacheDirectory>
        <startPage>qthelp://org.freecad.usermanual/doc/Online_Help_Startpage.html</startPage>
        <aboutMenuText>
            <text>About FreeCAD</text>
        </aboutMenuText>
        <aboutDialog>
            <file>about.txt</file>
            <!--
            <icon>images/icon.png</icon>
            -->
            <icon>freecad-icon-64.png</icon>
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
    qfilename = FOLDER + os.sep + "freecad.qhcp"
    f = open(qfilename,'w')
    f.write(qprojectfile)
    f.close()
    if VERBOSE: print ("Done writing qhcp file",qfilename)
    return qfilename

def buildtoc():
    '''
    gets the table of contents page and parses its
    contents into a clean lists structure
    '''
    
    qhelpfile = '''<?xml version="1.0" encoding="UTF-8"?>
<QtHelpProject version="1.0">
    <namespace>org.freecad.usermanual</namespace>
    <virtualFolder>doc</virtualFolder>
    <!--
    <customFilter name="FreeCAD '''+RELEASE+'''">
        <filterAttribute>FreeCAD</filterAttribute>
        <filterAttribute>'''+RELEASE+'''</filterAttribute>
    </customFilter>
    -->
    <filterSection>
        <!--
        <filterAttribute>FreeCAD</filterAttribute>
        <filterAttribute>'''+RELEASE+'''</filterAttribute>
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
        if not link: link = 'default.html'
        return title,link

    if VERBOSE: print ("Building table of contents...")
    f = open(FOLDER+os.sep+INDEX+'.html')
    html = ''
    for line in f: html += line
    f.close()
    html = html.replace("\n"," ")
    html = html.replace("> <","><")
    html = re.findall("<ul.*/ul>",html)[0]
    items = re.findall('<li[^>]*>.*?</li>|</ul></li>',html)
    inserttoc = '<section title="FreeCAD Documentation" ref="Online_Help_Toc.html">\n'
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
    for fil in os.listdir(FOLDER):
        insertfiles += ("<file>"+fil+"</file>\n")
    insertfiles += "</files>\n"

    qhelpfile = re.compile('<insertkeywords>').sub(insertkeywords,qhelpfile)
    qhelpfile = re.compile('<inserttoc>').sub(inserttoc,qhelpfile)
    qhelpfile = re.compile('<insertfiles>').sub(insertfiles,qhelpfile)
    qfilename = FOLDER + os.sep + "freecad.qhp"
    f = open(qfilename,'wb')
    f.write(qhelpfile)
    f.close()
    if VERBOSE: print ("Done writing qhp file",qfilename)
    return qfilename
    
if __name__ == "__main__":
    crawl()

