#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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


# This is the start page template. It builds a HTML global variable that contains
# the html code of the start page. It is built only once per FreeCAD session for now...

import sys,os,FreeCAD,FreeCADGui,tempfile,time,zipfile,urllib,re
from . import TranslationTexts
from PySide import QtCore,QtGui

FreeCADGui.addLanguagePath(":/translations")
FreeCADGui.updateLocale()

iconprovider = QtGui.QFileIconProvider()
iconbank = {} # to store already created icons so we don't overpollute the temp dir
tempfolder = None # store icons inside a subfolder in temp dir
defaulticon = None # store a default icon for problematic file types


def encode(text):

    "make sure we are always working with unicode in python2"

    if sys.version_info.major < 3:
        if not isinstance(text,unicode):
            return text.decode("utf8")
    return text


def gethexcolor(color):

    "returns a color hex value #000000"

    r = str(hex(int(((color>>24)&0xFF))))[2:].zfill(2)
    g = str(hex(int(((color>>16)&0xFF))))[2:].zfill(2)
    b = str(hex(int(((color>>8)&0xFF))))[2:].zfill(2)
    return "#"+r+g+b



def isOpenableByFreeCAD(filename):

    "check if FreeCAD can handle this file type"

    if os.path.isdir(filename):
        return False
    if os.path.basename(filename)[0] == ".":
        return False
    extensions = [key.lower() for key in FreeCAD.getImportType().keys()]
    ext = os.path.splitext(filename)[1].lower()
    if ext:
        if ext[0] == ".":
            ext = ext[1:]
    if ext in extensions:
        return True
    return False



def getInfo(filename):

    "returns available file information"

    global iconbank,tempfolder

    tformat = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString("TimeFormat","%m/%d/%Y %H:%M:%S")

    def getLocalTime(timestamp):
        "returns a local time from a timestamp"
        return time.strftime(tformat,time.localtime(timestamp))

    def getSize(size):
        "returns a human-readable size"
        if size > 1024*1024:
            hsize = str(int(size/(1024*1024))) + "Mb"
        elif size > 1024:
            hsize = str(int(size/1024)) + "Kb"
        else:
            hsize = str(int(size)) + "b"
        return hsize

    if os.path.exists(filename):

        if os.path.isdir(filename):
            return None

        # get normal file info
        s = os.stat(filename)
        size = getSize(s.st_size)
        ctime = getLocalTime(s.st_ctime)
        mtime = getLocalTime(s.st_mtime)
        author = ""
        company = TranslationTexts.T_UNKNOWN
        lic = TranslationTexts.T_UNKNOWN
        image = None
        descr = ""

        # get additional info from fcstd files
        if filename.lower().endswith(".fcstd"):
            try:
                zfile=zipfile.ZipFile(filename)
            except:
                print("Cannot read file: ",filename)
                return None
            files=zfile.namelist()
            # check for meta-file if it's really a FreeCAD document
            if files[0] == "Document.xml":
                doc = str(zfile.read(files[0]))
                doc = doc.replace("\n"," ")
                r = re.findall("Property name=\"CreatedBy.*?String value=\"(.*?)\"\/>",doc)
                if r:
                    author = r[0]
                r = re.findall("Property name=\"Company.*?String value=\"(.*?)\"\/>",doc)
                if r:
                    company = r[0]
                r = re.findall("Property name=\"License.*?String value=\"(.*?)\"\/>",doc)
                if r:
                    lic = r[0]
                r = re.findall("Property name=\"Comment.*?String value=\"(.*?)\"\/>",doc)
                if r:
                    descr = r[0]
                if "thumbnails/Thumbnail.png" in files:
                    if filename in iconbank:
                        image = iconbank[filename]
                    else:
                        imagedata=zfile.read("thumbnails/Thumbnail.png")
                        image = tempfile.mkstemp(dir=tempfolder,suffix='.png')[1]
                        thumb = open(image,"wb")
                        thumb.write(imagedata)
                        thumb.close()
                        iconbank[filename] = image

        # retrieve default mime icon if needed
        if not image:
            i = QtCore.QFileInfo(filename)
            t = iconprovider.type(i)
            if not t:
                t = "Unknown"
            if t in iconbank:
                image = iconbank[t]
            else:
                icon = iconprovider.icon(i)
                if icon.availableSizes():
                    preferred = icon.actualSize(QtCore.QSize(128,128))
                    px = icon.pixmap(preferred)
                    image = tempfile.mkstemp(dir=tempfolder,suffix='.png')[1]
                    px.save(image)
                else:
                    image = getDefaultIcon()
                iconbank[t] = image

        return [image,size,author,ctime,mtime,descr,company,lic]

    return None



