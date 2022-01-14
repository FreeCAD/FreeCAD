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
import io
import codecs
import shutil
import time
from typing import Dict, Tuple, List, Union

import FreeCAD
from NetworkManager import AM_NETWORK_MANAGER

translate = FreeCAD.Qt.translate

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
        self.comment = ""
        self.code = ""
        self.url = ""
        self.version = ""
        self.date = ""
        self.src_filename = ""
        self.author = ""
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

    def fill_details_from_file(self, filename: str) -> None:
        with open(filename, errors="replace") as f:
            self.code = f.read()
            self.fill_details_from_code(self.code)

    def fill_details_from_code(self, code: str) -> None:
        # Number of parsed fields of metadata. Overrides anything set previously (the code is considered authoritative).
        # For now:
        # __Comment__
        # __Web__
        # __Version__
        # __Files__
        # __Author__
        # __Date__
        max_lines_to_search = 50
        line_counter = 0
        number_of_fields = 5
        ic = re.IGNORECASE  # Shorten the line for Black
        re_comment = re.compile(r"^__Comment__\s*=\s*(['\"])(.*)\1", flags=ic)
        re_url = re.compile(r"^__Web__\s*=\s*(['\"])(.*)\1", flags=ic)
        re_version = re.compile(r"^__Version__\s*=\s*(['\"])(.*)\1", flags=ic)
        re_files = re.compile(r"^__Files__\s*=\s*(['\"])(.*)\1", flags=ic)
        re_author = re.compile(r"^__Author__\s*=\s*(['\"])(.*)\1", flags=ic)
        re_date = re.compile(r"^__Date__\s*=\s*(['\"])(.*)\1", flags=ic)

        f = io.StringIO(code)
        while f and line_counter < max_lines_to_search:
            line = f.readline()
            line_counter += 1
            if not line.startswith(
                "__"
            ):  # Speed things up a bit... this comparison is very cheap
                continue
            match = re.match(re_comment, line)
            if match:
                self.comment = match.group(2)
                self.comment = re.sub("<.*?>", "", self.comment)  # Strip any HTML tags
                number_of_fields -= 1
            match = re.match(re_author, line)
            if match:
                self.author = match.group(2)
                number_of_fields -= 1
            match = re.match(re_url, line)
            if match:
                self.url = match.group(2)
                number_of_fields -= 1
            match = re.match(re_version, line)
            if match:
                self.version = match.group(2)
                number_of_fields -= 1
            match = re.match(re_date, line)
            if match:
                self.date = match.group(2)
                number_of_fields -= 1
            match = re.match(re_files, line)
            if match:
                self.other_files = [of.strip() for of in match.group(2).split(",")]
                number_of_fields -= 1
            if number_of_fields <= 0:
                break

        # Truncate long comments to speed up searches, and clean up display
        if len(self.comment) > 512:
            self.comment = self.comment[:511] + "…"
        self.parsed = True

    def fill_details_from_wiki(self, url):
        code = ""
        p = AM_NETWORK_MANAGER.blocking_get(url)
        if not p:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    f"Unable to open macro wiki page at {url}",
                )
                + "\n"
            )
            return
        p = p.data().decode("utf8")
        # check if the macro page has its code hosted elsewhere, download if
        # needed
        if "rawcodeurl" in p:
            rawcodeurl = re.findall('rawcodeurl.*?href="(http.*?)">', p)
            if rawcodeurl:
                rawcodeurl = rawcodeurl[0]
                u2 = AM_NETWORK_MANAGER.blocking_get(rawcodeurl)
                if not u2:
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "AddonsInstaller",
                            f"Unable to open macro code URL {rawcodeurl}",
                        )
                        + "\n"
                    )
                    return
                code = u2.data().decode("utf8")
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
                    f"Unable to retrieve a description from the wiki for macro {self.name}",
                )
                + "\n"
            )
            desc = "No description available"
        self.desc = desc
        self.comment, _, _ = desc.partition("<br")  # Up to the first line break
        self.comment = re.sub("<.*?>", "", self.comment)  # Strip any tags
        self.url = url
        if isinstance(code, list):
            flat_code = ""
            for chunk in code:
                flat_code += chunk
            code = flat_code
        self.code = code
        self.fill_details_from_code(self.code)
        if not self.author:
            self.author = self.parse_desc("Author: ")
        if not self.date:
            self.date = self.parse_desc("Last modified: ")

    def parse_desc(self, line_start: str) -> Union[str, None]:
        components = self.desc.split(">")
        for component in components:
            if component.startswith(line_start):
                end = component.find("<")
                return component[len(line_start) : end]

    def install(self, macro_dir: str) -> Tuple[bool, List[str]]:
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
