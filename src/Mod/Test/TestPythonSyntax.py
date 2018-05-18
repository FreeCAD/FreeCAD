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
            if (not fn in whitelist) and os.path.splitext(fn)[1] == '.py':
                with open(os.path.join(sub_dir, fn), encoding='utf-8') as py_file:
                    try:
                        ast.parse(py_file.read())
                    except SyntaxError:
                        log.append(os.path.join(sub_dir, fn))
    message = "\n\n" + "#" * 30 + "\n"
    message += "{} python files are not parseable:\n\n".format(len(log))
    for i in log:
        message += i + "\n"
    message += "#" * 30 + "\n\n"
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
        test_python_syntax(App.getHomePath(), self.whitelist)
