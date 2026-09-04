# SPDX-License-Identifier: LGPL-2.1-or-later

from pathlib import Path
import sys
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.discovery import module_state_for_source


class ModuleStateTest(unittest.TestCase):
    def test_discovers_module_creation_and_relationships(self) -> None:
        source = """
PyObject* imported = PyImport_ImportModule("Part");
PyObject* added = PyImport_AddModule("FreeCADGui");
PyObject* initialized = Base::initModule();
Py::Object wrapper(initialized);
PyObject* child(wrapper.getAttr("Units").ptr());
PyModule_AddObject(initialized, "Units", child);
"""

        state = module_state_for_source(source)

        self.assertEqual(state.variables["imported"], "Part")
        self.assertEqual(state.variables["added"], "FreeCADGui")
        self.assertEqual(state.variables["initialized"], "Base")
        self.assertEqual(state.variables["child"], "Base.Units")


if __name__ == "__main__":
    unittest.main()
