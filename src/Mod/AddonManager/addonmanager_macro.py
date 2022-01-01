# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>             *
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

import os
import re
import sys
import codecs
import shutil
from typing import Dict, Union, List

import FreeCAD

from addonmanager_utilities import translate
from addonmanager_utilities import urlopen
from addonmanager_utilities import remove_directory_if_empty

try:
    from HTMLParser import HTMLParser

    unescape = HTMLParser().unescape
except ImportError:
    from html import unescape

#  @package AddonManager_macro
#  \ingroup ADDONMANAGER
#  \brief Unified handler for FreeCAD macros that can be obtained from
#  different sources
#  @{


class Macro(object):
    """This class provides a unified way to handle macros coming from different sources"""

    def __init__(self, name):
        self.name = name
        self.on_wiki = False
        self.on_git = False
        self.desc = ""
        self.code = ""
        self.url = ""
        self.version = ""
        self.src_filename = ""
        self.other_files = []
        self.parsed = False

    def __eq__(self, other):
        return self.filename == other.filename

    @classmethod
    def from_cache(self, cache_dict: Dict):
        instance = Macro(cache_dict["name"])
        for key, value in cache_dict.items():
            instance.__dict__[key] = value
        return instance

    def to_cache(self) -> Dict:
        """For cache purposes this entire class is dumped directly"""

        return self.__dict__

    @property
    def filename(self):
        if self.on_git:
            return os.path.basename(self.src_filename)
        return (self.name + ".FCMacro").replace(" ", "_")

    def is_installed(self):
        if self.on_git and not self.src_filename:
            return False
        return os.path.exists(
            os.path.join(FreeCAD.getUserMacroDir(True), self.filename)
        ) or os.path.exists(
            os.path.join(FreeCAD.getUserMacroDir(True), "Macro_" + self.filename)
        )

    def fill_details_from_file(self, filename):
        with open(filename) as f:
            # Number of parsed fields of metadata.  For now, __Comment__,
            # __Web__, __Version__, __Files__.
            number_of_required_fields = 4
            re_desc = re.compile(r"^__Comment__\s*=\s*(['\"])(.*)\1")
            re_url = re.compile(r"^__Web__\s*=\s*(['\"])(.*)\1")
            re_version = re.compile(r"^__Version__\s*=\s*(['\"])(.*)\1")
            re_files = re.compile(r"^__Files__\s*=\s*(['\"])(.*)\1")
            for line in f.readlines():
                match = re.match(re_desc, line)
                if match:
                    self.desc = match.group(2)
                    number_of_required_fields -= 1
                match = re.match(re_url, line)
                if match:
                    self.url = match.group(2)
                    number_of_required_fields -= 1
                match = re.match(re_version, line)
                if match:
                    self.version = match.group(2)
                    number_of_required_fields -= 1
                match = re.match(re_files, line)
                if match:
                    self.other_files = [of.strip() for of in match.group(2).split(",")]
                    number_of_required_fields -= 1
                if number_of_required_fields <= 0:
                    break
            f.seek(0)
            self.code = f.read()
            self.parsed = True

    def fill_details_from_wiki(self, url):
        code = ""
        u = urlopen(url)
        if u is None:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    f"Could not connect to {url} - check connection and proxy settings",
                )
                + "\n"
            )
            return
        p = u.read()
        if isinstance(p, bytes):
            p = p.decode("utf-8")
        u.close()
        # check if the macro page has its code hosted elsewhere, download if
        # needed
        if "rawcodeurl" in p:
            rawcodeurl = re.findall('rawcodeurl.*?href="(http.*?)">', p)
            if rawcodeurl:
                rawcodeurl = rawcodeurl[0]
                u2 = urlopen(rawcodeurl)
                if u2 is None:
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "AddonsInstaller",
                            "Unable to open macro code URL {rawcodeurl}",
                        )
                        + "\n"
                    )
                    return
                # code = u2.read()
                # github is slow to respond...  We need to use this trick below
                response = ""
                block = 8192
                # expected = int(u2.headers["content-length"])
                while True:
                    # print("expected:", expected, "got:", len(response))
                    data = u2.read(block)
                    if not data:
                        break
                    if isinstance(data, bytes):
                        data = data.decode("utf-8")
                    response += data
                if response:
                    code = response
                u2.close()
        if not code:
            code = re.findall(r"<pre>(.*?)</pre>", p.replace("\n", "--endl--"))
            if code:
                # take the biggest code block
                code = sorted(code, key=len)[-1]
                code = code.replace("--endl--", "\n")
                # Clean HTML escape codes.
                code = unescape(code)
                code = code.replace(b"\xc2\xa0".decode("utf-8"), " ")
            else:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller", "Unable to fetch the code of this macro."
                    )
                    + "\n"
                )

        desc = re.findall(
            r"<td class=\"ctEven left macro-description\">(.*?)</td>",
            p.replace("\n", " "),
        )
        if desc:
            desc = desc[0]
        else:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Unable to retrieve a description for this macro.",
                )
                + "\n"
            )
            desc = "No description available"
        self.desc = desc
        self.url = url
        if isinstance(code, list):
            flat_code = ""
            for chunk in code:
                flat_code += chunk
            code = flat_code
        self.code = code
        self.parsed = True

    def install(self, macro_dir: str) -> (bool, List[str]):
        """Install a macro and all its related files

        Returns True if the macro was installed correctly.

        Parameters
        ----------
        - macro_dir: the directory to install into
        """

        if not self.code:
            return False, ["No code"]
        if not os.path.isdir(macro_dir):
            try:
                os.makedirs(macro_dir)
            except OSError:
                return False, [f"Failed to create {macro_dir}"]
        macro_path = os.path.join(macro_dir, self.filename)
        try:
            with codecs.open(macro_path, "w", "utf-8") as macrofile:
                macrofile.write(self.code)
        except IOError:
            return False, [f"Failed to write {macro_path}"]
        # Copy related files, which are supposed to be given relative to
        # self.src_filename.
        base_dir = os.path.dirname(self.src_filename)
        warnings = []
        for other_file in self.other_files:
            dst_dir = os.path.join(macro_dir, os.path.dirname(other_file))
            if not os.path.isdir(dst_dir):
                try:
                    os.makedirs(dst_dir)
                except OSError:
                    return False, [f"Failed to create {dst_dir}"]
            src_file = os.path.join(base_dir, other_file)
            dst_file = os.path.join(macro_dir, other_file)
            try:
                shutil.copy(src_file, dst_file)
            except IOError:
                warnings.append(f"Failed to copy {src_file} to {dst_file}")
        if len(warnings) > 0:
            return False, warnings

        FreeCAD.Console.PrintLog(f"Macro {self.name} was installed successfully.\n")
        return True, []

    def remove(self) -> bool:
        """Remove a macro and all its related files

        Returns True if the macro was removed correctly.
        """

        if not self.is_installed():
            # Macro not installed, nothing to do.
            return True
        macro_dir = FreeCAD.getUserMacroDir(True)
        macro_path = os.path.join(macro_dir, self.filename)
        macro_path_with_macro_prefix = os.path.join(macro_dir, "Macro_" + self.filename)
        if os.path.exists(macro_path):
            os.remove(macro_path)
        elif os.path.exists(macro_path_with_macro_prefix):
            os.remove(macro_path_with_macro_prefix)
        # Remove related files, which are supposed to be given relative to
        # self.src_filename.
        for other_file in self.other_files:
            dst_file = os.path.join(macro_dir, other_file)
            try:
                os.remove(dst_file)
                remove_directory_if_empty(os.path.dirname(dst_file))
            except Exception:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        f"Failed to remove macro file '{dst_file}': it might not exist, or its permissions changed",
                    )
                    + "\n"
                )
        return True


#  @}
