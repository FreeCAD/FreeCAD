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

import os,FreeCAD,FreeCADGui,tempfile,time,zipfile,urllib,re,cStringIO
from PyQt4 import QtGui
from xml.etree.ElementTree import parse

FreeCADGui.addLanguagePath(":/translations")
FreeCADGui.updateLocale()

def translate(context,text):
    "convenience function for the Qt translator"
    # return str(QtGui.QApplication.translate(context, text, None, QtGui.QApplication.UnicodeUTF8).toUtf8())
    u = QtGui.QApplication.translate(context, text, None,
                                     QtGui.QApplication.UnicodeUTF8).toUtf8()
    s = cStringIO.StringIO()
    for i in u:
        if ord(i) == 39:
            s.write("\\'")
        else:
            s.write(i)
    t = s.getvalue()
    s.close()
    return t

# texts to be translated

text01 = translate("StartPage","FreeCAD Start Center")
text02 = translate("StartPage","Start a new project")
text03 = translate("StartPage","Recent Files")
text04 = translate("StartPage","Latest videos")
text05 = translate("StartPage","Latest news")
text06 = translate("StartPage","On the web")
text07 = translate("StartPage","This is the FreeCAD Homepage. Here you will be able to find a lot of information about FreeCAD, including tutorials, examples and user documentation.")
text08 = translate("StartPage","FreeCAD Homepage")
text09 = translate("StartPage","Example projects")
text10 = translate("StartPage","Schenkel STEP file")
text11 = translate("StartPage","Load a PartDesign example")
text12 = translate("StartPage","Load a Drawing extraction")
text13 = translate("StartPage","Load a Robot simulation example")
text14 = translate("StartPage","Projects from the Web")
text15 = translate("StartPage","Schenkel STEP")
text16 = translate("StartPage","Complex Part")
text17 = translate("StartPage","Close this window after opening or creating a file")
text18 = translate("StartPage","Don't show me this window again next time")
text19 = translate("StartPage","Designing parts")
text20 = translate("StartPage","The <b>Part Design</b> workbench is designed to create complex pieces based on constrained 2D sketches. Use it to draw 2D shapes, constrain some of their elements and extrude them to form 3D pieces.")
text21 = translate("StartPage","Example workflow")
text22 = translate("StartPage","Part Design")
text23 = translate("StartPage","Designing architectural elements")
text24 = translate("StartPage","The <b>Architectural Design</b> workbench is specially designed for working with architectural elements such as walls or windows. Start by drawing 2D shapes, and use them as guides to build architecutral objects.")
text25 = translate("StartPage","Architectual Design")
text26 = translate("StartPage","Working with Meshes")
text27 = translate("StartPage","The <b>Mesh Workbench</b> is used to work with Mesh objects. Meshes are simpler 3D objects than Part objects, but they are often easier to import and export to/from other applications.")
text28 = translate("StartPage","FreeCAD offers you several tools to convert between Mesh and Part objects.")
text29 = translate("StartPage","Work with Meshes")
text30 = translate("StartPage","The complete workbench")
text31 = translate("StartPage","FreeCAD Complete workbench")
text32 = translate("StartPage","populated with some of the most commonly used tools.")
text33 = translate("StartPage","file size:")
text34 = translate("StartPage","creation time:")
text35 = translate("StartPage","last modified:")
text36 = translate("StartPage","location:")
text37 = translate("StartPage","User manual")
text38 = translate("StartPage","http://www.freecadweb.org/wiki/index.php?title=Online_Help_Toc")
text39 = translate("StartPage","Tutorials")
text40 = translate("StartPage","Python resources")
text41 = translate("StartPage","File not found")
text42 = translate("StartPage","from <a href=http://twitter.com/FreeCADNews>@FreeCADNews</a>")
text43 = translate("StartPage","The FreeCAD-tutorial blog")
text44 = translate("StartPage","from <a href=http://www.youtube.com/user/FreeCADNews?feature=mhee>FreeCADNews channel</a>")
text45 = translate("StartPage","This is the official user manual of FreeCAD, built, maintained and translated by the FreeCAD community.")
text46 = translate("StartPage","The tutorials section on the FreeCAD website")
text47 = translate("StartPage","The section of the FreeCAd website dedicate dto python scripting, with examples, explanations, and API commands.")
text48 = translate("StartPage","A blog dedicated to teaching FreeCAD, maintained by members of the FreeCAD community")
text49 = translate("StartPage","Getting started")
text50 = translate("StartPage","The FreeCAD interface is divided in workbenches, which are sets of tools suited for a specific task. You can start with one of the workbenches in this list, or with the complete workbench, which presents you with some of the most used tools gathered from other workbenches. Click to read more about workbenches on the FreeCAD website.")
text51 = translate("StartPage","http://www.freecadweb.org/wiki/index.php?title=Workbench_Concept")
text52 = translate("StartPage","Ship Design")
text53 = translate("StartPage","Designing and calculating ships")
text54 = translate("StartPage","The <b>Ship Design</b> module offers several tools to help ship designers to view, model and calculate profiles and other specific properties of ship hulls.")
text55 = translate("StartPage","Load an Architectural example model")

