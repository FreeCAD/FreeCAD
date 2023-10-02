# ***************************************************************************
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


# This is the start page template. It builds an HTML global variable that
# contains the html code of the start page.
# Note: It is built only once per FreeCAD session for now...

import sys
import os
import tempfile
import time
import zipfile
import re
import FreeCAD
import FreeCADGui
import codecs
import urllib.parse
from . import TranslationTexts
from PySide import QtCore, QtGui

try:
    from addonmanager_macro import Macro as AM_Macro

    has_am_macro = True
except ImportError:
    has_am_macro = False


FreeCADGui.addLanguagePath(":/translations")
FreeCADGui.updateLocale()

iconprovider = QtGui.QFileIconProvider()
iconbank = {}  # store pre-existing icons so we don't overpollute temp dir
tempfolder = None  # store icons inside a subfolder in temp dir
defaulticon = None  # store a default icon for problematic file types


def gethexcolor(color):

    "returns a color hex value #000000"

    r = str(hex(int(((color >> 24) & 0xFF))))[2:].zfill(2)
    g = str(hex(int(((color >> 16) & 0xFF))))[2:].zfill(2)
    b = str(hex(int(((color >> 8) & 0xFF))))[2:].zfill(2)
    return "#" + r + g + b


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

    global iconbank, tempfolder

    tformat = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString(
        "TimeFormat", "%m/%d/%Y %H:%M:%S"
    )

    def getLocalTime(timestamp):
        "returns a local time from a timestamp"
        return time.strftime(tformat, time.localtime(timestamp))

    def getSize(size):
        "returns a human-readable size"
        if size > 1024 * 1024:
            hsize = str(int(size / (1024 * 1024))) + "Mb"
        elif size > 1024:
            hsize = str(int(size / 1024)) + "Kb"
        else:
            hsize = str(int(size)) + "b"
        return hsize

    def getFreeDesktopThumbnail(filename):
        "if we have gnome libs available, try to find a system-generated thumbnail"
        path = os.path.abspath(filename)
        thumb = None
        try:
            import gnome.ui
            import gnomevfs
        except Exception:
            # alternative method
            import hashlib

            fhash = hashlib.md5(
                bytes(urllib.parse.quote("file://" + path, safe=":/"), "ascii")
            ).hexdigest()
            thumb = os.path.join(os.path.expanduser("~"), ".thumbnails", "normal", fhash + ".png")
        else:
            uri = gnomevfs.get_uri_from_local_path(path)
            thumb = gnome.ui.thumbnail_path_for_uri(uri, "normal")
        if thumb and os.path.exists(thumb):
            return thumb
        return None

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
                zfile = zipfile.ZipFile(filename)
            except Exception:
                print("Cannot read file: ", filename)
                return None
            files = zfile.namelist()
            # check for meta-file if it's really a FreeCAD document
            if files[0] == "Document.xml":
                try:
                    doc = zfile.read(files[0]).decode("utf-8")
                except OSError as e:
                    print(
                        "Fail to load corrupted FCStd file: '{0}' with this error: {1}".format(
                            filename, str(e)
                        )
                    )
                    return None
                doc = doc.replace("\n", " ")
                r = re.findall('Property name="CreatedBy.*?String value="(.*?)"/>', doc)
                if r:
                    author = r[0]
                    # remove email if present in author field
                    if "&lt;" in author:
                        author = author.split("&lt;")[0].strip()
                r = re.findall('Property name="Company.*?String value="(.*?)"/>', doc)
                if r:
                    company = r[0]
                r = re.findall('Property name="License.*?String value="(.*?)"/>', doc)
                if r:
                    lic = r[0]
                r = re.findall('Property name="Comment.*?String value="(.*?)"/>', doc)
                if r:
                    descr = r[0]
                if "thumbnails/Thumbnail.png" in files:
                    if filename in iconbank:
                        image = iconbank[filename]
                    else:
                        imagedata = zfile.read("thumbnails/Thumbnail.png")
                        image = tempfile.mkstemp(dir=tempfolder, suffix=".png")[1]
                        thumb = open(image, "wb")
                        thumb.write(imagedata)
                        thumb.close()
                        iconbank[filename] = image

        elif filename.lower().endswith(".fcmacro"):
            # For FreeCAD macros, use the Macro Editor icon (but we have to have it in a file for
            # the web view to load it)
            image = os.path.join(tempfolder, "fcmacro_icon.svg")
            if not os.path.exists(image):
                f = QtCore.QFile(":/icons/MacroEditor.svg")
                f.copy(image)
            iconbank[filename] = image

            if has_am_macro:
                macro = AM_Macro(os.path.basename(filename))
                macro.fill_details_from_file(filename)
                author = macro.author

        elif QtGui.QImageReader.imageFormat(filename):
            # use image itself as icon if it's an image file
            image = filename
            iconbank[filename] = image

        else:
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
                    preferred = icon.actualSize(QtCore.QSize(128, 128))
                    px = icon.pixmap(preferred)
                    image = tempfile.mkstemp(dir=tempfolder, suffix=".png")[1]
                    px.save(image)
                else:
                    image = getDefaultIcon()
                iconbank[t] = image
        return [image, size, author, ctime, mtime, descr, company, lic]

    return None


