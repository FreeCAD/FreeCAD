#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012                                                    * 
#*   Yorik van Havre <yorik@uncreated.net>                                 * 
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

# This is the start page template

import os,FreeCAD,FreeCADGui,tempfile,time,zipfile,urllib,re
from PySide import QtGui
from xml.etree.ElementTree import parse

from .TranslationTexts import (text01, text02, text03, text04, text05, text06,
                              text07, text08, text09, text10, text11, text12,
                              text13, text14, text15, text16, text17, text18,
                              text19, text20, text21, text22, text23, text24,
                              text25, text26, text27, text28, text29, text30,
                              text31, text32, text33, text34, text35, text36,
                              text37, text38, text39, text40, text41, text42,
                              text43, text44, text45, text46, text47, text48,
                              text49, text50, text51, text52, text53, text54,
                              text55, text56, text57, text58, text59, text60,
                              text61, text62, text63, text64, text65, text66,
                              text67, text68, text69, text70, text71)

try:
    import io as cStringIO
except:
    import cStringIO

FreeCADGui.addLanguagePath(":/translations")
FreeCADGui.updateLocale()

# get FreeCAD version
v = FreeCAD.Version()
vmajor, vminor = v[0], v[1]
vbuild = v[2].split(" ")[0]

# here is the html page skeleton
resources_dir = os.path.join(FreeCAD.getResourceDir(), "Mod", "Start", "StartPage")
html_filename = os.path.join(resources_dir, "StartPage.html")
js_filename = os.path.join(resources_dir, "StartPage.js")
css_filename = os.path.join(resources_dir, "StartPage.css")

with open(html_filename, 'r') as f:
    startpage_html = f.read()

with open(js_filename, 'r') as f:
    startpage_js = f.read()

with open(css_filename, 'r') as f:
    startpage_css = f.read()

def getInfo(filename):
    "returns available file information"

    def getLocalTime(timestamp):
        "returns a local time from a timestamp"       
        return time.strftime("%m/%d/%Y %H:%M:%S",time.localtime(timestamp))

    def getSize(size):
        "returns a human-readable size" 
        if size > 1024*1024:
            hsize = str(size/(1024*1024)) + "Mb"
        elif size > 1024:
            hsize = str(size/1024) + "Kb"
        else:
            hsize = str(size) + "b"
        return hsize
        
    html = '<h3>'+os.path.basename(filename)+'</h3>'
    
    if os.path.exists(filename):
        # get normal file info
        s = os.stat(filename)
        html += "<p>" + text33 + " " + getSize(s.st_size) + "<br/>"
        html += text34 + " " + getLocalTime(s.st_ctime) + "<br/>"
        html += text35 + " " + getLocalTime(s.st_mtime) + "<br/>"
        html += "<span>" + text36 + " " + filename + "</span></p>"
        # get additional info from fcstd files
        if os.path.splitext(filename)[1].upper() in [".FCSTD"]:
            zfile=zipfile.ZipFile(filename)
            files=zfile.namelist()
            # check for meta-file if it's really a FreeCAD document
            if files[0] == "Document.xml":
                html += "<p><b>" + text65 + "</b></p>"
                image="thumbnails/Thumbnail.png"
                doc = str(zfile.read(files[0]))
                doc = doc.replace("\n"," ")
                author = re.findall("Property name=\"CreatedBy.*?String value=\"(.*?)\"\/>",doc)
                if author: 
                    html += "<p>" + text66 + ": " + author[0] + "</p>"
                company = re.findall("Property name=\"Company.*?String value=\"(.*?)\"\/>",doc)
                if company: 
                    html += "<p>" + text67 + ": " + company[0] + "</p>"
                lic = re.findall("Property name=\"License.*?String value=\"(.*?)\"\/>",doc)
                if lic: 
                    html += "<p>" + text68 + ": " + lic[0] + "</p>"
                if image in files:
                    image=zfile.read(image)
                    thumbfile = tempfile.mkstemp(suffix='.png')[1]
                    thumb = open(thumbfile,"wb")
                    thumb.write(image)
                    thumb.close()
                    html += '<img src=file://'
                    html += thumbfile + '><br/>'
        else:
            print ("not a freecad file: "+os.path.splitext(filename)[1].upper())
    else:
        html += "<p>" + text41 + "</p>"
    return html

def getRecentFiles():
    "returns a list of 3 latest recent files"
    rf = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
    ct = rf.GetInt("RecentFiles")
    html = '<ul>'
    for i in range(4):
        if i < ct:
            mr = rf.GetString("MRU%d" % (i))
            if os.path.exists(mr):
                fn = os.path.basename(mr)
                html += '<li>'
                if mr[-5:].upper() == "FCSTD":
                    html += '<img src="images/freecad-doc.png" style="width: 16px">&nbsp;'
                else:
                    html += '<img src="images/blank.png" style="width: 16px">&nbsp;'
                html += '<a '
                html += 'onMouseover="show(\''+getInfo(mr).replace("'","&rsquo;")+'\')" '
                html += 'onMouseout="show(\'\')" '
                html += 'href="LoadMRU'+str(i)+'.py">'
                html += fn
                html += '</a></li>'
            else:
                fn = os.path.basename(mr)
                html += '<li>'
                if mr[-5:].upper() == "FCSTD":
                    html += '<img src="images/freecad-doc.png" style="width: 16px">&nbsp;'
                else:
                    html += '<img src="images/blank.png" style="width: 16px">&nbsp;'
                html += '<span class="disabled">'
                html += fn
                html += '</span></li>'
                
    html += '</ul>'
    return html

