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

import six
import sys,os,FreeCAD,FreeCADGui,tempfile,time,zipfile,urllib,re,hashlib
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

    if six.PY2:
        if not isinstance(text,six.text_type):
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
        return os.path.exists(filename+'/Document.xml')
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

def saveIcon(key,data,ext):
    '''
    Save icon data into iconbank
    
    key: the text key of the icon
    data: data of the icon. Can either be a QImage, QPixmap, QByteArray or
          Python bytes
    ext: extension of the file. In case of data being QImage/QPixmap, this is
         also used as the image file saving format.

    This function computes the sha1 hash of the icon data and save the file
    using the hash as the name with the given extension. If a file with a same
    hash exists, it is not saved
    '''

    if isinstance(data,(QtGui.QImage,QtGui.QPixmap)):
        array = QtCore.QByteArray()
        buf = QtCore.QBuffer(array)
        buf.open(QtCore.QIODevice.WriteOnly)
        data.save(buf,ext)
        buf.close()

        content = array

        if ext != 'xpm':
            # PNG format seems to store timestamps that messes up digest. Let's
            # calculate digest on XPM instead
            content = QtCore.QByteArray()
            buf = QtCore.QBuffer(content)
            data.save(buf,'xpm')
            buf.close()

        data = array

    if isinstance(data,QtCore.QByteArray):
        sha = QtCore.QCryptographicHash.hash(
                content,QtCore.QCryptographicHash.Sha1).toHex()
        sha = sha.data().decode('latin1')
    else:
        sha = hashlib.sha1()
        sha.update(data)
        sha = sha.hexdigest()

    name = '{}/{}.{}'.format(tempfolder,sha,ext)

    if not os.path.exists(name):
        if isinstance(data,QtCore.QByteArray):
            f = QtCore.QFile(name)
            f.open(QtCore.QIODevice.WriteOnly)
            f.write(data)
            f.close()
        else:
            with open(name,'wb') as f:
                f.write(data)

    iconbank[key] = name
    return name

_Re_Pattern = "<Property name=\"{}\".*?String value=\"(.*?)\"\/>"
_Re_CreatedBy = re.compile(_Re_Pattern.format("CreatedBy"))
_Re_Company = re.compile(_Re_Pattern.format("Company"))
_Re_License = re.compile(_Re_Pattern.format("License"))
_Re_Comment = re.compile(_Re_Pattern.format("Comment"))
_Re_CreationDate = re.compile(_Re_Pattern.format("CreationDate"))
_Re_LastModifiedDate = re.compile(_Re_Pattern.format("LastModifiedDate"))

def getLocalTime(timestamp):
    tformat = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString("TimeFormat","%m/%d/%Y %H:%M:%S")

    "returns a local time from a timestamp"
    return time.strftime(tformat,time.localtime(timestamp))