def getDefaultIcon():

    "retrieves or creates a default file icon"

    global defaulticon

    if not defaulticon:
        i = QtCore.QFileInfo(__file__)  # MUST provide an existing file in qt5
        icon = iconprovider.icon(i)
        preferred = icon.actualSize(QtCore.QSize(128, 128))
        px = icon.pixmap(preferred)
        image = tempfile.mkstemp(dir=tempfolder, suffix=".png")[1]
        px.save(image)
        defaulticon = image

    return defaulticon


def build_new_file_card(template):

    """builds an html <li> element representing a new file
    quick start button"""

    templates = {
        "empty_file": [
            TranslationTexts.T_TEMPLATE_EMPTYFILE_NAME,
            TranslationTexts.T_TEMPLATE_EMPTYFILE_DESC,
        ],
        "open_file": [
            TranslationTexts.T_TEMPLATE_OPENFILE_NAME,
            TranslationTexts.T_TEMPLATE_OPENFILE_DESC,
        ],
        "parametric_part": [
            TranslationTexts.T_TEMPLATE_PARAMETRICPART_NAME,
            TranslationTexts.T_TEMPLATE_PARAMETRICPART_DESC,
        ],
        # "csg_part": [TranslationTexts.T_TEMPLATE_CSGPART_NAME, TranslationTexts.T_TEMPLATE_CSGPART_DESC],
        "2d_draft": [
            TranslationTexts.T_TEMPLATE_2DDRAFT_NAME,
            TranslationTexts.T_TEMPLATE_2DDRAFT_DESC,
        ],
        "architecture": [
            TranslationTexts.T_TEMPLATE_ARCHITECTURE_NAME,
            TranslationTexts.T_TEMPLATE_ARCHITECTURE_DESC,
        ],
    }

    if template not in templates:
        return ""

    image = "file:///" + os.path.join(
        os.path.join(FreeCAD.getResourceDir(), "Mod", "Start", "StartPage"),
        "images/new_" + template + ".png",
    ).replace("\\", "/")

    result = ""
    result += '<li class="quickstart-button-card">'
    result += '<a href="LoadNew.py?template=' + urllib.parse.quote(template) + '">'
    result += '<img src="' + image + '" alt="' + template + '">'
    result += '<div class="caption">'
    result += "<h3>" + templates[template][0] + "</h3>"
    result += "<p>" + templates[template][1] + "</p>"
    result += "</div>"
    result += "</a>"
    result += "</li>"
    return result


