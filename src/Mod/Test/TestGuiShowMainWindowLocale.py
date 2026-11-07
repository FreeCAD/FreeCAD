# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 The FreeCAD project association <fpa@freecad.org>
# SPDX-FileNotice: Part of the FreeCAD project.

"""GUI regression tests for the numeric locale after FreeCADGui.showMainWindow().

To run tests:
    FreeCAD -t TestGuiShowMainWindowLocale
"""

import os
import subprocess
import sys
import tempfile
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import FreeCAD
from PySide import QtCore

# showMainWindow() only creates its own QApplication in a console process, the GUI running
# this suite already has one. The check therefore runs in a child FreeCADCmd
CHILD_SCRIPT = """
import locale
import os
import FreeCAD
import FreeCADGui
FreeCADGui.showMainWindow()
ctype = locale.setlocale(locale.LC_CTYPE)
numeric = locale.setlocale(locale.LC_NUMERIC)
value = FreeCAD.Units.Quantity("17.2 mm").Value
os.write(1, f"RESULT|{ctype}|{numeric}|{value!r}\\n".encode())
os._exit(0)
"""

# The program embedding FreeCAD may own the QApplication, as src/Tools/embedded/PySide/minimal.py
PRELUDES = {
    "freecad": "",
    "host": "from PySide import QtWidgets\napp = QtWidgets.QApplication([])",
}

# One locale per family of glibc numeric separators, each skipped when it's not installed
LOCALES = (
    "C.UTF-8",
    "en_US.UTF-8",
    "de_DE.UTF-8",  # Decimal comma, point grouping
    "cs_CZ.UTF-8",  # Decimal comma, narrow no-break space grouping
    "de_CH.UTF-8",  # Right single quotation mark grouping
    "en_IN",  # Grouping by three then by two
    "ps_AF",  # Arabic decimal and thousands separators
)


def _console_executable():
    names = ("FreeCADCmd.exe",) if sys.platform == "win32" else ("FreeCADCmd", "freecadcmd")
    folders = (
        Path(QtCore.QCoreApplication.applicationDirPath()),
        Path(FreeCAD.getHomePath()) / "bin",
    )
    for folder in folders:
        for name in names:
            candidate = folder / name
            if candidate.is_file():
                return candidate
    return None


def _same_locale(reported, requested):
    # Glibc may report C.UTF-8 as C.utf8
    def normalise(name):
        return name.lower().replace("-", "")

    return normalise(reported) == normalise(requested)


class TestGuiShowMainWindowLocale(unittest.TestCase):
    def setUp(self):
        self.executable = _console_executable()
        if self.executable is None:
            self.skipTest("FreeCADCmd not found next to the running FreeCAD")

    def run_child(self, start, locale_name):
        """Start the GUI from FreeCADCmd under locale_name, return (ctype, numeric, value, error)"""
        with tempfile.TemporaryDirectory() as user_home:
            script = Path(user_home) / "show_main_window.py"
            script.write_text(PRELUDES[start] + CHILD_SCRIPT, encoding="utf-8")
            env = dict(os.environ)
            env["LANG"] = locale_name
            env["LC_ALL"] = locale_name
            env["FREECAD_USER_HOME"] = user_home
            result = subprocess.run(
                [str(self.executable), str(script)],
                env=env,
                capture_output=True,
                text=True,
                timeout=180,
                check=False,
            )
        for line in result.stdout.splitlines():
            if line.startswith("RESULT|"):
                ctype, numeric, value = line.split("|")[1:]
                return ctype, numeric, float(value), None
        # Failures are raised in the test thread, not in the worker that ran the child
        error = (
            f"FreeCADCmd gave no result under {locale_name} (exit code {result.returncode})\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
        return None, None, None, error

    def check_locales(self, start):
        # Each child is a full GUI start, they run side by side
        with ThreadPoolExecutor(max_workers=min(len(LOCALES), os.cpu_count() or 1)) as pool:
            found = pool.map(lambda name: self.run_child(start, name), LOCALES)
            results = dict(zip(LOCALES, found))
        for locale_name, (ctype, numeric, value, error) in results.items():
            with self.subTest(locale=locale_name):
                if error:
                    self.fail(error)
                if not _same_locale(ctype, locale_name):
                    self.skipTest(f"{locale_name} not available, the child runs under {ctype}")
                self.assertEqual(numeric, "C")
                self.assertAlmostEqual(value, 17.2, places=12)

    def test_numeric_locale_when_show_main_window_creates_the_application(self):
        self.check_locales("freecad")

    def test_numeric_locale_when_the_application_already_exists(self):
        self.check_locales("host")