def getDefaultIcon():

    "retrieves or creates a default file icon"

    global defaulticon

    if not defaulticon:
        i = QtCore.QFileInfo("Unknown")
        icon = iconprovider.icon(i)
        preferred = icon.actualSize(QtCore.QSize(128,128))
        px = icon.pixmap(preferred)
        image = tempfile.mkstemp(dir=tempfolder,suffix='.png')[1]
        px.save(image)
        defaulticon = image

    return defaulticon



def buildCard(filename,method,arg=None):

    "builds a html <li> element representing a file. method is a script + a keyword, for ex. url.py?key="

    result = encode("")
    if os.path.exists(filename) and isOpenableByFreeCAD(filename):
        basename = os.path.basename(filename)
        if not arg:
            arg = basename
        finfo = getInfo(filename)
        if finfo:
            image = finfo[0]
            size = finfo[1]
            author = finfo[2]
            infostring = encode(TranslationTexts.T_CREATIONDATE+": "+finfo[3]+"\n")
            infostring += encode(TranslationTexts.T_LASTMODIFIED+": "+finfo[4])
            if finfo[5]:
                infostring += "\n\n" + encode(finfo[5])
            if size:
                result += '<a href="'+method+arg+'" title="'+infostring+'">'
                result += '<li class="icon">'
                result += '<img src="file:///'+image+'">'
                result += '<div class="caption">'
                result += '<h4>'+encode(basename)+'</h4>'
                result += '<p>'+size+'</p>'
                result += '<p>'+encode(author)+'</p>'
                result += '</div>'
                result += '</li>'
                result += '</a>'
    return result



