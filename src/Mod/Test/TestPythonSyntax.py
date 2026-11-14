# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 looooo

# ***************************************************************************
# *   Copyright (c) 2018 looooo <sppedflyer@gmail.com>                      *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

import ast
import unittest
import warnings
from pathlib import Path

import FreeCAD as App


def test_python_syntax(rootdir: Path, whitelist=None):
    whitelist = whitelist or []
    log = []
    for file in rootdir.rglob("*.py"):
        if any(rootdir / directory in file.parents for directory in whitelist):
            continue
        if file.is_file():
            with file.open(encoding="utf-8") as py_file:
                try:
                    with warnings.catch_warnings():
                        warnings.simplefilter("error", SyntaxWarning)
                        ast.parse(py_file.read())
                except SyntaxError as err:
                    log.append(str(err).replace("<unknown>", str(file)))
    message = f"\n\n{'#' * 30}\n"
    message += f"{len(log)} python files are not parseable:\n\n"
    for i, m in enumerate(log):
        message += f"{i + 1} {m}\n"
    if log:
        raise RuntimeError(
            f"there are some files not parse-able with the used python-interpreter{message}"
        )
    else:
        return


class PythonSyntaxTestCase(unittest.TestCase):
    """
    Test that all python files in FreeCAD have valid syntax
    """

    def testAll(self):
        mod_dir = Path(App.getHomePath()) / "Mod"
        whitelist = ["pivy"]  # 3rd-party code
        test_python_syntax(mod_dir, whitelist=whitelist)
