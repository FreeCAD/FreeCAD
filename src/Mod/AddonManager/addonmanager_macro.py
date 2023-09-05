# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
# *   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>             *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Unified handler for FreeCAD macros that can be obtained from different sources. """

import os
import re
import io
import codecs
import shutil
from html import unescape
from typing import Dict, Tuple, List, Union, Optional
import urllib.parse

from addonmanager_macro_parser import MacroParser
import addonmanager_utilities as utils

import addonmanager_freecad_interface as fci

translate = fci.translate


#  @package AddonManager_macro
#  \ingroup ADDONMANAGER
#  \brief Unified handler for FreeCAD macros that can be obtained from
#  different sources
#  @{


class Macro:
    """This class provides a unified way to handle macros coming from different
    sources"""

    # Use a stored class variable for this so that we can override it during testing
    blocking_get = None

    # pylint: disable=too-many-instance-attributes
    def __init__(self, name):
        self.name = name
        self.on_wiki = False
        self.on_git = False
        self.desc = ""
        self.comment = ""
        self.code = ""
        self.url = ""
        self.raw_code_url = ""
        self.wiki = ""
        self.version = ""
        self.date = ""
        self.src_filename = ""
        self.author = ""
        self.icon = ""
        self.icon_source = None
        self.xpm = ""  # Possible alternate icon data
        self.other_files = []
        self.parsed = False
        self._console = fci.Console
        if Macro.blocking_get is None:
            Macro.blocking_get = utils.blocking_get

    def __eq__(self, other):
        return self.filename == other.filename

    @classmethod
    def from_cache(cls, cache_dict: Dict):
        """Use data from the cache dictionary to create a new macro, returning a
        reference to it."""
        instance = Macro(cache_dict["name"])
        for key, value in cache_dict.items():
            instance.__dict__[key] = value
        return instance

    def to_cache(self) -> Dict:
        """For cache purposes all public members of the class are returned"""
        cache_dict = {}
        for key, value in self.__dict__.items():
            if key[0] != "_":
                cache_dict[key] = value
        return cache_dict

    @property
    def filename(self):
        """The filename of this macro"""
        if self.on_git:
            return os.path.basename(self.src_filename)
        return (self.name + ".FCMacro").replace(" ", "_")

    def is_installed(self):
        """Returns True if this macro is currently installed (that is, if it exists
        in the user macro directory), or False if it is not. Both the exact filename,
        as well as the filename prefixed with "Macro", are considered an installation
        of this macro.
        """
        if self.on_git and not self.src_filename:
            return False
        return os.path.exists(
            os.path.join(fci.DataPaths().macro_dir, self.filename)
        ) or os.path.exists(os.path.join(fci.DataPaths().macro_dir, "Macro_" + self.filename))

    def fill_details_from_file(self, filename: str) -> None:
        """Opens the given Macro file and parses it for its metadata"""
        with open(filename, errors="replace", encoding="utf-8") as f:
            self.code = f.read()
            self.fill_details_from_code(self.code)

    def fill_details_from_code(self, code: str) -> None:
        """Read the passed-in code and parse it for known metadata elements"""
        parser = MacroParser(self.name, code)
        for key, value in parser.parse_results.items():
            if value:
                self.__dict__[key] = value
        self.clean_icon()
        self.parsed = True

    def fill_details_from_wiki(self, url):
        """For a given URL, download its data and attempt to get the macro's metadata
        out of it. If the macro's code is hosted elsewhere, as specified by a
        "rawcodeurl" found on the wiki page, that code is downloaded and used as the
        source."""
        code = ""
        p = Macro.blocking_get(url)
        if not p:
            self._console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Unable to open macro wiki page at {}",
                ).format(url)
                + "\n"
            )
            return
        p = p.decode("utf8")
        # check if the macro page has its code hosted elsewhere, download if
        # needed
        if "rawcodeurl" in p:
            code = self._fetch_raw_code(p)
        if not code:
            code = self._read_code_from_wiki(p)
        if not code:
            self._console.PrintWarning(
                translate("AddonsInstaller", "Unable to fetch the code of this macro.") + "\n"
            )
            return

        desc = re.findall(
            r"<td class=\"ctEven left macro-description\">(.*?)</td>",
            p.replace("\n", " "),
        )
        if desc:
            desc = desc[0]
        else:
            self._console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Unable to retrieve a description from the wiki for macro {}",
                ).format(self.name)
                + "\n"
            )
            desc = "No description available"
        self.desc = desc
        self.comment, _, _ = desc.partition("<br")  # Up to the first line break
        self.comment = re.sub("<.*?>", "", self.comment)  # Strip any tags
        self.url = url
        if isinstance(code, list):
            code = "".join(code)
        self.code = code
        self.fill_details_from_code(self.code)
        if not self.icon and not self.xpm:
            self.parse_wiki_page_for_icon(p)
        self.clean_icon()

        if not self.author:
            self.author = self.parse_desc("Author: ")
        if not self.date:
            self.date = self.parse_desc("Last modified: ")

    def _fetch_raw_code(self, page_data) -> Optional[str]:
        """Fetch code from the raw code URL specified on the wiki page."""
        code = None
        self.raw_code_url = re.findall('rawcodeurl.*?href="(http.*?)">', page_data)
        if self.raw_code_url:
            self.raw_code_url = self.raw_code_url[0]
            u2 = Macro.blocking_get(self.raw_code_url)
            if not u2:
                self._console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        "Unable to open macro code URL {}",
                    ).format(self.raw_code_url)
                    + "\n"
                )
                return None
            code = u2.decode("utf8")
        return code

    @staticmethod
    def _read_code_from_wiki(p: str) -> Optional[str]:
        code = re.findall(r"<pre>(.*?)</pre>", p.replace("\n", "--endl--"))
        if code:
            # take the biggest code block
            code = sorted(code, key=len)[-1]
            code = code.replace("--endl--", "\n")
            # Clean HTML escape codes.
            code = unescape(code)
            code = code.replace(b"\xc2\xa0".decode("utf-8"), " ")
        return code

    def clean_icon(self):
        """Downloads the macro's icon from whatever source is specified and stores a local
        copy, potentially updating the internal icon location to that local storage."""
        if self.icon.startswith("http://") or self.icon.startswith("https://"):
            self._console.PrintLog(f"Attempting to fetch macro icon from {self.icon}\n")
            parsed_url = urllib.parse.urlparse(self.icon)
            p = Macro.blocking_get(self.icon)
            if p:
                cache_path = fci.DataPaths().cache_dir
                am_path = os.path.join(cache_path, "AddonManager", "MacroIcons")
                os.makedirs(am_path, exist_ok=True)
                _, _, filename = parsed_url.path.rpartition("/")
                base, _, extension = filename.rpartition(".")
                if base.lower().startswith("file:"):
                    self._console.PrintMessage(
                        f"Cannot use specified icon for {self.name}, {self.icon} "
                        "is not a direct download link\n"
                    )
                    self.icon = ""
                else:
                    constructed_name = os.path.join(am_path, base + "." + extension)
                    with open(constructed_name, "wb") as f:
                        f.write(p)
                    self.icon_source = self.icon
                    self.icon = constructed_name
            else:
                self._console.PrintLog(
                    f"MACRO DEVELOPER WARNING: failed to download icon from {self.icon}"
                    f" for macro {self.name}\n"
                )
                self.icon = ""

    def parse_desc(self, line_start: str) -> Union[str, None]:
        """Get data from the wiki for the value specified by line_start."""
        components = self.desc.split(">")
        for component in components:
            if component.startswith(line_start):
                end = component.find("<")
                return component[len(line_start) : end]
        return None

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
        except OSError:
            return False, [f"Failed to write {macro_path}"]
        # Copy related files, which are supposed to be given relative to
        # self.src_filename.
        warnings = []

        self._copy_icon_data(macro_dir, warnings)
        success = self._copy_other_files(macro_dir, warnings)

        if warnings or not success > 0:
            return False, warnings

        self._console.PrintLog(f"Macro {self.name} was installed successfully.\n")
        return True, []

    def _copy_icon_data(self, macro_dir, warnings):
        """Copy any available icon data into the install directory"""
        base_dir = os.path.dirname(self.src_filename)
        if self.xpm:
            xpm_file = os.path.join(base_dir, self.name + "_icon.xpm")
            with open(xpm_file, "w", encoding="utf-8") as f:
                f.write(self.xpm)
        if self.icon:
            if os.path.isabs(self.icon):
                dst_file = os.path.normpath(os.path.join(macro_dir, os.path.basename(self.icon)))
                try:
                    shutil.copy(self.icon, dst_file)
                except OSError:
                    warnings.append(f"Failed to copy icon to {dst_file}")
            elif self.icon not in self.other_files:
                self.other_files.append(self.icon)

    def _copy_other_files(self, macro_dir, warnings) -> bool:
        """Copy any specified "other files" into the install directory"""
        base_dir = os.path.dirname(self.src_filename)
        for other_file in self.other_files:
            if not other_file:
                continue
            if os.path.isabs(other_file):
                dst_dir = macro_dir
            else:
                dst_dir = os.path.join(macro_dir, os.path.dirname(other_file))
            if not os.path.isdir(dst_dir):
                try:
                    os.makedirs(dst_dir)
                except OSError:
                    warnings.append(f"Failed to create {dst_dir}")
                    return False
            if os.path.isabs(other_file):
                src_file = other_file
                dst_file = os.path.normpath(os.path.join(macro_dir, os.path.basename(other_file)))
            else:
                src_file = os.path.normpath(os.path.join(base_dir, other_file))
                dst_file = os.path.normpath(os.path.join(macro_dir, other_file))
            self._fetch_single_file(other_file, src_file, dst_file, warnings)
            try:
                shutil.copy(src_file, dst_file)
            except OSError:
                warnings.append(f"Failed to copy {src_file} to {dst_file}")
        return True  # No fatal errors, but some files may have failed to copy

    def _fetch_single_file(self, other_file, src_file, dst_file, warnings):
        if not os.path.isfile(src_file):
            # If the file does not exist, see if we have a raw code URL to fetch from
            if self.raw_code_url:
                fetch_url = self.raw_code_url.rsplit("/", 1)[0] + "/" + other_file
                self._console.PrintLog(f"Attempting to fetch {fetch_url}...\n")
                p = Macro.blocking_get(fetch_url)
                if p:
                    with open(dst_file, "wb") as f:
                        f.write(p)
                else:
                    self._console.PrintWarning(
                        translate(
                            "AddonsInstaller",
                            "Unable to fetch macro-specified file {} from {}",
                        ).format(other_file, fetch_url)
                        + "\n"
                    )
            else:
                warnings.append(
                    translate(
                        "AddonsInstaller",
                        "Could not locate macro-specified file {} (expected at {})",
                    ).format(other_file, src_file)
                )

    def parse_wiki_page_for_icon(self, page_data: str) -> None:
        """Attempt to find a url for the icon in the wiki page. Sets self.icon if
        found."""

        # Method 1: the text "toolbar icon" appears on the page, and provides a direct
        # link to an icon

        # pylint: disable=line-too-long
        # Try to get an icon from the wiki page itself:
        # <a rel="nofollow" class="external text"
        # href="https://wiki.freecad.org/images/f/f5/blah.png">ToolBar Icon</a>
        icon_regex = re.compile(r'.*href="(.*?)">ToolBar Icon', re.IGNORECASE)
        wiki_icon = ""
        if "ToolBar Icon" in page_data:
            f = io.StringIO(page_data)
            lines = f.readlines()
            for line in lines:
                if ">ToolBar Icon<" in line:
                    match = icon_regex.match(line)
                    if match:
                        wiki_icon = match.group(1)
                        if "file:" not in wiki_icon.lower():
                            self.icon = wiki_icon
                            return
                        break

        # See if we found an icon, but it wasn't a direct link:
        icon_regex = re.compile(r'.*img.*?src="(.*?)"', re.IGNORECASE)
        if wiki_icon.startswith("http"):
            # It's a File: wiki link. We can load THAT page and get the image from it...
            self._console.PrintLog(f"Found a File: link for macro {self.name} -- {wiki_icon}\n")
            p = Macro.blocking_get(wiki_icon)
            if p:
                p = p.decode("utf8")
                f = io.StringIO(p)
                lines = f.readlines()
                trigger = False
                for line in lines:
                    if trigger:
                        match = icon_regex.match(line)
                        if match:
                            wiki_icon = match.group(1)
                            self.icon = "https://wiki.freecad.org/" + wiki_icon
                            return
                    elif "fullImageLink" in line:
                        trigger = True

            #    <div class="fullImageLink" id="file">
            #        <a href="/images/a/a2/Bevel.svg">
            #            <img alt="File:Bevel.svg" src="/images/a/a2/Bevel.svg"
            #            width="64" height="64"/>
            #        </a>


#  @}