def handle():

    "builds the HTML code of the start page"

    global iconbank,tempfolder

    # reuse stuff from previous runs to reduce temp dir clutter

    import Start
    if hasattr(Start,"iconbank"):
        iconbank = Start.iconbank
    if hasattr(Start,"tempfolder"):
        tempfolder = Start.tempfolder
    else:
        tempfolder = tempfile.mkdtemp(prefix="FreeCADStartThumbnails")

    # build the html page skeleton

    resources_dir = os.path.join(FreeCAD.getResourceDir(), "Mod", "Start", "StartPage")
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
    template = p.GetString("Template","")
    if template:
        html_filename = template
    else:
        html_filename = os.path.join(resources_dir, "StartPage.html")
    js_filename = os.path.join(resources_dir, "StartPage.js")
    css_filename = os.path.join(resources_dir, "StartPage.css")
    with open(html_filename, 'r') as f:
        HTML = f.read()
    with open(js_filename, 'r') as f:
        JS = f.read()
    with open(css_filename, 'r') as f:
        CSS = f.read()
    HTML = HTML.replace("JS",JS)
    HTML = HTML.replace("CSS",CSS)
    HTML = encode(HTML)

    # get the stylesheet if we are using one

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("UseStyleSheet",False):
        qssfile = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow").GetString("StyleSheet","")
        if qssfile:
            with open(qssfile, 'r') as f:
                ALTCSS = encode(f.read())
            HTML = HTML.replace("<!--QSS-->","<style type=\"text/css\">"+ALTCSS+"</style>")

    # get FreeCAD version

    v = FreeCAD.Version()
    VERSIONSTRING = encode(TranslationTexts.T_VERSION + " " + v[0] + "." + v[1] + " " + TranslationTexts.T_BUILD + " " + v[2])
    HTML = HTML.replace("VERSIONSTRING",VERSIONSTRING)

    # translate texts

    texts = [t for t in dir(TranslationTexts) if t.startswith("T_")]
    for text in texts:
        HTML = HTML.replace(text,encode(getattr(TranslationTexts,text)))

    # build a "create new" icon with the FreeCAD background color gradient

    if not "createimg" in iconbank:
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        c1 = gethexcolor(p.GetUnsigned("BackgroundColor2"))
        c2 = gethexcolor(p.GetUnsigned("BackgroundColor3"))
        gradient = QtGui.QLinearGradient(0, 0, 0, 128)
        gradient.setColorAt(0.0, QtGui.QColor(c1))
        gradient.setColorAt(1.0, QtGui.QColor(c2))
        i = QtGui.QImage(128,128,QtGui.QImage.Format_RGB16)
        pa = QtGui.QPainter(i)
        pa.fillRect(i.rect(),gradient)
        pa.end()
        createimg = tempfile.mkstemp(dir=tempfolder,suffix='.png')[1]
        i.save(createimg)
        iconbank["createimg"] = createimg

    # build SECTION_RECENTFILES

    SECTION_RECENTFILES = encode("")
    rf = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
    rfcount = rf.GetInt("RecentFiles",0)
    SECTION_RECENTFILES = encode("<h2>"+TranslationTexts.T_RECENTFILES+"</h2>")
    SECTION_RECENTFILES += "<ul>"
    SECTION_RECENTFILES += '<a href="LoadNew.py" title="'+encode(TranslationTexts.T_CREATENEW)+'">'
    SECTION_RECENTFILES += '<li class="icon">'
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("NewFileGradient",False):
        SECTION_RECENTFILES += '<img src="file:///'+encode(iconbank["createimg"])+'">'
    else:
        SECTION_RECENTFILES += '<img src="file:///'+os.path.join(resources_dir, "images/new_file_thumbnail.svg")+'">'
    SECTION_RECENTFILES += '<div class="caption">'
    SECTION_RECENTFILES += '<h4>'+encode(TranslationTexts.T_CREATENEW)+'</h4>'
    SECTION_RECENTFILES += '</div>'
    SECTION_RECENTFILES += '</li>'
    SECTION_RECENTFILES += '</a>'
    for i in range(rfcount):
        filename = rf.GetString("MRU%d" % (i))
        SECTION_RECENTFILES += encode(buildCard(filename,method="LoadMRU.py?MRU=",arg=str(i)))
    SECTION_RECENTFILES += '</ul>'
    HTML = HTML.replace("SECTION_RECENTFILES",SECTION_RECENTFILES)

    # build SECTION_EXAMPLES

    SECTION_EXAMPLES = encode("")
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowExamples",True):
        SECTION_EXAMPLES = encode("<h2>"+TranslationTexts.T_EXAMPLES+"</h2>")
        SECTION_EXAMPLES += "<ul>"
        examples_path = FreeCAD.getResourceDir()+"examples"
        if os.path.exists(examples_path):
            examples = os.listdir(examples_path)
            for basename in examples:
                filename = FreeCAD.getResourceDir()+"examples"+os.sep+basename
                SECTION_EXAMPLES += encode(buildCard(filename,method="LoadExample.py?filename="))
        SECTION_EXAMPLES += "</ul>"
    HTML = HTML.replace("SECTION_EXAMPLES",SECTION_EXAMPLES)

    # build SECTION_CUSTOM

    SECTION_CUSTOM = encode("")
    cfolder = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString("ShowCustomFolder","")
    if cfolder:
        if not os.path.isdir(cfolder):
            cfolder = os.path.dirname(cfolder)
        SECTION_CUSTOM = encode("<h2>"+os.path.basename(os.path.normpath(cfolder))+"</h2>")
        SECTION_CUSTOM += "<ul>"
        for basename in os.listdir(cfolder):
            filename = os.path.join(cfolder,basename)
            SECTION_CUSTOM += encode(buildCard(filename,method="LoadCustom.py?filename="))
        SECTION_CUSTOM += "</ul>"
    HTML = HTML.replace("SECTION_CUSTOM",SECTION_CUSTOM)

	# build IMAGE_SRC paths
    HTML = HTML.replace("IMAGE_SRC_USERHUB",'file:///'+os.path.join(resources_dir, 'images/userhub.png'))
    HTML = HTML.replace("IMAGE_SRC_POWERHUB",'file:///'+os.path.join(resources_dir, 'images/poweruserhub.png'))
    HTML = HTML.replace("IMAGE_SRC_DEVHUB",'file:///'+os.path.join(resources_dir, 'images/developerhub.png'))
    HTML = HTML.replace("IMAGE_SRC_MANUAL",'file:///'+os.path.join(resources_dir, 'images/manual.png'))
    imagepath= 'file:///'+os.path.join(resources_dir, 'images/installed.png')
    imagepath = imagepath.replace('\\','/')  # replace Windows backslash with slash to make the path javascript compatible
    HTML = HTML.replace("IMAGE_SRC_INSTALLED",imagepath)

    # build UL_WORKBENCHES

    wblist = []
    UL_WORKBENCHES = '<ul class="workbenches">'
    FreeCAD.getResourceDir()
    for wb in sorted(FreeCADGui.listWorkbenches().keys()):
        if wb.endswith("Workbench"):
            wn = wb[:-9]
        else:
            wn = wb
        # fixes for non-standard names
        if wn == "flamingoTools":
            wn = "flamingo"
        elif wn == "Geodat":
            wn = "geodata"
        elif wn == "a2p":
            wn = "A2plus"
        elif wn == "ArchTexture":
            wn = "ArchTextures"
        elif wn == "CadQuery":
            wn = "cadquery_module"
        elif wn == "DefeaturingWB":
            wn = "Defeaturing"
        elif wn == "ManipulatorWB":
            wn = "Manipulator"
        elif wn == "PartOMagic":
            wn = "Part-o-magic"
        elif wn == "SM":
            wn = "sheetmetal"
        elif wn == "gear":
            wn = "FCGear"
        elif wn == "frame_":
            wn = "frame"
        elif wn == "None":
            continue
        wblist.append(wn.lower())
        if wb in iconbank:
            img = iconbank[wb]
        else:
            img = os.path.join(FreeCAD.getResourceDir(),"Mod",wn,"Resources","icons",wn+"Workbench.svg")
            if not os.path.exists(img):
                w = FreeCADGui.listWorkbenches()[wb]
                if hasattr(w,"Icon"):
                    xpm = w.Icon
                    if "XPM" in xpm:
                        xpm = xpm.replace("\n        ","\n") # some XPMs have some indent that QT doesn't like
                        r = [s[:-1].strip('"') for s in re.findall("(?s)\{(.*?)\};",xpm)[0].split("\n")[1:]]
                        p = QtGui.QPixmap(r)
                        p = p.scaled(24,24)
                        img = tempfile.mkstemp(dir=tempfolder,suffix='.png')[1]
                        p.save(img)
                    else:
                        img = xpm
                else:
                    img = os.path.join(resources_dir,"images/freecad.png")
            iconbank[wb] = img
        UL_WORKBENCHES += '<li>'
        UL_WORKBENCHES += '<img src="file:///'+iconbank[wb]+'">&nbsp;'
        UL_WORKBENCHES += '<a href="https://www.freecadweb.org/wiki/'+wn+'_Workbench">'+wn.replace("ReverseEngineering","ReverseEng")+'</a>'
        UL_WORKBENCHES += '</li>'
    UL_WORKBENCHES += '</ul>'
    HTML = HTML.replace("UL_WORKBENCHES",encode(UL_WORKBENCHES))

    # Detect additional addons that are not a workbench

    try:
        import dxfLibrary
    except:
        pass
    else:
        wblist.append("dxf-library")
    try:
        import RebarTools
    except:
        pass
    else:
        wblist.append("reinforcement")
    try:
        import CADExchangerIO
    except:
        pass
    else:
        wblist.append("cadexchanger")
    HTML = HTML.replace("var wblist = [];","var wblist = " + str(wblist) + ";")

    # set and replace colors and font settings

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
    if p.GetString("BackgroundImage",""):
        BACKGROUND = gethexcolor(p.GetUnsigned("BackgroundColor1",1331197183))+" url("+p.GetString("BackgroundImage","")+")"
    else:
        BACKGROUND = gethexcolor(p.GetUnsigned("BackgroundColor1",1331197183))
        # linear gradient not supported by QT "linear-gradient("+gethexcolor(p.GetUnsigned("BackgroundColor1",1331197183))+","+gethexcolor(p.GetUnsigned("BackgroundColor2",2141107711))+")"
    LINKCOLOR = gethexcolor(p.GetUnsigned("LinkColor",65535))
    BASECOLOR = gethexcolor(p.GetUnsigned("PageColor",4294967295))
    BOXCOLOR  = gethexcolor(p.GetUnsigned("BoxColor",3722305023))
    TEXTCOLOR = gethexcolor(p.GetUnsigned("PageTextColor",255))
    BGTCOLOR = gethexcolor(p.GetUnsigned("BackgroundTextColor",4294703103))
    SHADOW = "#888888"
    if QtGui.QColor(BASECOLOR).valueF() < 0.5: # dark page - we need to make darker shadows
        SHADOW = "#000000"
    FONTFAMILY = encode(p.GetString("FontFamily","Arial,Helvetica,sans"))
    if not FONTFAMILY:
        FONTFAMILY = "Arial,Helvetica,sans"
    FONTSIZE = p.GetInt("FontSize",13)
    HTML = HTML.replace("BASECOLOR",BASECOLOR)
    HTML = HTML.replace("BOXCOLOR",BOXCOLOR)
    HTML = HTML.replace("LINKCOLOR",LINKCOLOR)
    HTML = HTML.replace("TEXTCOLOR",TEXTCOLOR)
    HTML = HTML.replace("BGTCOLOR",BGTCOLOR)
    HTML = HTML.replace("BACKGROUND",BACKGROUND)
    HTML = HTML.replace("FONTFAMILY",FONTFAMILY)
    HTML = HTML.replace("FONTSIZE",str(FONTSIZE)+"px")

    # enable web access if permitted

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("AllowDownload",False):
        HTML = HTML.replace("var allowDownloads = 0;","var allowDownloads = 1;")

    # enable or disable forum

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowForum",False):
        HTML = HTML.replace("var showForum = 0;","var showForum = 1;")
        HTML = HTML.replace("display: none; /* forum display */","display: block; /* forum display */")

    # enable or disable notepad

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowNotes",False):
        HTML = HTML.replace("display: none; /* notes display */","display: block; /* notes display */")
        HTML = HTML.replace("width: 100%; /* thumbs display */","width: 70%; /* thumbs display */")

    # store variables for further use

    Start.iconbank = iconbank
    Start.tempfolder = tempfolder

    # make sure we are always returning unicode
    # HTML should be a str-object and therefore:
    # - for py2 HTML is a bytes object and has to be decoded to unicode
    # - for py3 HTML is already a unicode object and the next 2 lines can be removed
    #    once py2-support is removed.

    if isinstance(HTML, bytes):
        HTML = HTML.decode("utf8")

    return HTML



