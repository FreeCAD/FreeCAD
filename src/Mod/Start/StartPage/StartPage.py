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

import sys,os,FreeCAD,FreeCADGui,tempfile,time,zipfile,urllib,re,TranslationTexts
from PySide import QtCore,QtGui

FreeCADGui.addLanguagePath(":/translations")
FreeCADGui.updateLocale()

iconprovider = QtGui.QFileIconProvider()
iconbank = {} # to store already created icons so we don't overpollute the temp dir
tempfolder = None # store icons inside a subfolder in temp dir


def gethexcolor(color):

    "returns a color hex value #000000"

    r = str(hex(int(((color>>24)&0xFF))))[2:].zfill(2)
    g = str(hex(int(((color>>16)&0xFF))))[2:].zfill(2)
    b = str(hex(int(((color>>8)&0xFF))))[2:].zfill(2)
    return "#"+r+g+b



def isplainfile(filename):

    "check if this is any type we don't want to show"

    if os.path.isdir(filename):
        return False
    basename = os.path.basename(filename)
    if basename.startswith("."):
        return False
    if basename[-1].isdigit():
        if basename[-7:-1].lower() == "fcstd": # freecad backup file
            return False
    if basename.endswith("~"):
        return False
    if basename.lower().endswith(".bak"):
        return False
    return True



