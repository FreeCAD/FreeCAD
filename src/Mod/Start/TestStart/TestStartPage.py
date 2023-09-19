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
import re


class TestStartPage(unittest.TestCase):
    """Basic validation of the generated Start page."""

    MODULE = "TestStartPage"  # file name without extension

    def setUp(self):
        pass

    def test_all_css_placeholders_removed(self):
        """Check to see if all of the CSS placeholders have been replaced."""
        placeholders = [
            "BACKGROUND",
            "BGTCOLOR",
            "FONTFAMILY",
            "FONTSIZE",
            "LINKCOLOR",
            "TEXTCOLOR",
            "BOXCOLOR",
            "BASECOLOR",
            "SHADOW",
        ]

        page = StartPage.handle()
        for placeholder in placeholders:
            self.assertNotIn(
                placeholder, page, "{} was not removed from the CSS".format(placeholder)
            )

    def test_all_js_placeholders_removed(self):
        """Check to see if all of the JavaScript placeholders have been replaced."""
        placeholders = ["IMAGE_SRC_INSTALLED"]
        page = StartPage.handle()
        for placeholder in placeholders:
            self.assertNotIn(
                placeholder, page, "{} was not removed from the JS".format(placeholder)
            )

    def test_all_html_placeholders_removed(self):
        """Check to see if all of the HTML placeholders have been replaced."""
        placeholders = [
            "T_TITLE",
            "VERSIONSTRING",
            "T_DOCUMENTS",
            "T_HELP",
            "T_ACTIVITY",
            "SECTION_RECENTFILES",
            "T_TIP",
            "T_ADJUSTRECENT",
            "SECTION_EXAMPLES",
            "SECTION_CUSTOM",
            "T_CUSTOM",
            "T_NOTES",
            "T_GENERALDOCUMENTATION",
            "IMAGE_SRC_USERHUB",
            "T_USERHUB",
            "T_DESCR_USERHUB",
            "IMAGE_SRC_POWERHUB",
            "T_POWERHUB",
            "T_DESCR_POWERHUB",
            "IMAGE_SRC_DEVHUB",
            "T_DEVHUB",
            "T_DESCR_DEVHUB",
            "IMAGE_SRC_MANUAL",
            "T_MANUAL",
            "T_DESCR_MANUAL",
            "T_WBHELP",
            "T_DESCR_WBHELP",
            "UL_WORKBENCHES",
            "T_COMMUNITYHELP",
            "T_DESCR_COMMUNITYHELP1",
            "T_DESCR_COMMUNITYHELP2",
            "T_DESCR_COMMUNITYHELP3",
            "T_ADDONS",
            "T_DESCR_ADDONS",
            "T_OFFLINEPLACEHOLDER",
            "T_OFFLINEHELP",
            "T_EXTERNALLINKS",
            "T_RECENTCOMMITS",
            "T_DESCR_RECENTCOMMITS",
            "T_EXTERNALLINKS",
            "T_SEEONGITHUB",
            "T_FORUM",
            "T_DESCR_FORUM",
        ]
        page = StartPage.handle()
        for placeholder in placeholders:
            self.assertNotIn(
                placeholder, page, "{} was not removed from the HTML".format(placeholder)
            )

    def test_files_do_not_contain_backslashes(self):
        # This would be caught by the W3C validator if we didn't sanitize the filenames before sending them.
        page = StartPage.handle()
        fileRE = re.compile(r'"file:///(.*?)"')
        results = fileRE.findall(string=page)

        badFilenames = []
        for result in results:
            if result.find("\\") != -1:
                badFilenames.append(result)

        if len(badFilenames) > 0:
            self.fail(
                "The following filenames contain backslashes, which is prohibited in HTML: {}".format(
                    badFilenames
                )
            )

    def test_html_validates(self):
        # Send the generated html to the W3C validator for analysis (removing potentially-sensitive data first)
        import urllib.request
        import os
        import json

        page = self.sanitize(StartPage.handle())  # Remove potentially sensitive data

        # For debugging, if you want to ensure that the sanitization worked correctly:
        # from pathlib import Path
        # home = str(Path.home())
        # f=open(home+"/test.html", "w")
        # f.write(page)
        # f.close()

        validation_url = "https://validator.w3.org/nu/?out=json"
        data = page.encode("utf-8")  # data should be bytes
        req = urllib.request.Request(validation_url, data)
        req.add_header("Content-type", "text/html; charset=utf-8")
        errorCount = 0
        warningCount = 0
        infoCount = 0
        validationResultString = ""
        try:
            with urllib.request.urlopen(req) as response:
                text = response.read()

                responseJSON = json.loads(text)

                for message in responseJSON["messages"]:
                    if "type" in message:
                        if message["type"] == "info":
                            if "subtype" in message:
                                if message["subtype"] == "warning":
                                    warningCount += 1
                                    validationResultString += "WARNING: {}\n".format(
                                        ascii(message["message"])
                                    )
                            else:
                                infoCount += 1
                                validationResultString += "INFO: {}\n".format(
                                    ascii(message["message"])
                                )
                        elif message["type"] == "error":
                            errorCount += 1
                            validationResultString += "ERROR: {}\n".format(
                                ascii(message["message"])
                            )
                        elif message["type"] == "non-document-error":
                            FreeCAD.Console.PrintWarning(
                                "W3C validator returned a non-document error:\n {}".format(message)
                            )
                            return

        except urllib.error.HTTPError as e:
            FreeCAD.Console.PrintWarning("W3C validator returned response code {}".format(e.code))

        except urllib.error.URLError:
            FreeCAD.Console.PrintWarning("Could not communicate with W3C validator")

        if errorCount > 0 or warningCount > 0:
            StartPage.exportTestFile()
            FreeCAD.Console.PrintWarning(
                "HTML validation failed: Start page source written to your home directory for analysis."
            )
            self.fail(
                "W3C Validator analysis shows the Start page has {} errors and {} warnings:\n\n{}".format(
                    errorCount, warningCount, validationResultString
                )
            )
        elif infoCount > 0:
            FreeCAD.Console.PrintWarning(
                "The Start page is valid HTML, but the W3C sent back {} informative messages:\n{}.".format(
                    infoCount, validationResultString
                )
            )

    def sanitize(self, html):

        # Anonymize all local filenames
        fileRE = re.compile(r'"file:///.*?"')
        html = fileRE.sub(repl=r'"file:///A/B/C"', string=html)

        # Anonymize titles, which are used for mouseover text and might contain document information
        titleRE = re.compile(r'title="[\s\S]*?"')  # Some titles have newlines in them
        html = titleRE.sub(repl=r'title="Y"', string=html)

        # Anonymize the document names, which we display in <h4> tags
        h4RE = re.compile(r"<h4>.*?</h4>")
        html = h4RE.sub(repl=r"<h4>Z</h4>", string=html)

        # Remove any simple single-line paragraphs, which might contain document author information, file size information, etc.
        pRE = re.compile(r"<p>[^<]*?</p>")
        html = pRE.sub(repl=r"<p>X</p>", string=html)

        return html
