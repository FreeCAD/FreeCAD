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

""" Unified handler for FreeCAD macros that can be obtained from different sources. """

import os
import re
import io
import codecs
import shutil
from html import unescape
from typing import Dict, Tuple, List, Union, Optional

import FreeCAD
import NetworkManager
from PySide import QtCore

from addonmanager_utilities import remove_directory_if_empty, is_float

translate = FreeCAD.Qt.translate


#  @package AddonManager_macro
#  \ingroup ADDONMANAGER
#  \brief Unified handler for FreeCAD macros that can be obtained from
#  different sources
#  @{


class Macro:
    """This class provides a unified way to handle macros coming from different sources"""

    # Use a stored class variable for this so that we can override it during testing
    network_manager = None

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

    def __eq__(self, other):
        return self.filename == other.filename

    @classmethod
    def from_cache(cls, cache_dict: Dict):
        """Use data from the cache dictionary to create a new macro, returning a reference
        to it."""
        instance = Macro(cache_dict["name"])
        for key, value in cache_dict.items():
            instance.__dict__[key] = value
        return instance

    def to_cache(self) -> Dict:
        """For cache purposes this entire class is dumped directly"""

        return self.__dict__

    @classmethod
    def _get_network_manager(cls):
        if cls.network_manager is None:
            # Make sure we're initialized:
            NetworkManager.InitializeNetworkManager()
            cls.network_manager = NetworkManager.AM_NETWORK_MANAGER
        return cls.network_manager

    @property
    def filename(self):
        """The filename of this macro"""
        if self.on_git:
            return os.path.basename(self.src_filename)
        return (self.name + ".FCMacro").replace(" ", "_")

    def is_installed(self):
        """Returns True if this macro is currently installed (that is, if it exists in the
        user macro directory), or False if it is not. Both the exact filename, as well as
        the filename prefixed with "Macro", are considered an installation of this macro."""
        if self.on_git and not self.src_filename:
            return False
        return os.path.exists(
            os.path.join(FreeCAD.getUserMacroDir(True), self.filename)
        ) or os.path.exists(
            os.path.join(FreeCAD.getUserMacroDir(True), "Macro_" + self.filename)
        )

    def fill_details_from_file(self, filename: str) -> None:
        """Opens the given Macro file and parses it for its metadata"""
        with open(filename, errors="replace", encoding="utf-8") as f:
            self.code = f.read()
            self.fill_details_from_code(self.code)

    def fill_details_from_code(self, code: str) -> None:
        """Reads in the macro code from the given string and parses it for its metadata."""
        # Number of parsed fields of metadata. Overrides anything set previously (the code is
        # considered authoritative).
        # For now:
        # __Comment__
        # __Web__
        # __Wiki__
        # __Version__
        # __Files__
        # __Author__
        # __Date__
        # __Icon__
        max_lines_to_search = 200
        line_counter = 0

        string_search_mapping = {
            "__comment__": "comment",
            "__web__": "url",
            "__wiki__": "wiki",
            "__version__": "version",
            "__files__": "other_files",
            "__author__": "author",
            "__date__": "date",
            "__icon__": "icon",
            "__xpm__": "xpm",
        }

        string_search_regex = re.compile(r"\s*(['\"])(.*)\1")
        f = io.StringIO(code)
        while f and line_counter < max_lines_to_search:
            line = f.readline()
            if not line:
                break
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            line_counter += 1
            if not line.startswith("__"):
                # Speed things up a bit... this comparison is very cheap
                continue

            lowercase_line = line.lower()
            for key, value in string_search_mapping.items():
                if lowercase_line.startswith(key):
                    _, _, after_equals = line.partition("=")
                    match = re.match(string_search_regex, after_equals)

                    # We do NOT support triple-quoted strings, except for the icon XPM data
                    # Most cases should be caught by this code
                    if match and '"""' not in after_equals:
                        self._standard_extraction(value, match.group(2))
                        string_search_mapping.pop(key)
                        break

                    # For cases where either there is no match, or we found a triple quote,
                    # more processing is needed

                    # Macro authors are supposed to be providing strings here, but in some
                    # cases they are not doing so. If this is the "__version__" tag, try
                    # to apply some special handling to accepts numbers, and "__date__"
                    if key == "__version__":
                        self._process_noncompliant_version(after_equals)
                        string_search_mapping.pop(key)
                        break

                    # Icon data can be actual embedded XPM data, inside a triple-quoted string
                    if key in ("__icon__", "__xpm__"):
                        self._process_icon(f, key, after_equals)
                        string_search_mapping.pop(key)
                        break

                    FreeCAD.Console.PrintError(
                        translate(
                            "AddonsInstaller",
                            "Syntax error while reading {} from macro {}",
                        ).format(key, self.name)
                        + "\n"
                    )
                    FreeCAD.Console.PrintError(line + "\n")

        # Do some cleanup of the values:
        if self.comment:
            self.comment = re.sub("<.*?>", "", self.comment)  # Strip any HTML tags

        # Truncate long comments to speed up searches, and clean up display
        if len(self.comment) > 512:
            self.comment = self.comment[:511] + "…"

        # Make sure the icon is not an absolute path, etc.
        self.clean_icon()

        self.parsed = True

    def _standard_extraction(self, value: str, match_group):
        """For most macro metadata values, this extracts the required data"""
        if isinstance(self.__dict__[value], str):
            self.__dict__[value] = match_group
        elif isinstance(self.__dict__[value], list):
            self.__dict__[value] = [of.strip() for of in match_group.split(",")]
        else:
            FreeCAD.Console.PrintError(
                "Internal Error: bad type in addonmanager_macro class.\n"
            )

    def _process_noncompliant_version(self, after_equals):
        if "__date__" in after_equals.lower():
            self.version = self.date
        elif is_float(after_equals):
            self.version = str(after_equals).strip()
        else:
            FreeCAD.Console.PrintLog(
                f"Unrecognized value for __version__ in macro {self.name}"
            )
            self.version = "(Unknown)"

    def _process_icon(self, f, key, after_equals):
        # If this is an icon, it's possible that the icon was actually directly
        # specified in the file as XPM data. This data **must** be between
        # triple double quotes in order for the Addon Manager to recognize it.
        if '"""' in after_equals:
            _, _, xpm_data = after_equals.partition('"""')
            while True:
                line = f.readline()
                if not line:
                    FreeCAD.Console.PrintError(
                        translate(
                            "AddonsInstaller",
                            "Syntax error while reading {} from macro {}",
                        ).format(key, self.name)
                        + "\n"
                    )
                    break
                if '"""' in line:
                    last_line, _, _ = line.partition('"""')
                    xpm_data += last_line
                    break
                xpm_data += line
            self.xpm = xpm_data

    def fill_details_from_wiki(self, url):
        """For a given URL, download its data and attempt to get the macro's metadata out of
        it. If the macro's code is hosted elsewhere, as specified by a "rawcodeurl" found on
        the wiki page, that code is downloaded and used as the source."""
        code = ""
        nm = Macro._get_network_manager()
        p = nm.blocking_get(url)
        if not p:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Unable to open macro wiki page at {}",
                ).format(url)
                + "\n"
            )
            return
        p = p.data().decode("utf8")
        # check if the macro page has its code hosted elsewhere, download if
        # needed
        if "rawcodeurl" in p:
            code = self._fetch_raw_code(p)
        if not code:
            code = self._read_code_from_wiki(p)
        if not code:
            FreeCAD.Console.PrintWarning(
                translate("AddonsInstaller", "Unable to fetch the code of this macro.")
                + "\n"
            )
            return

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
        """Fetch code from the rawcodeurl specified on the wiki page."""
        code = None
        self.raw_code_url = re.findall('rawcodeurl.*?href="(http.*?)">', page_data)
        if self.raw_code_url:
            self.raw_code_url = self.raw_code_url[0]
            nm = Macro._get_network_manager()
            u2 = nm.blocking_get(self.raw_code_url)
            if not u2:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        "Unable to open macro code URL {}",
                    ).format(self.raw_code_url)
                    + "\n"
                )
                return None
            code = u2.data().decode("utf8")
        return code

    def _read_code_from_wiki(self, p: str) -> Optional[str]:
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
            FreeCAD.Console.PrintLog(
                f"Attempting to fetch macro icon from {self.icon}\n"
            )
            nm = Macro._get_network_manager()
            p = nm.blocking_get(self.icon)
            if p:
                cache_path = FreeCAD.getUserCachePath()
                am_path = os.path.join(cache_path, "AddonManager", "MacroIcons")
                os.makedirs(am_path, exist_ok=True)
                _, _, filename = self.icon.rpartition("/")
                base, _, extension = filename.rpartition(".")
                if base.lower().startswith("file:"):
                    # pylint: disable=line-too-long
                    FreeCAD.Console.PrintMessage(
                        f"Cannot use specified icon for {self.name}, {self.icon} is not a direct download link\n"
                    )
                    self.icon = ""
                else:
                    constructed_name = os.path.join(am_path, base + "." + extension)
                    with open(constructed_name, "wb") as f:
                        f.write(p.data())
                    self.icon_source = self.icon
                    self.icon = constructed_name
            else:
                # pylint: disable=line-too-long
                FreeCAD.Console.PrintLog(
                    f"MACRO DEVELOPER WARNING: failed to download icon from {self.icon} for macro {self.name}\n"
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
        except IOError:
            return False, [f"Failed to write {macro_path}"]
        # Copy related files, which are supposed to be given relative to
        # self.src_filename.
        warnings = []

        self._copy_icon_data(macro_dir, warnings)
        success = self._copy_other_files(macro_dir, warnings)

        if warnings or not success > 0:
            return False, warnings

        FreeCAD.Console.PrintLog(f"Macro {self.name} was installed successfully.\n")
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
                dst_file = os.path.normpath(
                    os.path.join(macro_dir, os.path.basename(self.icon))
                )
                try:
                    shutil.copy(self.icon, dst_file)
                except IOError:
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
                dst_file = os.path.normpath(
                    os.path.join(macro_dir, os.path.basename(other_file))
                )
            else:
                src_file = os.path.normpath(os.path.join(base_dir, other_file))
                dst_file = os.path.normpath(os.path.join(macro_dir, other_file))
            self._fetch_single_file(other_file, src_file, dst_file, warnings)
            try:
                shutil.copy(src_file, dst_file)
            except IOError:
                warnings.append(f"Failed to copy {src_file} to {dst_file}")
        return True  # No fatal errors, but some files may have failed to copy

    def _fetch_single_file(self, other_file, src_file, dst_file, warnings):
        if not os.path.isfile(src_file):
            # If the file does not exist, see if we have a raw code URL to fetch from
            if self.raw_code_url:
                fetch_url = self.raw_code_url.rsplit("/", 1)[0] + "/" + other_file
                FreeCAD.Console.PrintLog(f"Attempting to fetch {fetch_url}...\n")
                nm = Macro._get_network_manager()
                p = nm.blocking_get(fetch_url)
                if p:
                    with open(dst_file, "wb") as f:
                        f.write(p)
                else:
                    FreeCAD.Console.PrintWarning(
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
                        "Could not locate macro-specified file {} (should have been at {})",
                    ).format(other_file, src_file)
                )

    def remove(self) -> bool:
        """Remove a macro and all its related files

        Returns True if the macro was removed correctly.
        """

        if not self.is_installed():
            # Macro not installed, nothing to do.
            return True
        macro_dir = FreeCAD.getUserMacroDir(True)

        try:
            self._remove_core_macro_file(macro_dir)
            self._remove_xpm_data(macro_dir)
            self._remove_other_files(macro_dir)
        except IsADirectoryError:
            FreeCAD.Console.PrintError(
                translate(
                    "AddonsInstaller",
                    "Tried to remove a directory when a file was expected\n",
                )
            )
            return False
        except FileNotFoundError:
            FreeCAD.Console.PrintError(
                translate(
                    "AddonsInstaller",
                    "Macro file could not be found, nothing to remove\n",
                )
            )
            return False
        return True

    def _remove_other_files(self, macro_dir):
        # Remove related files, which are supposed to be given relative to
        # self.src_filename.
        for other_file in self.other_files:
            if not other_file:
                continue
            FreeCAD.Console.PrintMessage(other_file + "...")
            dst_file = os.path.join(macro_dir, other_file)
            if not dst_file or not os.path.exists(dst_file):
                FreeCAD.Console.PrintMessage("X\n")
                continue
            try:
                os.remove(dst_file)
                remove_directory_if_empty(os.path.dirname(dst_file))
                FreeCAD.Console.PrintMessage("✓\n")
            except IsADirectoryError:
                FreeCAD.Console.PrintMessage(" is a directory, not removed\n")
            except FileNotFoundError:
                FreeCAD.Console.PrintMessage(" could not be found, nothing to remove\n")
        if os.path.isabs(self.icon):
            dst_file = os.path.normpath(
                os.path.join(macro_dir, os.path.basename(self.icon))
            )
            if os.path.exists(dst_file):
                try:
                    FreeCAD.Console.PrintMessage(os.path.basename(self.icon) + "...")
                    os.remove(dst_file)
                    FreeCAD.Console.PrintMessage("✓\n")
                except IsADirectoryError:
                    FreeCAD.Console.PrintMessage(" is a directory, not removed\n")
                except FileNotFoundError:
                    FreeCAD.Console.PrintMessage(
                        " could not be found, nothing to remove\n"
                    )
        return True

    def _remove_core_macro_file(self, macro_dir):
        macro_path = os.path.join(macro_dir, self.filename)
        macro_path_with_macro_prefix = os.path.join(macro_dir, "Macro_" + self.filename)
        if os.path.exists(macro_path):
            os.remove(macro_path)
        elif os.path.exists(macro_path_with_macro_prefix):
            os.remove(macro_path_with_macro_prefix)

    def _remove_xpm_data(self, macro_dir):
        if self.xpm:
            xpm_file = os.path.join(macro_dir, self.name + "_icon.xpm")
            if os.path.exists(xpm_file):
                os.remove(xpm_file)

    def parse_wiki_page_for_icon(self, page_data: str) -> None:
        """Attempt to find a url for the icon in the wiki page. Sets self.icon if found."""

        # Method 1: the text "toolbar icon" appears on the page, and provides a direct
        # link to an icon

        # pylint: disable=line-too-long
        # Try to get an icon from the wiki page itself:
        # <a rel="nofollow" class="external text" href="https://www.freecadweb.org/wiki/images/f/f5/Macro_3D_Parametric_Curve.png">ToolBar Icon</a>
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
            FreeCAD.Console.PrintLog(
                f"Found a File: link for macro {self.name} -- {wiki_icon}\n"
            )
            nm = Macro._get_network_manager()
            p = nm.blocking_get(wiki_icon)
            if p:
                p = p.data().decode("utf8")
                f = io.StringIO(p)
                lines = f.readlines()
                trigger = False
                for line in lines:
                    if trigger:
                        match = icon_regex.match(line)
                        if match:
                            wiki_icon = match.group(1)
                            self.icon = "https://www.freecadweb.org/wiki" + wiki_icon
                            return
                    elif "fullImageLink" in line:
                        trigger = True

            #    <div class="fullImageLink" id="file">
            #        <a href="/images/a/a2/Bevel.svg">
            #            <img alt="File:Bevel.svg" src="/images/a/a2/Bevel.svg" width="64" height="64"/>
            #        </a>


#  @}