def getFeed(url,numitems=3):
    "returns a html list with links from the given RSS feed url"
    xml = parse(urllib.urlopen(url)).getroot()
    items = []
    channel = xml.find('channel')
    for element in channel.findall('item'):
        items.append({'title': element.find('title').text,
                      'description': element.find('description').text,
                      'link': element.find('link').text})
    if len(items) > numitems:
        items = items[:numitems]
    resp = '<ul>'
    for item in items:
        descr = re.compile("style=\".*?\"").sub('',item['description'])
        descr = re.compile("alt=\".*?\"").sub('',descr)
        descr = re.compile("\"").sub('',descr)
        d1 = re.findall("<img.*?>",descr)[0]
        d2 = re.findall("<span>.*?</span>",descr)[0]
        descr = "<h3>" + item['title'] + "</h3>"
        descr += d1 + "<br/>"
        descr += d2
        resp += '<li><a onMouseover="show(\''
        resp += descr
        resp += '\')" onMouseout="show(\'\')" href="'
        resp += item['link']
        resp += '">'
        resp += item['title']
        resp += '</a></li>'
    resp += '</ul>'
    print(resp)
    return resp

def getCustomBlocks():
    "fetches custom html files in FreeCAD user dir"
    output = ""
    return output

def setColors(html):
    "gets theme colors from the system, and sets appropriate styles"
    defaults = {"#basecolor":"#191B26",
                "#linkcolor":"#0092E8",
                "#textcolor":"#FFFFFF",
                "#windowcolor":"#FFFFFF",
                "#windowtextcolor":"#000000"}
    try:
        palette = QtGui.qApp.palette()
    except:
        pass
    else:
        #defaults["#basecolor"] = palette.base().color().name()
        defaults["#basecolor"] = "#171A2B url(images/Background.jpg)"
        #defaults["#linkcolor"] = palette.link().color().name() # UGLY!!
        defaults["#textcolor"] = palette.text().color().name()
        defaults["#windowcolor"] = palette.window().color().name()
        defaults["#windowtextcolor"] = palette.windowText().color().name()
    for k,v in defaults.items():
        html = html.replace(k,str(v))
    return html

def insert_page_resources(html):
    html = html.replace("startpage_js", startpage_js)
    html = html.replace("startpage_css", startpage_css)
    return html

def replace_html_text(html):
    html = html.replace("text01", text01)
    html = html.replace("text02", text02)
    html = html.replace("text03", text03)
    html = html.replace("text05", text05)
    html = html.replace("text06", text06)
    html = html.replace("text07", text07)
    html = html.replace("text08", text08)
    html = html.replace("text09", text09)
    html = html.replace("text10", text10)
    html = html.replace("text11", text11)
    html = html.replace("text12", text12)
    html = html.replace("text13", text13)
    html = html.replace("text17", text17)
    html = html.replace("text18", text18)
    html = html.replace("text19", text19)
    html = html.replace("text20", text20)
    html = html.replace("text21", text21)
    html = html.replace("text22", text22)
    html = html.replace("text23", text23)
    html = html.replace("text24", text24)
    html = html.replace("text25", text25)
    html = html.replace("text26", text26)
    html = html.replace("text27", text27)
    html = html.replace("text28", text28)
    html = html.replace("text29", text29)
    html = html.replace("text37", text37)
    html = html.replace("text38", text38)
    html = html.replace("text39", text39)
    html = html.replace("text40", text40)
    html = html.replace("text43", text43)
    html = html.replace("text45", text45)
    html = html.replace("text46", text46)
    html = html.replace("text47", text47)
    html = html.replace("text48", text48)
    html = html.replace("text49", text49)
    html = html.replace("text50", text50)
    html = html.replace("text51", text51)
    html = html.replace("text52", text52)
    html = html.replace("text53", text53)
    html = html.replace("text54", text54)
    html = html.replace("text55", text55)
    html = html.replace("text56", text56)
    html = html.replace("text57", text57)
    html = html.replace("text60", text60)
    html = html.replace("text61", text61)
    html = html.replace("text62", text62)
    html = html.replace("text64", text64)
    html = html.replace("text69", text69)
    return html

def replace_js_text(html):
    html = html.replace("vmajor", vmajor)
    html = html.replace("vminor", vminor)
    html = html.replace("vbuild", vbuild)
    html = html.replace("text58", text58)
    html = html.replace("text59", text59)
    html = html.replace("text63", text63)
    html = html.replace("text70", text70)
    html = html.replace("text71", text71)
    return html

def handle():
    "returns the complete html startpage"
    # add strings into files
    html = insert_page_resources(startpage_html)
    html = replace_js_text(html)
    
    # add recent files
    recentfiles = getRecentFiles()
    html = html.replace("recentfiles",recentfiles)
        
    # add custom blocks
    html = html.replace("customblocks",getCustomBlocks())
    
    # enable web access if permitted
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("AllowDownload",False):
        html = html.replace("var allowDownloads = 0;","var allowDownloads = 1;")

    html = replace_html_text(html)
    # fetches system colors
    html = setColors(html)
    return html

def exportTestFile():
    f = open(os.path.expanduser("~")+os.sep+"freecad-startpage.html","wb")
    f.write(handle())
    f.close()