def buildCard(filename, method, arg=None):

    """builds an html <li> element representing a file.
    method is a script + a keyword, for ex. url.py?key="""

    result = ""
    if os.path.exists(filename) and isOpenableByFreeCAD(filename):
        basename = os.path.basename(filename)
        if not arg:
            arg = basename
        finfo = getInfo(filename)
        if finfo:
            image = finfo[0]
            size = finfo[1]
            author = finfo[2]
            infostring = TranslationTexts.T_CREATIONDATE + ": " + finfo[3] + "\n"
            infostring += TranslationTexts.T_LASTMODIFIED + ": " + finfo[4]
            if finfo[5]:
                infostring += "\n\n" + finfo[5]
            if size:
                result += '<li class="file-card">'
                result += (
                    '<a href="' + method + urllib.parse.quote(arg) + '" title="' + infostring + '">'
                )
                result += (
                    '<img src="file:///' + image.replace("\\", "/") + '" alt="' + basename + '">'
                )
                result += '<div class="caption">'
                result += "<h4>" + basename + "</h4>"
                result += "<p>" + author + "</p>"
                result += "<p>" + size + "</p>"
                result += "</div>"
                result += "</a>"
                result += "</li>"
    return result


def handle():

    "builds the HTML code of the start page"

    global iconbank, tempfolder

    # reuse stuff from previous runs to reduce temp dir clutter

    import Start

    if hasattr(Start, "iconbank"):
        iconbank = Start.iconbank
    if hasattr(Start, "tempfolder"):
        tempfolder = Start.tempfolder
    else:
        tempfolder = tempfile.mkdtemp(prefix="FreeCADStartThumbnails")

    # build the html page skeleton

    resources_dir = os.path.join(FreeCAD.getResourceDir(), "Mod", "Start", "StartPage")
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
    template = p.GetString("Template", "")
    if template:
        html_filename = template
    else:
        html_filename = os.path.join(resources_dir, "StartPage.html")
    js_filename = os.path.join(resources_dir, "StartPage.js")
    css_filename = p.GetString("CSSFile", os.path.join(resources_dir, "StartPage.css"))
    with open(html_filename, "r") as f:
        HTML = f.read()
    with open(js_filename, "r") as f:
        JS = f.read()
    with open(css_filename, "r") as f:
        CSS = f.read()
    HTML = HTML.replace("JS", JS)
    HTML = HTML.replace("DEFAULT_CSS", CSS)
    HTML = HTML.replace(
        "CUSTOM_CSS",
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
        .GetString("CustomCSS", "")
        .replace("\n", ""),
    )

    # set the language

    HTML = HTML.replace("BCP47_LANGUAGE", QtCore.QLocale().bcp47Name())

    # get the stylesheet if we are using one

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool(
        "UseStyleSheet", False
    ):
        qssfile = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow").GetString(
            "StyleSheet", ""
        )
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
                        ALTCSS = QtCore.QTextStream(f).readAll()
                        HTML = HTML.replace(
                            "<!--QSS-->", '<style type="text/css">' + ALTCSS + "</style>"
                        )
                else:
                    with codecs.open(path, encoding="utf-8") as f:
                        ALTCSS = f.read()
                        HTML = HTML.replace(
                            "<!--QSS-->", '<style type="text/css">' + ALTCSS + "</style>"
                        )

    # handle file thumbnail icons visibility and size

    if not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool(
        "ShowFileThumbnailIcons", True
    ):
        HTML = HTML.replace(
            "display: block; /* thumb icons display */", "display: none; /* thumb icons display */"
        )
        HTML = HTML.replace("THUMBCARDSIZE", "75px")

    thumb_icons_size = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetInt(
        "FileThumbnailIconsSize", 128
    )
    HTML = HTML.replace("THUMBSIZE", str(thumb_icons_size) + "px")
    HTML = HTML.replace("THUMBCARDSIZE", str(thumb_icons_size + 75) + "px")

    # turn tips off if needed

    if not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool(
        "ShowTips", True
    ):
        HTML = HTML.replace(
            "display: block; /* footnote tips display */",
            "display: none; /* footnote tips display */",
        )

    # get FreeCAD version

    v = FreeCAD.Version()
    VERSIONSTRING = (
        TranslationTexts.T_VERSION
        + " "
        + v[0]
        + "."
        + v[1]
        + "."
        + v[2]
        + " "
        + TranslationTexts.T_BUILD
        + " "
        + v[3]
    )
    HTML = HTML.replace("VERSIONSTRING", VERSIONSTRING)

    # translate texts

    texts = [t for t in dir(TranslationTexts) if t.startswith("T_")]
    for text in texts:
        HTML = HTML.replace(text, getattr(TranslationTexts, text))

    # build a "create new" icon with the FreeCAD background color gradient

    if not "createimg" in iconbank:
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        c1 = gethexcolor(p.GetUnsigned("BackgroundColor2"))
        c2 = gethexcolor(p.GetUnsigned("BackgroundColor3"))
        gradient = QtGui.QLinearGradient(0, 0, 0, 128)
        gradient.setColorAt(0.0, QtGui.QColor(c1))
        gradient.setColorAt(1.0, QtGui.QColor(c2))
        i = QtGui.QImage(128, 128, QtGui.QImage.Format_RGB16)
        pa = QtGui.QPainter(i)
        pa.fillRect(i.rect(), gradient)
        pa.end()
        createimg = tempfile.mkstemp(dir=tempfolder, suffix=".png")[1]
        i.save(createimg)
        iconbank["createimg"] = createimg

    # build SECTION_NEW_FILE

    SECTION_NEW_FILE = "<h2>" + TranslationTexts.T_NEWFILE + "</h2>"
    SECTION_NEW_FILE += "<ul>"
    SECTION_NEW_FILE += build_new_file_card("empty_file")
    SECTION_NEW_FILE += build_new_file_card("open_file")
    SECTION_NEW_FILE += build_new_file_card("parametric_part")
    # SECTION_NEW_FILE += build_new_file_card("csg_part")
    SECTION_NEW_FILE += build_new_file_card("2d_draft")
    SECTION_NEW_FILE += build_new_file_card("architecture")
    SECTION_NEW_FILE += "</ul>"
    HTML = HTML.replace("SECTION_NEW_FILE", SECTION_NEW_FILE)

    # build SECTION_RECENTFILES

    rf = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
    rfcount = rf.GetInt("RecentFiles", 0)
    SECTION_RECENTFILES = "<h2>" + TranslationTexts.T_RECENTFILES + "</h2>"
    SECTION_RECENTFILES += "<ul>"
    for i in range(rfcount):
        filename = rf.GetString("MRU%d" % (i))
        SECTION_RECENTFILES += buildCard(filename, method="LoadMRU.py?MRU=", arg=str(i))
    SECTION_RECENTFILES += "</ul>"
    HTML = HTML.replace("SECTION_RECENTFILES", SECTION_RECENTFILES)

    # build SECTION_EXAMPLES

    SECTION_EXAMPLES = ""
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool(
        "ShowExamples", True
    ):
        SECTION_EXAMPLES = "<h2>" + TranslationTexts.T_EXAMPLES + "</h2>"
        SECTION_EXAMPLES += "<ul>"
        examples_path = FreeCAD.getResourceDir() + "examples"
        if os.path.exists(examples_path):
            examples = os.listdir(examples_path)
            for basename in examples:
                filename = FreeCAD.getResourceDir() + "examples" + os.sep + basename
                SECTION_EXAMPLES += buildCard(filename, method="LoadExample.py?filename=")
        SECTION_EXAMPLES += "</ul>"
    HTML = HTML.replace("SECTION_EXAMPLES", SECTION_EXAMPLES)

    # build SECTION_CUSTOM

    SECTION_CUSTOM = ""
    cfolders = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString(
        "ShowCustomFolder", ""
    )
    if cfolders:
        dn = 0
        for cfolder in cfolders.split(";;"):  # allow several paths separated by ;;
            if not os.path.isdir(cfolder):
                cfolder = os.path.dirname(cfolder)
            if not os.path.exists(cfolder):
                FreeCAD.Console.PrintWarning("Custom folder not found: %s" % cfolder)
            else:
                SECTION_CUSTOM += "<h2>" + os.path.basename(os.path.normpath(cfolder)) + "</h2>"
                SECTION_CUSTOM += "<ul>"
                for basename in os.listdir(cfolder):
                    filename = os.path.join(cfolder, basename)
                    SECTION_CUSTOM += buildCard(
                        filename, method="LoadCustom.py?filename=" + str(dn) + "_"
                    )
                SECTION_CUSTOM += "</ul>"
                # hide the custom section tooltip if custom section is set (users know about it if they enabled it)
                HTML = HTML.replace(
                    'id="customtip" class', 'id="customtip" style="display:none;" class'
                )
                dn += 1
    HTML = HTML.replace("SECTION_CUSTOM", SECTION_CUSTOM)

    # build IMAGE_SRC paths

    HTML = HTML.replace(
        "IMAGE_SRC_FREECAD",
        "file:///" + os.path.join(resources_dir, "images/freecad.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_ICON_DOCUMENTS",
        "file:///" + os.path.join(resources_dir, "images/icon_documents.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_ICON_HELP",
        "file:///" + os.path.join(resources_dir, "images/icon_help.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_ICON_ACTIVITY",
        "file:///" + os.path.join(resources_dir, "images/icon_activity.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_ICON_BLOG",
        "file:///" + os.path.join(resources_dir, "images/icon_blog.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_USERHUB",
        "file:///" + os.path.join(resources_dir, "images/userhub.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_POWERHUB",
        "file:///" + os.path.join(resources_dir, "images/poweruserhub.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_DEVHUB",
        "file:///" + os.path.join(resources_dir, "images/developerhub.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_MANUAL",
        "file:///" + os.path.join(resources_dir, "images/manual.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_SETTINGS",
        "file:///" + os.path.join(resources_dir, "images/icon_settings.png").replace("\\", "/"),
    )
    HTML = HTML.replace(
        "IMAGE_SRC_INSTALLED",
        "file:///" + os.path.join(resources_dir, "images/installed.png").replace("\\", "/"),
    )

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
        elif wn == "ReverseEngineering":
            wn = "Reverse_Engineering"
        elif wn == "None":
            continue
        wblist.append(wn.lower())
        if wb in iconbank:
            img = iconbank[wb]
        else:
            img = os.path.join(
                FreeCAD.getResourceDir(), "Mod", wn, "Resources", "icons", wn + "Workbench.svg"
            )
            if not os.path.exists(img):
                w = FreeCADGui.listWorkbenches()[wb]
                if hasattr(w, "Icon") and w.Icon:
                    xpm = w.Icon
                    if "XPM" in xpm:
                        xpm = xpm.replace(
                            "\n        ", "\n"
                        )  # some XPMs have some indent that QT doesn't like
                        r = [
                            s[:-1].strip('"')
                            for s in re.findall("(?s){(.*?)};", xpm)[0].split("\n")[1:]
                        ]
                        p = QtGui.QPixmap(r)
                        p = p.scaled(24, 24)
                        img = tempfile.mkstemp(dir=tempfolder, suffix=".png")[1]
                        p.save(img)
                    else:
                        img = xpm
                else:
                    img = os.path.join(resources_dir, "images/freecad.png")
            iconbank[wb] = img
        UL_WORKBENCHES += "<li>"
        UL_WORKBENCHES += (
            '<img src="file:///' + img.replace("\\", "/") + '" alt="' + wn + '">&nbsp;'
        )
        UL_WORKBENCHES += (
            '<a href="https://www.freecad.org/wiki/'
            + wn
            + '_Workbench">'
            + wn.replace("Reverse_Engineering", "ReverseEng")
            + "</a>"
        )
        UL_WORKBENCHES += "</li>"
    UL_WORKBENCHES += "</ul>"
    HTML = HTML.replace("UL_WORKBENCHES", UL_WORKBENCHES)

    # Detect additional addons that are not a workbench

    try:
        import dxfLibrary
    except Exception:
        pass
    else:
        wblist.append("dxf-library")
    try:
        import RebarTools
    except Exception:
        pass
    else:
        wblist.append("reinforcement")
    try:
        import CADExchangerIO
    except Exception:
        pass
    else:
        wblist.append("cadexchanger")
    HTML = HTML.replace("var wblist = [];", "var wblist = " + str(wblist) + ";")

    # set and replace colors and font settings

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
    if p.GetString("BackgroundImage", ""):
        BACKGROUND = (
            gethexcolor(p.GetUnsigned("BackgroundColor1", 1331197183))
            + " url("
            + p.GetString("BackgroundImage", "")
            + ")"
        )
    else:
        BACKGROUND = gethexcolor(p.GetUnsigned("BackgroundColor1", 1331197183))
        # linear gradient not supported by QT "linear-gradient("+gethexcolor(p.GetUnsigned("BackgroundColor1",1331197183))+","+gethexcolor(p.GetUnsigned("BackgroundColor2",2141107711))+")"
    LINKCOLOR = gethexcolor(p.GetUnsigned("LinkColor", 65535))
    BASECOLOR = gethexcolor(p.GetUnsigned("PageColor", 4294967295))
    BOXCOLOR = gethexcolor(p.GetUnsigned("BoxColor", 3722305023))
    TEXTCOLOR = gethexcolor(p.GetUnsigned("PageTextColor", 255))
    BGTCOLOR = gethexcolor(p.GetUnsigned("BackgroundTextColor", 1600086015))
    SHADOW = "#888888"
    if QtGui.QColor(BASECOLOR).valueF() < 0.5:  # dark page - we need to make darker shadows
        SHADOW = "#000000"
    FONTFAMILY = p.GetString("FontFamily", "Arial,Helvetica,sans")
    if not FONTFAMILY:
        FONTFAMILY = "Arial,Helvetica,sans"
    FONTSIZE = p.GetInt("FontSize", 13)
    HTML = HTML.replace("BASECOLOR", BASECOLOR)
    HTML = HTML.replace("BOXCOLOR", BOXCOLOR)
    HTML = HTML.replace("LINKCOLOR", LINKCOLOR)
    HTML = HTML.replace("TEXTCOLOR", TEXTCOLOR)
    HTML = HTML.replace("BGTCOLOR", BGTCOLOR)
    HTML = HTML.replace("BACKGROUND", BACKGROUND)
    HTML = HTML.replace("SHADOW", SHADOW)
    HTML = HTML.replace("FONTFAMILY", FONTFAMILY)
    HTML = HTML.replace("FONTSIZE", str(FONTSIZE) + "px")

    # enable web access if permitted

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool(
        "AllowDownload", False
    ):
        HTML = HTML.replace("var allowDownloads = 0;", "var allowDownloads = 1;")

    # enable or disable forum

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowForum", False):
        HTML = HTML.replace("var showForum = 0;", "var showForum = 1;")
        HTML = HTML.replace(
            "display: none; /* forum display */", "display: block; /* forum display */"
        )

    # enable or disable notepad

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool("ShowNotes", False):
        HTML = HTML.replace(
            "display: none; /* notes display */", "display: block; /* notes display */"
        )
        HTML = HTML.replace("width: 100%; /* thumbs display */", "width: 70%; /* thumbs display */")

    # store variables for further use

    Start.iconbank = iconbank
    Start.tempfolder = tempfolder

    return HTML


def exportTestFile():

    "Allow to check if everything is Ok"

    with codecs.open(
        os.path.expanduser("~") + os.sep + "freecad-startpage.html",
        encoding="utf-8",
        mode="w",
    ) as f:
        f.write(handle())
        f.close()


def postStart(switch_wb=True):

    "executes needed operations after loading a file"

    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")

    # switch workbench
    if switch_wb:
        wb = param.GetString("AutoloadModule", "")
        if "$LastModule" == wb:
            wb = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General").GetString(
                "LastModule", ""
            )
        if wb:
            # don't switch workbenches if we are not in Start anymore
            if FreeCADGui.activeWorkbench() and (
                FreeCADGui.activeWorkbench().name() == "StartWorkbench"
            ):
                FreeCADGui.activateWorkbench(wb)

    # close start tab
    cl = param.GetBool("closeStart", False)
    if cl:
        title = QtGui.QApplication.translate("Workbench", "Start page")
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

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetBool(
        "DoNotShowOnOpen", False
    ) and (not hasattr(Start, "CanOpenStartPage")):
        if len(sys.argv) > 1:
            postStart()
    Start.CanOpenStartPage = True