# here is the html page skeleton

page = """
<html>
  <head>
    <title>FreeCAD - Start page</title>

    <script language="javascript">

        function JSONscriptRequest(fullUrl) {
            // REST request path
            this.fullUrl = fullUrl; 
            // Get the DOM location to put the script tag
            this.headLoc = document.getElementsByTagName("head").item(0);
            // Generate a unique script tag id
            this.scriptId = 'JscriptId' + JSONscriptRequest.scriptCounter++;
        }

        // Static script ID counter
        JSONscriptRequest.scriptCounter = 1;

        JSONscriptRequest.prototype.buildScriptTag = function () {
            // Create the script tag
            this.scriptObj = document.createElement("script");
            // Add script object attributes
            this.scriptObj.setAttribute("type", "text/javascript");
            this.scriptObj.setAttribute("charset", "utf-8");
            this.scriptObj.setAttribute("src", this.fullUrl);
            this.scriptObj.setAttribute("id", this.scriptId);
        }
 
        JSONscriptRequest.prototype.removeScriptTag = function () {
            // Destroy the script tag
            this.headLoc.removeChild(this.scriptObj);  
        }

        JSONscriptRequest.prototype.addScriptTag = function () {
            // Create the script tag
            this.headLoc.appendChild(this.scriptObj);
        }

        function show(theText) {
            ddiv = document.getElementById("description");
            if (theText == "") theText = "&nbsp;";
            ddiv.innerHTML = theText;
        }

        function loadFeeds() {
            ddiv = document.getElementById("youtube");
            ddiv.innerHTML = "Fetching data from the web...";
            var obj=new JSONscriptRequest('http://gdata.youtube.com/feeds/base/users/FreeCADNews/favorites?alt=json-in-script&v=2&orderby=published&callback=showLinks');
            obj.buildScriptTag(); // Build the script tag
            obj.addScriptTag(); // Execute (add) the script tag
            ddiv.innerHTML = "Done fetching";
            ddiv = document.getElementById("news");
            ddiv.innerHTML = "Fetching data from the web...";
            var tobj=new JSONscriptRequest('http://pipes.yahoo.com/pipes/pipe.run?_id=da8b612e97a6bb4588b1ce27db30efd9&_render=json&_callback=showTweets');
            tobj.buildScriptTag(); // Build the script tag
            tobj.addScriptTag(); // Execute (add) the script tag
            ddiv.innerHTML = "Done fetching";
        }

        function showLinks(data) {
            ddiv = document.getElementById('youtube');
            ddiv.innerHTML = "Received";
            var feed = data.feed;
            var entries = feed.entry || [];
            var html = ['<ul>'];
            for (var i = 0; i < 5; i++) {
                html.push('<li><a href="',entries[i].link[0].href,'">', entries[i].title.$t, '</a></li>');
            }
            html.push('</ul>');
            ddiv.innerHTML = html.join('');
        }

        function showTweets(data) {
            ddiv = document.getElementById('news');
            ddiv.innerHTML = "Received";
            var html = ['<ul>'];
            for (var i = 0; i < 8; i++) {
                html.push('<li><a href="', data.value.items[i].link, '">', data.value.items[i].title, '</a></li>');
            }
            html.push('</ul>');
            ddiv.innerHTML = html.join('');
        }

    </script>

    <style type="text/css">

        body {
            background: #171A2B url(Background.jpg);
            color: white;
            font-family: Arial, Helvetica, Sans;
            font-size: 11px;
        }

        a {
            color: #0092E8;
            font-weight: bold;
            text-decoration: none;
            padding: 2px;
        }

        a:hover {
            color: white;
            background: #0092E8;
            border-radius: 5px;
        }

        p {
            text-align: justify;
        }

        .left {
            text-align: left;
        }

        h1 {
            font-size: 3em;
            letter-spacing: 2px;
            padding: 20px 0 0 80px;
            align: bottom;
        }

        h2 {
            font-size: 1.2em;
        }

        ul {
            list-style-type: none;
            padding: 0;
        }

        .column {
            width: 300px;
            float: left;
            margin-left: 10px;
        }

        .block {
            background: rgba(30,31,33,0.6);;
            border-radius: 5px;
            padding: 8px;
            margin-bottom: 10px;
        }

        .options {
            clear: both;
        }

        .from {
            font-size: 0.7em;
            font-weight: normal;
        }

    </style>

  </head>

  <body onload="loadFeeds()">

    <h1><img src="FreeCAD.png">&nbsp;""" + text01 + """</h1>

    <div class="column">

      <div class="block">
        <h2>""" + text02 + """</h2>
          defaultworkbenches
      </div>

      <div class="block">
        <h2>""" + text03 + """</h2>
          recentfiles
      </div>

      <div class="block">
        <h2>""" + text04 + """</h2>
        <div id="youtube">youtube videos</div>
      </div>

      <div class="block">
        <h2>""" + text05 + """</h2>
        <div id="news">news feed</div>
      </div>

    </div>

    <div class="column">

      <div class="block">
        <h2>""" + text06 + """</h2>
            defaultlinks
      </div>

      <div class="block">
        <h2>""" + text09 + """</h2>
            defaultexamples
      </div>


      customblocks

    </div>

    <div class="column" id="description">
      &nbsp;
    </div>

    <!--
    <form class="options">
      <input type="checkbox" name="closeThisDialog">
      """ + text17 + """<br/>
      <input type="checkbox" name="dontShowAgain">
      """ + text18 + """
    </form>
    -->

  </body>
</html>
"""