def getInfo(filename):

    "returns available file information"

    global iconbank,tempfolder

    def getSize(size):
        "returns a human-readable size"
        if size > 1024*1024:
            hsize = str(int(size/(1024*1024))) + "Mb"
        elif size > 1024:
            hsize = str(int(size/1024)) + "Kb"
        else:
            hsize = str(int(size)) + "b"
        return hsize

    def getFreeDesktopThumbnail(filename):
        "if we have gnome libs available, try to find a system-generated thumbnail"
        try:
            import gnome.ui
            import gnomevfs
        except:
            return None

        path = os.path.abspath(filename)
        uri = gnomevfs.get_uri_from_local_path(path)
        thumb = gnome.ui.thumbnail_path_for_uri(uri, "normal")
        if os.path.exists(thumb):
            return thumb
        return None


    if os.path.exists(filename):

        docfile = None
        zfile = None
        if os.path.isdir(filename):
            docfile = filename + '/Document.xml'
            if not os.path.exists(docfile):
                return None
            s = os.stat(docfile)
            size = sum(os.path.getsize(filename+'/'+f) for f in \
                    os.listdir(filename) if os.path.isfile(filename+'/'+f))
        else:
            # get normal file info
            s = os.stat(filename)
            size = s.st_size

            if filename.lower().endswith(".fcstd"):
                try:
                    zfile=zipfile.ZipFile(filename)
                except:
                    print("Cannot read file: ",filename)
                    return None
                files=zfile.namelist()

        size = getSize(size)
        ctime = getLocalTime(s.st_ctime)
        mtime = getLocalTime(s.st_mtime)
        author = ""
        company = TranslationTexts.T_UNKNOWN
        lic = TranslationTexts.T_UNKNOWN
        image = None
        doc = None
        descr = ""

        # get additional info from fcstd files
        imagePath="thumbnails/Thumbnail.png"
        if zfile:
            # check for meta-file if it's really a FreeCAD document
            if files[0] == "Document.xml":
                doc = str(zfile.read(files[0]))
                doc = doc.replace("\n"," ")
            if  imagePath in files:
                if filename in iconbank:
                    image = iconbank[filename]
                else:
                    image = saveIcon(filename,zfile.read(imagePath),'png')
        elif docfile:
            with open(docfile) as f:
                doc = f.read().replace('\n',' ')
            image=filename+'/'+imagePath
            if not os.path.exists(image):
                image = None

        if doc:
            r = _Re_CreatedBy.search(doc)
            if r:
                author = r.group(1)
                # remove email if present in author field
                if "&lt;" in author:
                    author = author.split("&lt;")[0].strip()
            r = _Re_Company.search(doc)
            if r:
                company = r.group(1)
            r = _Re_License.search(doc)
            if r:
                lic = r.group(1)
            r = _Re_Comment.search(doc)
            if r:
                descr = r.group(1)
            r = _Re_CreationDate.search(doc)
            if r:
                ctime = r.group(1)
            r = _Re_LastModifiedDate.search(doc)
            if r:
                mtime = r.group(1)

        if not image:
            # use image itself as icon if it's an image file
            if os.path.splitext(filename)[1].lower() in [".jpg",".jpeg",".png",".svg"]:
                image = filename
                iconbank[filename] = image

            # use freedesktop thumbnail if available
            fdthumb = getFreeDesktopThumbnail(filename)
            if fdthumb:
                image = fdthumb
                iconbank[filename] = fdthumb

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
                    image = saveIcon(t,icon.pixmap(preferred),'png')
                else:
                    image = getDefaultIcon()
        return [image,size,author,ctime,mtime,descr,company,lic]

    return None



def getDefaultIcon():

    "retrieves or creates a default file icon"

    global defaulticon

    if not defaulticon:
        i = QtCore.QFileInfo(__file__) # MUST provide an existing file in qt5
        icon = iconprovider.icon(i)
        preferred = icon.actualSize(QtCore.QSize(128,128))
        defaulticon = saveIcon('defaulticon',icon.pixmap(preferred),'png')

    return defaulticon



