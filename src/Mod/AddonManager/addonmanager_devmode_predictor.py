# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

""" Class to guess metadata based on folder contents. Note that one of the functions
of this file is to guess the license being applied to the new software package based
in its contents. It is up to the user to make the final determination about whether
the selected license is the correct one, and inclusion here shouldn't be construed as
endorsement of any particular license. In addition, the inclusion of those text strings
does not imply a modification to the license for THIS software, which is licensed
under the LGPLv2.1 license (as stated above)."""

import datetime
import os

import FreeCAD
from addonmanager_git import initialize_git, GitManager
from addonmanager_utilities import get_readme_url

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods


class AddonSlice:
    """A tiny class to implement duck-typing for the URL-parsing utility functions"""

    def __init__(self, url, branch):
        self.url = url
        self.branch = branch


class Predictor:
    """Guess the appropriate metadata to apply to a project based on various parameters
    found in the supplied directory."""

    def __init__(self):
        self.path = None
        self.metadata = FreeCAD.Metadata()
        self.license_data = None
        self.readme_data = None
        self.license_file = ""
        self.git_manager: GitManager = initialize_git()
        if not self.git_manager:
            raise Exception("Cannot use Developer Mode without git installed")

    def predict_metadata(self, path: str) -> FreeCAD.Metadata:
        """Create a predicted Metadata object based on the contents of the passed-in directory"""
        if not os.path.isdir(path):
            return None
        self.path = path
        self._predict_author_info()
        self._predict_name()
        self._predict_description()
        self._predict_contents()
        self._predict_icon()
        self._predict_urls()
        self._predict_license()
        self._predict_version()

        return self.metadata

    def _predict_author_info(self):
        """Look at the git commit history and attempt to discern maintainer and author
        information."""

        committers = self.git_manager.get_last_committers(self.path)

        # This is a dictionary keyed to the author's name (which can be many
        # things, depending on the author) containing two fields, "email" and "count". It
        # is common for there to be multiple entries representing the same human being,
        # so a passing attempt is made to reconcile:
        filtered_committers = {}
        for key, committer in committers.items():
            if "github" in key.lower():
                # Robotic merge commit (or other similar), ignore
                continue
            # Does any other committer share any of these emails?
            for other_key, other_committer in committers.items():
                if other_key == key:
                    continue
                for other_email in other_committer["email"]:
                    if other_email in committer["email"]:
                        # There is overlap in the two email lists, so this is probably the
                        # same author, with a different name (username, pseudonym, etc.)
                        if not committer["aka"]:
                            committer["aka"] = set()
                        committer["aka"].add(other_key)
                        committer["count"] += other_committer["count"]
                        committer["email"].combine(other_committer["email"])
                        committers.pop(other_key)
                        break
            filtered_committers[key] = committer
        maintainers = []
        for name, info in filtered_committers.items():
            if "aka" in info:
                for other_name in info["aka"]:
                    # Heuristic: the longer name is more likely to be the actual legal name
                    if len(other_name) > len(name):
                        name = other_name
            # There is no logical basis to choose one email address over another, so just
            # take the first one
            email = info["email"][0]
            commit_count = info["count"]
            maintainers.append({"name": name, "email": email, "count": commit_count})

        # Sort by count of commits
        maintainers.sort(key=lambda i: i["count"], reverse=True)

        self.metadata.Maintainer = maintainers

    def _predict_name(self):
        """Predict the name based on the local path name and/or the contents of a
        README.md file."""

        normed_path = self.path.replace(os.path.sep, "/")
        path_components = normed_path.split("/")
        final_path_component = path_components[-1]
        predicted_name = final_path_component.replace("/", "")
        self.metadata.Name = predicted_name

    def _predict_description(self):
        """Predict the description based on the contents of a README.md file."""
        self._load_readme()

        if not self.readme_data:
            return

        lines = self.readme_data.split("\n")
        description = ""
        for line in lines:
            if "#" in line:
                continue  # Probably not a line of description
            if "![" in line:
                continue  # An image link, probably separate from any description
            if not line and description:
                break  # We're done: this is a blank line, and we've read some data already
            if description:
                description += " "
            description += line

        if description:
            self.metadata.Description = description

    def _predict_contents(self):
        """Predict the contents based on the contents of the directory."""

    def _predict_icon(self):
        """Predict the icon based on either a class which defines an Icon member, or
        the contents of the local directory structure."""

    def _predict_urls(self):
        """Predict the URLs based on git settings"""

        branch = self.git_manager.current_branch(self.path)
        remote = self.git_manager.get_remote(self.path)

        addon = AddonSlice(remote, branch)
        readme = get_readme_url(addon)

        self.metadata.addUrl("repository", remote, branch)
        self.metadata.addUrl("readme", readme)

    def _predict_license(self):
        """Predict the license based on any existing license file."""

        # These are processed in order, so the BSD 3 clause must come before the 2, for example,
        # because the only difference between them is the additional clause.
        known_strings = {
            "Apache-2.0": (
                "Apache License, Version 2.0",
                "Apache License\nVersion 2.0, January 2004",
            ),
            "BSD-3-Clause": (
                "The 3-Clause BSD License",
                "3. Neither the name of the copyright holder nor the names of its contributors \
                may be used to endorse or promote products derived from this software without \
                specific prior written permission.",
            ),
            "BSD-2-Clause": (
                "The 2-Clause BSD License",
                "2. Redistributions in binary form must reproduce the above copyright notice, \
                this list of conditions and the following disclaimer in the documentation and/or \
                other materials provided with the distribution.",
            ),
            "CC0v1": (
                "CC0 1.0 Universal",
                "voluntarily elects to apply CC0 to the Work and publicly distribute the Work \
                under its terms",
            ),
            "GPLv2": (
                "GNU General Public License version 2",
                "GNU GENERAL PUBLIC LICENSE\nVersion 2, June 1991",
            ),
            "GPLv3": (
                "GNU General Public License version 3",
                "The GNU General Public License is a free, copyleft license for software and \
                other kinds of works.",
            ),
            "LGPLv2.1": (
                "GNU Lesser General Public License version 2.1",
                "GNU Lesser General Public License\nVersion 2.1, February 1999",
            ),
            "LGPLv3": (
                "GNU Lesser General Public License version 3",
                "GNU LESSER GENERAL PUBLIC LICENSE\nVersion 3, 29 June 2007",
            ),
            "MIT": (
                "The MIT License",
                "including without limitation the rights to use, copy, modify, merge, publish, \
                distribute, sublicense, and/or sell copies of the Software",
            ),
            "MPL-2.0": (
                "Mozilla Public License 2.0",
                "https://opensource.org/licenses/MPL-2.0",
            ),
        }
        self._load_license()
        if self.license_data:
            for shortcode, test_data in known_strings.items():
                if shortcode.lower() in self.license_data.lower():
                    self.metadata.addLicense(shortcode, self.license_file)
                    return
                for test_text in test_data:
                    # Do the comparison without regard to whitespace or capitalization
                    if (
                        "".join(test_text.split()).lower()
                        in "".join(self.license_data.split()).lower()
                    ):
                        self.metadata.addLicense(shortcode, self.license_file)
                        return

    def _predict_version(self):
        """Default to a CalVer style set to today's date"""
        year = datetime.date.today().year
        month = datetime.date.today().month
        day = datetime.date.today().day
        version_string = f"{year}.{month:>02}.{day:>02}"
        self.metadata.Version = version_string

    def _load_readme(self):
        """Load in any existing readme"""
        valid_names = ["README.md", "README.txt", "README"]
        for name in valid_names:
            full_path = os.path.join(self.path, name)
            if os.path.exists(full_path):
                with open(full_path, encoding="utf-8") as f:
                    self.readme_data = f.read()
                    return

    def _load_license(self):
        """Load in any existing license"""
        valid_names = [
            "LICENSE",
            "LICENCE",
            "COPYING",
            "LICENSE.txt",
            "LICENCE.txt",
            "COPYING.txt",
        ]
        for name in valid_names:
            full_path = os.path.join(self.path.replace("/", os.path.sep), name)
            if os.path.isfile(full_path):
                with open(full_path, encoding="utf-8") as f:
                    self.license_data = f.read()
                    self.license_file = name
                    return
