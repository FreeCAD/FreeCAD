#! python
# SPDX-License-Identifier: LGPL-2.1-or-later

# (c) 2006 Jürgen Riegel  GPL

import os
import sys
import getopt
import json
from pathlib import Path
import model.generateModel_Module
import model.generateModel_Python
import templates.templateModule
import templates.templateClassPyExport
import templates.templateModulePyExport

Usage = """generate - generates a FreeCAD Module out of an XML or Python model

Usage:
   generate [Optionen] Model.xml/py Model2.xml/py Model3.xml/py ...

Options:
 -h, --help          print this help
 -o, --outputPath    specify the output path if differs from source path
 --depfile           write a Make-style dependency file
 --print-dependencies
                     print source dependencies without generating files
 --print-dependency-map
                     print source dependencies grouped by input as JSON

Generate source code out of an model definition.

Author:
  (c) 2006 Juergen Riegel
  juergen.riegel@web.de
    Licence: GPL

Version:
  0.3
"""


# Globals


def generate_model(filename):
    if filename.endswith(".xml"):
        return model.generateModel_Module.parse(filename)
    elif filename.endswith(".pyi"):
        return model.generateModel_Python.parse(filename)
    raise ValueError("invalid file extension")


def _generated_outputs(generate_model_inst, outputPath):
    output_dir = Path(outputPath)
    if len(generate_model_inst.Module) != 0:
        name = generate_model_inst.Module[0].Name
        return [output_dir / f"{name}.h", output_dir / f"{name}.cpp"]
    if len(generate_model_inst.PythonModule) != 0:
        name = generate_model_inst.PythonModule[0].Name
        return [output_dir / f"{name}ModulePy.h", output_dir / f"{name}ModulePy.cpp"]

    name = generate_model_inst.PythonExport[0].Name
    return [output_dir / f"{name}.h", output_dir / f"{name}.cpp"]


def _depfile_escape(path):
    result = Path(path).resolve().as_posix()
    result = result.replace("\\", "\\\\")
    result = result.replace(" ", "\\ ")
    result = result.replace("#", "\\#")
    result = result.replace("$", "$$")
    result = result.replace(":", "\\:")
    return result


def _unique_paths(paths):
    result = []
    seen = set()
    for path in paths:
        resolved = Path(path).resolve().as_posix()
        if resolved in seen:
            continue
        seen.add(resolved)
        result.append(resolved)
    return result


def write_depfile(depfile, targets, dependencies):
    depfile_path = Path(depfile)
    depfile_path.parent.mkdir(parents=True, exist_ok=True)

    escaped_targets = " ".join(_depfile_escape(target) for target in targets)
    escaped_dependencies = " ".join(
        _depfile_escape(dependency) for dependency in _unique_paths(dependencies)
    )
    depfile_path.write_text(f"{escaped_targets}: {escaped_dependencies}\n", encoding="utf-8")


def source_dependencies(filename):
    generate_model_inst = generate_model(filename)
    return _unique_paths(getattr(generate_model_inst, "SourceDependencies", []))


def source_dependency_map(filenames):
    result = []
    for filename in filenames:
        absolute_filename = Path(filename).resolve().as_posix()
        result.append(
            {
                "source": absolute_filename,
                "dependencies": source_dependencies(absolute_filename),
            }
        )
    return result


def generate(filename, outputPath, depfile=None):
    GenerateModelInst = generate_model(filename)

    if len(GenerateModelInst.Module) != 0:
        Module = templates.templateModule.TemplateModule()
        Module.outputDir = outputPath
        Module.module = GenerateModelInst.Module[0]
        Module.Generate()
        print("Done generating: " + GenerateModelInst.Module[0].Name)
    elif len(GenerateModelInst.PythonModule) != 0:
        Export = templates.templateModulePyExport.TemplateModulePyExport()
        Export.outputDir = Path(outputPath)
        Export.inputDir = Path(filename).parent
        Export.export = GenerateModelInst.PythonModule[0]
        Export.Generate()
        print("Done generating: " + GenerateModelInst.PythonModule[0].Name)
    else:
        Export = templates.templateClassPyExport.TemplateClassPyExport()
        Export.outputDir = outputPath + "/"
        Export.inputDir = os.path.dirname(filename) + "/"
        Export.export = GenerateModelInst.PythonExport[0]
        Export.is_python = filename.endswith(".pyi")
        Export.Generate()
        if Export.is_python:
            Export.Compare()
        print("Done generating: " + GenerateModelInst.PythonExport[0].Name)

    if depfile:
        write_depfile(
            depfile,
            _generated_outputs(GenerateModelInst, outputPath),
            [filename, *getattr(GenerateModelInst, "SourceDependencies", [])],
        )

    return GenerateModelInst


def main():
    verbose = False
    outputPath = ""
    depfile = None
    print_dependencies = False
    print_dependency_map = False

    class generateOutput:
        def write(self, data):
            pass

        def flush(self):  # mandatory for file-like objects
            pass

    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            "hvo:",
            [
                "help",
                "verbose",
                "outputPath=",
                "depfile=",
                "print-dependencies",
                "print-dependency-map",
            ],
        )
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
            outputPath = a
        if o in ("-v", "--verbose"):
            verbose = True
        if o == "--depfile":
            depfile = a
        if o == "--print-dependencies":
            print_dependencies = True
        if o == "--print-dependency-map":
            print_dependency_map = True

    if print_dependencies:
        for i in args:
            filename = os.path.abspath(i)
            for dependency in source_dependencies(filename):
                print(dependency)
        return

    if print_dependency_map:
        filenames = [os.path.abspath(i) for i in args]
        print(json.dumps(source_dependency_map(filenames)))
        return

    if not verbose:
        sys.stdout = generateOutput()

    # running through the files
    if len(args) == 0:
        sys.stderr.write(Usage)
    else:
        for i in args:
            filename = os.path.abspath(i)
            if outputPath == "":
                head, _ = os.path.split(filename)
                generate(filename, head, depfile)
            else:
                generate(filename, outputPath, depfile)


if __name__ == "__main__":
    main()
