# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>             *
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

import os
import re
import sys
import FreeCAD

from addonmanager_utilities import translate
from addonmanager_utilities import urllib2
from addonmanager_utilities import urlopen


class Macro(object):
    def __init__(self, name):
        self.name = name
        self.on_wiki = False
        self.on_git = False
        self.desc = ''
        self.code = ''
        self.url = ''
        self.version = ''
        self.src_filename = ''
        self.other_files = []
        self.parsed = False

    def __eq__(self, other):
        return self.filename == other.filename

    @property
    def filename(self):
        if self.on_git:
            return os.path.basename(self.src_filename)
        return (self.name + '.FCMacro').replace(' ', '_')

    def is_installed(self):
        if self.on_git and not self.src_filename:
            return False
        return (os.path.exists(os.path.join(FreeCAD.getUserMacroDir(True), self.filename))
                or os.path.exists(os.path.join(FreeCAD.getUserMacroDir(True), 'Macro_' + self.filename)))

    def fill_details_from_file(self, filename):
        with open(filename) as f:
            # Number of parsed fields of metadata. For now, __Comment__,
            # __Web__, __Version__, __Files__.
            number_of_required_fields = 4
            re_desc = re.compile(r"^__Comment__\s*=\s*(['\"])(.*)\1")
            re_url = re.compile(r"^__Web__\s*=\s*(['\"])(.*)\1")
            re_version = re.compile(r"^__Version__\s*=\s*(['\"])(.*)\1")
            re_files = re.compile(r"^__Files__\s*=\s*(['\"])(.*)\1")
            for l in f.readlines():
                match = re.match(re_desc, l)
                if match:
                    self.desc = match.group(2)
                    number_of_required_fields -= 1
                match = re.match(re_url, l)
                if match:
                    self.url = match.group(2)
                    number_of_required_fields -= 1
                match = re.match(re_version, l)
                if match:
                    self.version = match.group(2)
                    number_of_required_fields -= 1
                match = re.match(re_files, l)
                if match:
                    self.other_files = [of.strip() for of in match.group(2).split(',')]
                    number_of_required_fields -= 1
                if number_of_required_fields <= 0:
                    break
            f.seek(0)
            self.code = f.read()
            self.parsed = True

    def fill_details_from_wiki(self, url):
        code = ""
        try:
            u = urlopen(url)
        except urllib2.HTTPError:
            return
        p = u.read()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode('utf-8')
        u.close()
        # check if the macro page has its code hosted elsewhere, download if needed
        if "rawcodeurl" in p:
            rawcodeurl = re.findall("rawcodeurl.*?href=\"(http.*?)\">",p)
            if rawcodeurl:
                rawcodeurl = rawcodeurl[0]
                try:
                    u2 = urlopen(rawcodeurl)
                except urllib2.HTTPError:
                    return
                # code = u2.read()
                # github is slow to respond... We need to use this trick below
                response = ""
                block = 8192
                #expected = int(u2.headers['content-length'])
                while 1:
                    #print("expected:",expected,"got:",len(response))
                    data = u2.read(block)
                    if not data:
                        break
                    if sys.version_info.major >= 3 and isinstance(data, bytes):
                        data = data.decode('utf-8')
                    response += data
                if response:
                    code = response
                u2.close()
        if not code:
            code = re.findall('<pre>(.*?)<\/pre>', p.replace('\n', '--endl--'))
            if code:
                # code = code[0]
                # take the biggest code block
                code = sorted(code, key=len)[-1]
                code = code.replace('--endl--', '\n')
            else:
                FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Unable to fetch the code of this macro."))
            # Clean HTML escape codes.
            try:
                from HTMLParser import HTMLParser
            except ImportError:
                from html.parser import HTMLParser
            if sys.version_info.major < 3:
                code = code.decode('utf8')
            try:
                code = HTMLParser().unescape(code)
                code = code.replace(b'\xc2\xa0'.decode("utf-8"), ' ')
            except:
                FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Unable to clean macro code: ") + code + '\n')
            if sys.version_info.major < 3:
                code = code.encode('utf8')
        desc = re.findall("<td class=\"ctEven left macro-description\">(.*?)<\/td>", p.replace('\n', ' '))
        if desc:
            desc = desc[0]
        else:
            FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Unable to retrieve a description for this macro."))
            desc = "No description available"
        self.desc = desc
        self.url = url
        self.code = code
        self.parsed = True