def getWebExamples():
    return """
    <ul>
        <li><a href="http://freecad-project.de/svn/ExampleData/FileFormates/Schenkel.stp">""" + text15 + """</a></li>
        <li><a href="http://freecad-project.de/svn/ExampleData/Examples/CAD/Complex.FCStd">""" + text16 + """</a></li>
    </ul>"""
      
def getExamples():
    return """
    <ul>
        <li><img src="FreeCAD.png" style="width: 16px">&nbsp;<a href="LoadSchenkel.py">""" + text10 + """</a></li>
        <li><img src="FreeCAD.png" style="width: 16px">&nbsp;<a href="LoadPartDesignExample.py">""" + text11 + """</a></li>
        <li><img src="FreeCAD.png" style="width: 16px">&nbsp;<a href="LoadDrawingExample.py">""" + text12 + """</a></li>
        <li><img src="FreeCAD.png" style="width: 16px">&nbsp;<a href="LoadRobotExample.py">""" + text13 + """</a></li>
        <li><img src="FreeCAD.png" style="width: 16px">&nbsp;<a href="LoadArchExample.py">""" + text55 + """</a></li>
    </ul>"""
      
def getLinks():
    return """
    <ul>
        <li><img src="web.png">&nbsp;
            <a onMouseover="show('<p>""" + text07 + """</p>')" 
                onMouseout="show('')"
                href="http://www.freecadweb.org/">""" + text08 + """</a></li>
        <li><img src="web.png">&nbsp;
            <a onMouseover="show('<p>""" + text45 + """</p>')" 
                onMouseout="show('')"
                href=""" + text38 + """>""" + text37 + """</a></li>
        <li><img src="web.png">&nbsp;
            <a onMouseover="show('<p>""" + text46 + """</p>')" 
                onMouseout="show('')"
                href="http://www.freecadweb.org/wiki/index.php?title=Tutorials">""" + text39 + """</a></li>
        <li><img src="web.png">&nbsp;
            <a onMouseover="show('<p>""" + text47 + """</p>')" 
                onMouseout="show('')"
                href="http://www.freecadweb.org/wiki/index.php?title=Power_users_hub">""" + text40 + """</a></li>
        <li><img src="web.png">&nbsp;
            <a onMouseover="show('<p>""" + text48 + """</p>')" 
                onMouseout="show('')"
                href="http://freecad-tutorial.blogspot.com/">""" + text43 + """</a></li>
    </ul>"""