def exportTestFile():

    "Allow to check if everything is Ok"

    f = open(os.path.expanduser("~")+os.sep+"freecad-startpage.html","w")
    f.write(handle())
    f.close()



def postStart():

    "executes needed operations after loading a file"

    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")

    # switch workbench
    wb = param.GetString("AutoloadModule","")
    if wb:
        # don't switch workbenches if we are not in Start anymore
        if FreeCADGui.activeWorkbench() and (FreeCADGui.activeWorkbench().name() == "StartWorkbench"):
            FreeCADGui.activateWorkbench(wb)

    # close start tab
    cl = param.GetBool("closeStart",False)
    if cl:
        title = QtGui.QApplication.translate("Workbench","Start page")
        mw = FreeCADGui.getMainWindow()
        if mw:
            mdi = mw.findChild(QtGui.QMdiArea)
            if mdi:
                for mdichild in mdi.children():
                    for subw in mdichild.findChildren(QtGui.QMdiSubWindow):
                        if subw.windowTitle() == title:
                            subw.close()



def checkPostOpenStartPage():

    "on Start WB startup, check if we are loading a file and therefore need to close the StartPage"

    import Start
    if FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Mod/Start').GetBool('DoNotShowOnOpen',False) and (not hasattr(Start,'CanOpenStartPage')):
        if len(sys.argv) > 1:
            postStart()
    Start.CanOpenStartPage = True
