import sys
import os
import ast
import unittest
import FreeCAD as App


def test_python_syntax(rootdir, whitelist=None):
    whitelist = whitelist or []
    log = []
    for sub_dir, dirs, files in os.walk(rootdir):
        for fn in files:
            kargs = {}
            if sys.version_info.major >= 3:
                kargs["encoding"] = "utf-8"
            if (not fn in whitelist) and os.path.splitext(fn)[1] == '.py':
                with open(os.path.join(sub_dir, fn), **kargs) as py_file:
                    try:
                        ast.parse(py_file.read())
                    except SyntaxError as err:
                        log.append(str(err).replace("<unknown>", os.path.join(sub_dir, fn)))
    message = "\n\n" + "#" * 30 + "\n"
    message += "{} python files are not parseable:\n\n".format(len(log))
    for i, m in enumerate(log):
        message += str(i + 1) + " " + m + "\n"
    if log:
        raise RuntimeError("there are some files not parse-able with the used python-interpreter" + message)
    else:
        return

class PythonSyntaxTestCase(unittest.TestCase):
    """
    Test Case to test python syntax of all python files in FreeCAD
    """
    def setUp(self):
        self.whitelist = []
        self.whitelist += ["automotive_design.py"]
        self.whitelist += ["ifc2x3.py"]
        self.whitelist += ["ifc4.py"]
        

    def testAll(self):
        mod_dir = os.path.join(App.getHomePath(), "Mod")
        test_python_syntax(mod_dir, self.whitelist)