def getWorkbenches():
    return """
    <ul>
        <li><img src="blank.png">&nbsp;
            <a onMouseover="show('<h3>""" + text49 + """</h3> \
            <p>""" + text50 + """</p>')" 
            onMouseout="show('')" 
            href=""" + text51 + """>""" + text49 + """</a>
        </li>
        <li><img src="PartDesign.png">&nbsp;
            <a onMouseover="show('<h3>""" + text19 + """</h3> \
            <p>""" + text20 + """</p><p><small>""" + text21 + """ \
            :</small></p><img src=PartDesignExample.png>')" 
            onMouseout="show('')" 
            href="PartDesign.py">""" + text22 + """</a>
        </li>
        <li><img src="ArchDesign.png">&nbsp;
          <a onMouseover="show('<h3>""" + text23 + """</h3> \
            <p>""" + text24 + """</p><p><small>""" + text21 + """ \
            :</small></p><img src=ArchExample.png>')" 
            onMouseout="show('')"
            href="ArchDesign.py">""" + text25 + """</a>
        </li>
        <li><img src="Ship.png">&nbsp;
          <a onMouseover="show('<h3>""" + text53 + """</h3> \
            <p>""" + text54 + """</p><p><small>""" + text21 + """ \
            :</small></p><img src=ShipExample.png>')" 
            onMouseout="show('')"
            href="Ship.py">""" + text52 + """</a>
        </li>
        <li><img src="Mesh.png">&nbsp;
            <a onMouseover="show('<h3>""" + text26 + """</h3> \
            <p>""" + text27 + """</p><p>""" + text28 + """</p>')" 
            onMouseout="show('')" 
            href="Mesh.py">""" + text29 + """</a>
        </li>
        <li><img src="Complete.png">&nbsp;
            <a onMouseover="show('<h3>""" + text30 +"""</h3> \
            <p>This is the <b>""" + text31 + """</b>, \
            """ + text32 + """</p><img src=complete.jpg>')" 
            onMouseout="show('')" 
            href="DefaultWorkbench.py">""" + text31 + """</a>
        </li>
    </ul>"""

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
                html += "<p>FreeCAD Standard File</p>"
                image="thumbnails/Thumbnail.png"
                if image in files:
                    image=zfile.read(image)
                    thumbfile = tempfile.mkstemp(suffix='.png')[1]
                    thumb = open(thumbfile,"wb")
                    thumb.write(image)
                    thumb.close()
                    html += '<img src=file://'

                    html += thumbfile + '><br/>'
    else:
        html += "<p>" + text41 + "</p>"
            
    return html

def getRecentFiles():
    "returns a list of 3 latest recent files"
    rf = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
    ct = rf.GetInt("RecentFiles")
    html = '<ul>'
    for i in range(3):
        if i < ct:
            mr = rf.GetString("MRU%d" % (i))
            fn = os.path.basename(mr)
            html += '<li>'
            if mr[-5:].upper() == "FCSTD":
                html += '<img src="FreeCAD.png" style="width: 16px">&nbsp;'
            else:
                html += '<img src="blank.png" style="width: 16px">&nbsp;'
            html += '<a '
            html += 'onMouseover="show(\''+getInfo(mr)+'\')" '
            html += 'onMouseout="show(\'\')" '
            html += 'href="LoadMRU'+str(i)+'.py">'
            html += fn
            html += '</a></li>'
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
    print resp
    return resp

def getCustomBlocks():
    "fetches custom html files in FreeCAD user dir"
    output = ""
    return output

def handle():
    "returns the complete html startpage"
    
    # add recent files
    recentfiles = getRecentFiles()
    html = page.replace("recentfiles",recentfiles)

    # add default workbenches
    html = html.replace("defaultworkbenches",getWorkbenches())

    # add default web links
    html = html.replace("defaultlinks",getLinks())

    # add default examples
    html = html.replace("defaultexamples",getExamples())

    # add web examples
    #html = html.replace("webexamples",getWebExamples())

    # add custom blocks
    html = html.replace("customblocks",getCustomBlocks())
    
    return html