def buildCard(filename,method,arg=None):

    "builds a html <li> element representing a file. method is a script + a keyword, for ex. url.py?key="

    global tempfolder

    result = encode("")
    if os.path.exists(filename) and isOpenableByFreeCAD(filename):
        basename = os.path.basename(filename)
        if not arg:
            arg = basename

        cachename = None
        if filename.lower().endswith(".fcstd"):
            sha = hashlib.sha1()
            sha.update(filename + '_card')
            sha = sha.hexdigest()

            s = os.stat(filename)
            cacheheader = '<!--%s-->' % getLocalTime(s.st_mtime)
            cachename = '{}/{}.html'.format(tempfolder,sha)
            if os.path.exists(cachename):
                with open(cachename, 'r') as f:
                    result = f.read()
                    if isinstance(result, bytes):
                        result = result.decode("utf8")
                    if result.startswith(cacheheader):
                        return result.replace('$METHOD$', method).replace('$ARG$', arg)

        finfo = getInfo(filename)
        if finfo:
            image = finfo[0]
            size = finfo[1]
            author = finfo[2]
            infostring = encode(TranslationTexts.T_LOCATION+": "+os.path.dirname(filename)+"\n")
            infostring += encode(TranslationTexts.T_CREATIONDATE+": "+finfo[3]+"\n")
            infostring += encode(TranslationTexts.T_LASTMODIFIED+": "+finfo[4])
            if finfo[5]:
                infostring += "\n\n" + encode(finfo[5])
            if size:
                result += '<a href="$METHOD$$ARG$'+'" title="'+infostring+'">'
                result += '<li class="icon">'
                result += '<img src="file:///'+image+'">'
                result += '<div class="caption">'
                result += '<h4>'+encode(basename)+'</h4>'
                result += '<p>'+encode(author)+'</p>'
                result += '<p>'+size+'</p>'
                result += '</div>'
                result += '</li>'
                result += '</a>'

                if cachename:
                    with open(cachename, 'w') as f:
                        f.write(cacheheader.encode('utf8'))
                        f.write(result.encode('utf8'))

                return result.replace('$METHOD$', method).replace('$ARG$', arg)

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
        tempfolder = tempfile.gettempdir() + '/FreeCADStartThumbnails'

    if not os.path.exists(tempfolder):
        os.makedirs(tempfolder)

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
            # Search for stylesheet in user, system and resources locations
            user = os.path.join(FreeCAD.getUserAppDataDir(), "Gui", "Stylesheets")
            system = os.path.join(FreeCAD.getResourceDir(), "Gui", "Stylesheets")
            resources = ":/stylesheets"

            res = False
            if QtCore.QFile.exists(os.path.join(user, qssfile)):
                path = os.path.join(user, qssfile)
            elif QtCore.QFile.exists(os.path.join(system, qssfile)):
                path = os.path.join(system, qssfile)
            elif QtCore.QFile.exists(os.path.join(resources, qssfile)):
                res = True
                path = os.path.join(resources, qssfile)
            else:
                path = None

            if path:
                if res:
                    f = QtCore.QFile(path)
                    if f.open(QtCore.QIODevice.ReadOnly | QtCore.QFile.Text):
                        ALTCSS = encode(QtCore.QTextStream(f).readAll())
                        HTML = HTML.replace("<!--QSS-->","<style type=\"text/css\">"+ALTCSS+"</style>")
                else:
                    with open(path, 'r') as f:
                        ALTCSS = encode(f.read())
                        HTML = HTML.replace("<!--QSS-->","<style type=\"text/css\">"+ALTCSS+"</style>")

    # turn tips off if needed

    if not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowTips",True):
        HTML = HTML.replace("display: block; /* footnote tips display */","display: none; /* footnote tips display */")

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
        saveIcon('createimg',i,'xpm')

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
        # hide the custom section tooltip if custom section is set (users know about it if they enabled it)
        HTML = HTML.replace("id=\"customtip\"","id=\"customtip\" style=\"display:none;\"")
    HTML = HTML.replace("SECTION_CUSTOM",SECTION_CUSTOM)

    # build IMAGE_SRC paths

    HTML = HTML.replace("IMAGE_SRC_USERHUB",'file:///'+os.path.join(resources_dir, 'images/userhub.png'))
    HTML = HTML.replace("IMAGE_SRC_POWERHUB",'file:///'+os.path.join(resources_dir, 'images/poweruserhub.png'))
    HTML = HTML.replace("IMAGE_SRC_DEVHUB",'file:///'+os.path.join(resources_dir, 'images/developerhub.png'))
    HTML = HTML.replace("IMAGE_SRC_MANUAL",'file:///'+os.path.join(resources_dir, 'images/manual.png'))
    HTML = HTML.replace("IMAGE_SRC_SETTINGS",'file:///'+os.path.join(resources_dir, 'images/settings.png'))
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
        elif wn == "ksuWB":
            wn = "kicadStepUp"
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
                        img = saveIcon(wb,p,'png')
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
    if "$LastModule" == wb:
        wb = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General").GetString("LastModule","")
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
