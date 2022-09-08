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

import os

import FreeCAD
from addonmanager_git import initialize_git, GitFailed

class Predictor:
    """ Guess the appropriate metadata to apply to a project based on various parameters
    found in the supplied directory. """

    def __init__(self):
        self.path = None
        self.metadata = FreeCAD.Metadata()
        self.license_data = None
        self.license_file = ""

    def predict_metadata(self, path:os.PathLike) -> FreeCAD.Metadata:
        """ Create a predicted Metadata object based on the contents of the passed-in directory """
        self.path = path
        self._predict_author_info()
        self._predict_name()
        self._predict_description()
        self._predict_contents()
        self._predict_icon()
        self._predict_urls()
        self._predict_license()

    def _predict_author_info(self):
        """ Predict the author and maintainer info based on git history """

    def _predict_name(self):
        """ Predict the name based on the local path name and/or the contents of a 
        README.md file. """

    def _predict_description(self):
        """ Predict the description based on the contents of a README.md file. """

    def _predict_contents(self):
        """ Predict the contents based on the contents of the directory. """

    def _predict_icon(self):
        """ Predict the icon based on either a class which defines an Icon member, or
        the contents of the local directory structure. """

    def _predict_urls(self):
        """ Predict the URLs based on git settings """

    def _predict_license(self):
        """ Predict the license based on any existing license file. """

       # These are processed in order, so the BSD 3 clause must come before the 2, for example, because
       # the only difference between them is the additional clause.
        known_strings = {
        "Apache-2.0": (
            "Apache License, Version 2.0",
            "Apache License\nVersion 2.0, January 2004",
        ),
        "BSD-3-Clause": (
            "The 3-Clause BSD License",
            "3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission."
        ),
        "BSD-2-Clause": (
            "The 2-Clause BSD License",
            "2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution."
        ),
        "CC0v1": (
            "CC0 1.0 Universal",
            "voluntarily elects to apply CC0 to the Work and publicly distribute the Work under its terms"
        ),
        "GPLv2": (
            "GNU General Public License version 2",
            "GNU GENERAL PUBLIC LICENSE\nVersion 2, June 1991"
        ),
        "GPLv3": (
            "GNU General Public License version 3",
            "The GNU General Public License is a free, copyleft license for software and other kinds of works."
        ),
        "LGPLv2.1": (
            "GNU Lesser General Public License version 2.1",
            "GNU Lesser General Public License\nVersion 2.1, February 1999"
        ),
        "LGPLv3": (
            "GNU Lesser General Public License version 3",
            "GNU LESSER GENERAL PUBLIC LICENSE\nVersion 3, 29 June 2007"
        ),
        "MIT": (
            "The MIT License",
            "including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software",
        ),
        "MPL-2.0": (
            "Mozilla Public License 2.0",
            "https://opensource.org/licenses/MPL-2.0",
        ),
            }
        self._load_license()
        if self.license_data:
            for license, test_data in known_strings.items():
                if license in self.license_data:
                    self.metadata.License = {"name":license,"file":self.license_file}
                    return
                for test_text in test_data:
                    if test_text in self.license_data
                        self.metadata.License = {"name":license,"file":self.license_file}
                        return

    def _load_readme(self):
        """ Load in any existing readme """
        valid_names = ["README.md", "README.txt", "README"]
        for name in valid_names:
            full_path = os.path.join(self.path,name)
            if os.path.exists(full_path):
                with open(full_path,"r",encoding="utf-8") as f:
                    self.readme_data = f.read()
                    return

    def _load_license(self):
        """ Load in any existing license """
        valid_names = ["LICENSE", "LICENCE", "COPYING","LICENSE.txt", "LICENCE.txt", "COPYING.txt"]
        for name in valid_names:
            full_path = os.path.join(self.path,name)
            if os.path.exists(full_path):
                with open(full_path,"r",encoding="utf-8") as f:
                    self.license_data = f.read()
                    self.license_file = name
                    return