def getInfo(filename):

    "returns available file information"

    global iconbank,tempfolder

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

    if os.path.exists(filename):

        # get normal file info
        s = os.stat(filename)
        size = getSize(s.st_size)
        #ctime = getLocalTime(s.st_ctime)
        #mtime = getLocalTime(s.st_mtime)
        author = TranslationTexts.T_UNKNOWN
        #company = TranslationTexts.T_UNKNOWN
        #lic = TranslationTexts.T_UNKNOWN
        image = None

        # get additional info from fcstd files
        if filename.lower().endswith(".fcstd"):
            zfile=zipfile.ZipFile(filename)
            files=zfile.namelist()
            # check for meta-file if it's really a FreeCAD document
            if files[0] == "Document.xml":
                doc = str(zfile.read(files[0]))
                doc = doc.replace("\n"," ")
                r = re.findall("Property name=\"CreatedBy.*?String value=\"(.*?)\"\/>",doc)
                if r:
                    author = r[0]
                #r = re.findall("Property name=\"Company.*?String value=\"(.*?)\"\/>",doc)
                #if r:
                #    company = r
                #r = re.findall("Property name=\"License.*?String value=\"(.*?)\"\/>",doc)
                #if r:
                #    lic =r
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
            if t in iconbank:
                image = iconbank[t]
            else:
                icon = iconprovider.icon(i)
                px = icon.pixmap(128,128)
                image = tempfile.mkstemp(dir=tempfolder,suffix='.png')[1]
                px.save(image)
                iconbank[t] = image

        return image,size,author

    return None,None,None



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
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Start")
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


    # get FreeCAD version

    v = FreeCAD.Version()
    VERSIONSTRING = TranslationTexts.T_VERSION + " " + v[0] + "." + v[1] + " " + TranslationTexts.T_BUILD + " " + v[2]
    HTML = HTML.replace("VERSIONSTRING",VERSIONSTRING)


    # translate texts

    HTML = HTML.replace("T_TITLE",TranslationTexts.T_TITLE)
    HTML = HTML.replace("T_DOCUMENTS",TranslationTexts.T_DOCUMENTS)
    HTML = HTML.replace("T_HELP",TranslationTexts.T_HELP)
    HTML = HTML.replace("T_ACTIVITY",TranslationTexts.T_ACTIVITY)
    HTML = HTML.replace("T_RECENTFILES",TranslationTexts.T_RECENTFILES)
    HTML = HTML.replace("T_TIP",TranslationTexts.T_TIP)
    HTML = HTML.replace("T_ADJUSTRECENT",TranslationTexts.T_ADJUSTRECENT)
    HTML = HTML.replace("T_GENERALDOCUMENTATION",TranslationTexts.T_GENERALDOCUMENTATION)
    HTML = HTML.replace("T_USERHUB",TranslationTexts.T_USERHUB)
    HTML = HTML.replace("T_DESCR_USERHUB",TranslationTexts.T_DESCR_USERHUB)
    HTML = HTML.replace("T_POWERHUB",TranslationTexts.T_POWERHUB)
    HTML = HTML.replace("T_DESCR_POWERHUB",TranslationTexts.T_DESCR_POWERHUB)
    HTML = HTML.replace("T_DEVHUB",TranslationTexts.T_DEVHUB)
    HTML = HTML.replace("T_DESCR_DEVHUB",TranslationTexts.T_DESCR_DEVHUB)
    HTML = HTML.replace("T_MANUAL",TranslationTexts.T_MANUAL)
    HTML = HTML.replace("T_DESCR_MANUAL",TranslationTexts.T_DESCR_MANUAL)
    HTML = HTML.replace("T_WBHELP",TranslationTexts.T_WBHELP)
    HTML = HTML.replace("T_DESCR_WBHELP",TranslationTexts.T_DESCR_WBHELP)
    HTML = HTML.replace("T_COMMUNITYHELP",TranslationTexts.T_COMMUNITYHELP)
    HTML = HTML.replace("T_DESCR_COMMUNITYHELP1",TranslationTexts.T_DESCR_COMMUNITYHELP1)
    HTML = HTML.replace("T_DESCR_COMMUNITYHELP2",TranslationTexts.T_DESCR_COMMUNITYHELP2)
    HTML = HTML.replace("T_DESCR_COMMUNITYHELP3",TranslationTexts.T_DESCR_COMMUNITYHELP3)
    HTML = HTML.replace("T_ADDONS",TranslationTexts.T_ADDONS)
    HTML = HTML.replace("T_DESCR_ADDONS",TranslationTexts.T_DESCR_ADDONS)
    HTML = HTML.replace("T_OFFLINEHELP",TranslationTexts.T_OFFLINEHELP)
    HTML = HTML.replace("T_OFFLINEPLACEHOLDER",TranslationTexts.T_OFFLINEPLACEHOLDER)
    HTML = HTML.replace("T_RECENTCOMMITS",TranslationTexts.T_RECENTCOMMITS)
    HTML = HTML.replace("T_DESCR_RECENTCOMMITS",TranslationTexts.T_DESCR_RECENTCOMMITS)
    HTML = HTML.replace("T_SEEONGITHUB",TranslationTexts.T_SEEONGITHUB)
    HTML = HTML.replace("T_CUSTOM",TranslationTexts.T_CUSTOM)
    HTML = HTML.replace("T_FORUM",TranslationTexts.T_FORUM)
    HTML = HTML.replace("T_DESCR_FORUM",TranslationTexts.T_DESCR_FORUM)
    HTML = HTML.replace("T_EXTERNALLINKS",TranslationTexts.T_EXTERNALLINKS)


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


    # build UL_RECENTFILES

    rf = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
    rfcount = rf.GetInt("RecentFiles",0)
    if rfcount:
        UL_RECENTFILES = "<ul>"
        for i in range(rfcount):
            filename = rf.GetString("MRU%d" % (i))
            if os.path.exists(filename):
                basename = os.path.basename(filename)
                image,size,author = getInfo(filename)
                if size:
                    UL_RECENTFILES += '<li class="icon">'
                    UL_RECENTFILES += '<a href="LoadMRU.py?MRU='+str(i)+'" title="'+basename+'">'
                    UL_RECENTFILES += '<img src="'+image+'">'
                    UL_RECENTFILES += '</a>'
                    UL_RECENTFILES += '<div class="caption">'
                    UL_RECENTFILES += '<h4>'+basename+'</h4>'
                    UL_RECENTFILES += '<p>'+size+'</p>'
                    UL_RECENTFILES += '<p>'+author+'</p>'
                    UL_RECENTFILES += '</div>'
                    UL_RECENTFILES += '</li>'

        UL_RECENTFILES += '<li class="icon">'
        UL_RECENTFILES += '<a href="LoadNew.py" title="'+TranslationTexts.T_CREATENEW+'">'
        UL_RECENTFILES += '<img src="'+iconbank["createimg"]+'">'
        UL_RECENTFILES += '</a>'
        UL_RECENTFILES += '<div class="caption">'
        UL_RECENTFILES += '<h4>'+TranslationTexts.T_CREATENEW+'</h4>'
        UL_RECENTFILES += '</div>'
        UL_RECENTFILES += '</li>'

        UL_RECENTFILES += '</ul>'
        if sys.version_info.major < 3:
            UL_RECENTFILES = UL_RECENTFILES.decode("utf8")
        HTML = HTML.replace("UL_RECENTFILES",UL_RECENTFILES)


    # build SECTION_EXAMPLES

    SECTION_EXAMPLES = ""
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowExamples",True):
        SECTION_EXAMPLES = "<h2>"+TranslationTexts.T_EXAMPLES+"</h2>"
        SECTION_EXAMPLES += "<ul>"
        for basename in os.listdir(FreeCAD.getResourceDir()+"examples"):
            filename = FreeCAD.getResourceDir()+"examples"+os.sep+basename
            image,size,author = getInfo(filename)
            if size:
                SECTION_EXAMPLES += '<li class="icon">'
                SECTION_EXAMPLES += '<a href="LoadExample.py?filename='+basename+'" title="'+basename+'">'
                SECTION_EXAMPLES += '<img src="'+image+'">'
                SECTION_EXAMPLES += '</a>'
                SECTION_EXAMPLES += '<div class="caption">'
                SECTION_EXAMPLES += '<h4>'+basename+'</h4>'
                SECTION_EXAMPLES += '<p>'+size+'</p>'
                SECTION_EXAMPLES += '<p>'+author+'</p>'
                SECTION_EXAMPLES += '</div>'
                SECTION_EXAMPLES += '</li>'
        SECTION_EXAMPLES += "</ul>"
    if sys.version_info.major < 3:
        SECTION_EXAMPLES = SECTION_EXAMPLES.decode("utf8")
    HTML = HTML.replace("SECTION_EXAMPLES",SECTION_EXAMPLES)


    # build SECTION_CUSTOM

    SECTION_CUSTOM = ""
    cfolder = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString("ShowCustomFolder","")
    if cfolder:
        if not os.path.isdir(cfolder):
            cfolder = os.path.dirname(cfolder)
        SECTION_CUSTOM = "<h2>"+os.path.basename(os.path.normpath(cfolder))+"</h2>"
        SECTION_CUSTOM += "<ul>"
        for basename in os.listdir(cfolder):
            filename = os.path.join(cfolder,basename)
            if isplainfile(filename):
                image,size,author = getInfo(filename)
                if size:
                    SECTION_CUSTOM += '<li class="icon">'
                    SECTION_CUSTOM += '<a href="LoadCustom.py?filename='+urllib.quote(basename)+'" title="'+basename+'">'
                    SECTION_CUSTOM += '<img src="'+image+'">'
                    SECTION_CUSTOM += '</a>'
                    SECTION_CUSTOM += '<div class="caption">'
                    SECTION_CUSTOM += '<h4>'+basename+'</h4>'
                    SECTION_CUSTOM += '<p>'+size+'</p>'
                    SECTION_CUSTOM += '<p>'+author+'</p>'
                    SECTION_CUSTOM += '</div>'
                    SECTION_CUSTOM += '</li>'
        SECTION_CUSTOM += "</ul>"
    if sys.version_info.major < 3:
        SECTION_CUSTOM = SECTION_CUSTOM.decode("utf8")
    HTML = HTML.replace("SECTION_CUSTOM",SECTION_CUSTOM)


    # build UL_WORKBENCHES

    wblist = []
    UL_WORKBENCHES = '<ul class="workbenches">'
    FreeCAD.getResourceDir()
    for wb in sorted(FreeCADGui.listWorkbenches().keys()):
        if wb.endswith("Workbench"):
            wn = wb[:-9]
            wblist.append(wn.lower())
            if wb in iconbank:
                img = iconbank[wb]
            else:
                img = os.path.join(FreeCAD.getResourceDir(),"data","Mod",wn,"Resources","icons",wn+"Workbench.svg")
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
                        img="images/freecad.png"
                iconbank[wb] = img
            UL_WORKBENCHES += '<li>'
            UL_WORKBENCHES += '<img src="'+iconbank[wb]+'">&nbsp;'
            UL_WORKBENCHES += '<a href="https://www.freecadweb.org/wiki/'+wn+'_Workbench">'+wn.replace("ReverseEngineering","ReverseEng")+'</a>'
            UL_WORKBENCHES += '</li>'
    UL_WORKBENCHES += '</ul>'
    HTML = HTML.replace("UL_WORKBENCHES",UL_WORKBENCHES)
    HTML = HTML.replace("var wblist = [];","var wblist = " + str(wblist) + ";")


    # set and replace colors

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

    HTML = HTML.replace("BASECOLOR",BASECOLOR)
    HTML = HTML.replace("BOXCOLOR",BOXCOLOR)
    HTML = HTML.replace("LINKCOLOR",LINKCOLOR)
    HTML = HTML.replace("TEXTCOLOR",TEXTCOLOR)
    HTML = HTML.replace("BGTCOLOR",BGTCOLOR)
    HTML = HTML.replace("BACKGROUND",BACKGROUND)
    HTML = HTML.replace("SHADOW",SHADOW)


    # enable web access if permitted

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("AllowDownload",False):
        HTML = HTML.replace("var allowDownloads = 0;","var allowDownloads = 1;")


    # enable or disable forum

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowForum",False):
        HTML = HTML.replace("var showForum = 0;","var showForum = 1;")
        HTML = HTML.replace("display: none; /* forum display */","display: block; /* forum display */")


    # store variables for further use

    Start.iconbank = iconbank
    Start.tempfolder = tempfolder


    # encode if necessary

    if sys.version_info.major < 3:
        if isinstance(HTML,unicode):
            HTML = HTML.encode("utf8")

    return HTML



def exportTestFile():

    "Allow to check if everything is Ok"

    f = open(os.path.expanduser("~")+os.sep+"freecad-startpage.html","wb")
    f.write(handle())
    f.close()



def postStart():

    "executes needed operations after loading a file"

    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")

    # switch workbench
    wb = param.GetString("AutoloadModule","")
    if wb:
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

