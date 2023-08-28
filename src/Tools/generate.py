#! python
# -*- coding: utf-8 -*-
# (c) 2006 Jürgen Riegel  GPL

import os, sys, getopt
import generateBase.generateModel_Module
import generateTemplates.templateModule
import generateTemplates.templateClassPyExport

Usage = """generate - generates a FreeCAD Module out of an XML model

Usage:
   generate [Optionen] Model.xml Model2.xml Model3.xml ...

Options:
 -h, --help          print this help
 -o, --outputPath    specify the output path if differs from source path

Generate source code out of an model definition.

Author:
  (c) 2006 Juergen Riegel
  juergen.riegel@web.de
    Licence: GPL

Version:
  0.2
"""


# Globals


def generate(filename, path):
    # load model
    GenerateModelInst = generateBase.generateModel_Module.parse(filename)

    if len(GenerateModelInst.Module) != 0:
        Module = generateTemplates.templateModule.TemplateModule()
        Module.path = path
        Module.module = GenerateModelInst.Module[0]
        Module.Generate()
        print("Done generating: " + GenerateModelInst.Module[0].Name)
    else:
        Export = generateTemplates.templateClassPyExport.TemplateClassPyExport()
        Export.path = path + "/"
        Export.dirname = os.path.dirname(filename) + "/"
        Export.export = GenerateModelInst.PythonExport[0]
        Export.Generate()
        print("Done generating: " + GenerateModelInst.PythonExport[0].Name)


def main():
    defaultPath = ""

    class generateOutput:
        def write(self, data):
            pass

        def flush(self):  # mandatory for file-like objects
            pass

    sys.stdout = generateOutput()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "ho:", ["help", "outputPath="])
    except getopt.GetoptError:
        # print help information and exit:
        sys.stderr.write(Usage)
        sys.exit(2)

    # checking on the options
    for o, a in opts:
        if o in ("-h", "--help"):
            sys.stderr.write(Usage)
            sys.exit()
        if o in ("-o", "--outputPath"):
            defaultPath = a

    # running through the files
    if len(args) == 0:
        sys.stderr.write(Usage)
    else:
        for i in args:
            filename = os.path.abspath(i)
            if defaultPath == "":
                head, tail = os.path.split(filename)
                print(head, tail)
                generate(filename, head)
            else:
                generate(filename, defaultPath)


if __name__ == "__main__":
    main()
