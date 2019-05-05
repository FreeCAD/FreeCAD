#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Yorik van Havre <yorik@uncreated.net>              *
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

import sys
from PySide import QtGui

def translate(context,text):

    "convenience function for the Qt translator"

    try:
        _encoding = QtGui.QApplication.UnicodeUTF8
        u = QtGui.QApplication.translate(context, text, None, _encoding)
    except AttributeError:
        u = QtGui.QApplication.translate(context, text, None)

    if sys.version_info.major < 3:
        u = u.encode("utf8")

    return u.replace(chr(39), "&rsquo;")

T_TITLE = translate("StartPage", "Start")
T_DOCUMENTS = translate("StartPage", "Documents")
T_HELP = translate("StartPage", "Help")
T_ACTIVITY = translate("StartPage", "Activity")
T_RECENTFILES = translate("StartPage", "Recent files")
T_TIP = translate("StartPage", "Tip")
T_ADJUSTRECENT = translate("StartPage", "Adjust the number of recent files to be shown here in menu Edit -> Preferences -> General -> Size of recent file list")
T_EXAMPLES = translate("StartPage", "Examples")
T_GENERALDOCUMENTATION = translate("StartPage", "General documentation")
T_USERHUB = translate("StartPage", "User hub")
T_DESCR_USERHUB = translate("StartPage", "This section contains documentation useful for FreeCAD users in general: a list of all the workbenches, detailed instructions on how to install and use the FreeCAD application, tutorials, and all you need to get started.")
T_POWERHUB = translate("StartPage", "Power users hub")
T_DESCR_POWERHUB = translate("StartPage", "This section gathers documentation for advanced users and people interested in writing python scripts. You will also find there a repository of macros, instructions on how to install and use them, and more information about customizing FreeCAD to your specific needs.")
T_DEVHUB = translate("StartPage", "Developers hub")
T_DESCR_DEVHUB = translate("StartPage", "This section contains material for developers: How to compile FreeCAD yourself, how the FreeCAD source code is structured + how to navigate in it, how to develop new workbenches and/or embed FreeCAD in your own application.")
T_MANUAL = translate("StartPage", "Manual")
T_DESCR_MANUAL = translate("StartPage", "The FreeCAD manual is another, more linear way to present the information contained in this wiki. It is made to be read like a book, and will gently introduce you to many other pages from the hubs above. <a href=\"https://www.gitbook.com/book/yorikvanhavre/a-freecad-manual/details\">e-book versions</a> are also available.")
T_WBHELP = translate("StartPage", "Workbenches documentation")
T_DESCR_WBHELP = translate("StartPage", "These are the help pages of all the workbenches currently installed on this computer.")
T_COMMUNITYHELP = translate("StartPage", "Getting help from the community")
T_DESCR_COMMUNITYHELP1 = translate("StartPage", "The <a href=\"http://forum.freecadweb.org\">FreeCAD forum</a> is a great place to get help from other FreeCAD users and developers. The forum has many sections for different types of issues and discussion subjects. If in doubt, post in the more general <a href=\"https://forum.freecadweb.org/viewforum.php?f=3\">Help on using FreeCAD</a> section.")
T_DESCR_COMMUNITYHELP2 = translate("StartPage", "If it is the first time you are posting on the forum, be sure to <a href=\"https://forum.freecadweb.org/viewtopic.php?f=3&t=2264\">read the guidelines</a> first!")
T_DESCR_COMMUNITYHELP3 = translate("StartPage", "FreeCAD also maintains a public <a href=\"https://www.freecadweb.org/tracker\">bug tracker</a> where anybody can submit bugs and propose new features. To avoid causing extra work and give the best chances to see your bug solved, make sure you read the <a href=\"https://forum.freecadweb.org/viewtopic.php?f=3&t=5236\">bug submission guide</a> before posting.")
T_ADDONS = translate("StartPage", "Available addons")
T_DESCR_ADDONS = translate("StartPage", "Below is a list of available extra workbenches that can be added to your FreeCAD installation. Browse and install them from menu Tools -> Addons manager. You can learn more about any of them by clicking the links below.")
T_OFFLINEHELP = translate("StartPage", "If not bundled with your FreeCAD version, install the FreeCAD documentation package to get documentation hubs, workbench help and individual command documentation without an internet connection.")
T_OFFLINEPLACEHOLDER = translate("StartPage", "Cannot fetch information from GitHub. <a href=\"EnableDownload.py\">Authorize FreeCAD to access the internet</a> and reload the Start page.")
T_RECENTCOMMITS = translate("StartPage", "Recent commits")
T_DESCR_RECENTCOMMITS = translate("StartPage", "Below are the latest changes added to the <a href=\"http://github.com/FreeCAD/FreeCAD/\">FreeCAD source code</a>. These changes might not reflect yet in the FreeCAD version that you are currently running. Check the <a href=\"https://www.freecadweb.org/wiki/Downloads\">available options</a> if you wish to obtain a development version.")
T_SEEONGITHUB = translate("StartPage", "See all commits on github")
T_CUSTOM = translate("StartPage", "You can configure a custom folder to display here in menu Edit -> Preferences -> Start -> Show additional folder")
T_VERSION = translate("StartPage", "version")
T_BUILD = translate("StartPage", "build")
T_CREATENEW = translate("StartPage", "Create new...")
T_UNKNOWN = translate("StartPage", "Unknown")
T_FORUM = translate("StartPage", "Forum")
T_DESCR_FORUM = translate("StartPage", "The latest posts on the <a href=\"https://forum.freecadweb.org\">FreeCAD forum</a>:")
T_EXTERNALLINKS = translate("StartPage", "To open any of the links above in your desktop browser, Right-click -> Open in external browser")
T_CREATIONDATE = translate("StartPage", "Creation date")
T_LASTMODIFIED = translate("StartPage", "Last modification")
T_NOTES = translate("StartPage", "Notes")
