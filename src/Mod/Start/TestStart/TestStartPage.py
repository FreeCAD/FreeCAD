# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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

import unittest
import FreeCAD
import Start
from StartPage import StartPage
from html.parser import HTMLParser

class TestStartPage(unittest.TestCase):
    """Basic validation of the generated Start page."""

    MODULE = 'TestStartPage' # file name without extension


    def setUp(self):
        pass

    def test_all_css_placeholders_removed(self):
        """Check to see if all of the CSS placeholders have been replaced."""
        placeholders = ["BACKGROUND","BGTCOLOR","FONTFAMILY","FONTSIZE","LINKCOLOR",
                        "TEXTCOLOR","BOXCOLOR","BASECOLOR","SHADOW"]
        
        page = StartPage.handle()
        for placeholder in placeholders:
            self.assertNotIn (placeholder, page, "{} was not removed from the CSS".format(placeholder))

    def test_all_js_placeholders_removed(self):
        """Check to see if all of the JavaScript placeholders have been replaced."""
        placeholders = ["IMAGE_SRC_INSTALLED"]
        page = StartPage.handle()
        for placeholder in placeholders:
            self.assertNotIn (placeholder, page, "{} was not removed from the JS".format(placeholder))

    def test_all_html_placeholders_removed(self):
        """Check to see if all of the HTML placeholders have been replaced."""
        placeholders = ["T_TITLE","VERSIONSTRING","T_DOCUMENTS","T_HELP","T_ACTIVITY",
                        "SECTION_RECENTFILES","T_TIP","T_ADJUSTRECENT","SECTION_EXAMPLES",
                        "SECTION_CUSTOM","T_CUSTOM","T_NOTES","T_GENERALDOCUMENTATION",
                        "IMAGE_SRC_USERHUB", "T_USERHUB", "T_DESCR_USERHUB",
                        "IMAGE_SRC_POWERHUB","T_POWERHUB","T_DESCR_POWERHUB",
                        "IMAGE_SRC_DEVHUB",  "T_DEVHUB",  "T_DESCR_DEVHUB",
                        "IMAGE_SRC_MANUAL",  "T_MANUAL",  "T_DESCR_MANUAL",
                        "T_WBHELP","T_DESCR_WBHELP","UL_WORKBENCHES",
                        "T_COMMUNITYHELP","T_DESCR_COMMUNITYHELP1","T_DESCR_COMMUNITYHELP2",
                        "T_DESCR_COMMUNITYHELP3","T_ADDONS","T_DESCR_ADDONS",
                        "T_OFFLINEPLACEHOLDER","T_OFFLINEHELP","T_EXTERNALLINKS",
                        "T_RECENTCOMMITS","T_DESCR_RECENTCOMMITS","T_EXTERNALLINKS",
                        "T_SEEONGITHUB","T_FORUM","T_DESCR_FORUM"]
        page = StartPage.handle()
        for placeholder in placeholders:
            self.assertNotIn (placeholder, page, "{} was not removed from the HTML".format(placeholder))
    