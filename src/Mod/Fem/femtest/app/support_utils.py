# ***************************************************************************
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "Tools for FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import math
import os
import sys
import tempfile
import unittest
from typing import Union, List, Iterator

import FreeCAD

from os.path import join


def get_fem_test_home_dir():
    return join(FreeCAD.getHomePath(), "Mod", "Fem", "femtest", "data")


def get_fem_test_tmp_dir(dirname=None):
    from uuid import uuid4

    _unique_id = str(uuid4())[-12:]
    # print(_unique_id)
    if dirname is None:
        temp_dir = join(tempfile.gettempdir(), "FEM_unittests", _unique_id)
    else:
        temp_dir = join(tempfile.gettempdir(), "FEM_unittests", dirname + "_" + _unique_id)
    if not os.path.exists(temp_dir):
        os.makedirs(temp_dir)
    return temp_dir


def get_unit_test_tmp_dir(temp_dir, unittestdir):
    testdir = join(temp_dir, unittestdir)
    if not os.path.exists(testdir):
        os.makedirs(testdir)
    return testdir


def fcc_print(message):
    FreeCAD.Console.PrintMessage(f"{message} \n")


def get_namefromdef(strdel="", stradd=""):
    # https://code.activestate.com/recipes/66062-determining-current-function-name/
    return (sys._getframe(1).f_code.co_name).replace(strdel, stradd)


def get_defmake_count(fem_vtk_post=True):
    """
    count the def make in module ObjectsFem
    could also be done in bash with
    grep -c  "def make" src/Mod/Fem/ObjectsFem.py
    """
    name_modfile = join(FreeCAD.getHomePath(), "Mod", "Fem", "ObjectsFem.py")
    modfile = open(name_modfile)
    lines_modefile = modfile.readlines()
    modfile.close()
    lines_defmake = [li for li in lines_modefile if li.startswith("def make")]
    if not fem_vtk_post:
        # FEM VTK post processing is disabled
        # we are not able to create VTK post objects
        new_lines = []
        for li in lines_defmake:
            if "Post" not in li:
                new_lines.append(li)
        lines_defmake = new_lines
    return len(lines_defmake)


def get_fem_test_defs():

    test_path = join(FreeCAD.getHomePath(), "Mod", "Fem", "femtest", "app")
    print(f"Modules, classes, methods taken from: {test_path}")

    collected_test_module_paths = []
    for tfile in sorted(os.listdir(test_path)):
        if tfile.startswith("test") and tfile.endswith(".py"):
            collected_test_module_paths.append(join(test_path, tfile))

    collected_test_modules = []
    collected_test_classes = []
    collected_test_methods = []
    for f in collected_test_module_paths:
        module_name = os.path.splitext(os.path.basename(f))[0]
        module_path = f"femtest.app.{module_name}"
        if module_path not in collected_test_modules:
            collected_test_modules.append(module_path)
        class_name = ""
        tfile = open(f)
        for ln in tfile:
            ln = ln.lstrip()
            ln = ln.rstrip()
            if ln.startswith("class "):
                ln = ln.lstrip("class ")
                ln = ln.split("(")[0]
                class_name = ln
                class_path = f"femtest.app.{module_name}.{class_name}"
                if class_path not in collected_test_classes:
                    collected_test_classes.append(class_path)
            if ln.startswith("def test"):
                ln = ln.lstrip("def ")
                ln = ln.split("(")[0]
                if ln == "test_00print":
                    continue
                method_path = f"femtest.app.{module_name}.{class_name}.{ln}"
                collected_test_methods.append(method_path)
        tfile.close()

    # write to file
    file_path = join(tempfile.gettempdir(), "test_commands.sh")
    cf = open(file_path, "w")
    cf.write("# created by Python\n")
    cf.write("'''\n")
    cf.write("from femtest.app.support_utils import get_fem_test_defs\n")
    cf.write("get_fem_test_defs()\n")
    cf.write("\n")
    cf.write("\n")
    cf.write("# all FEM App tests\n")
    cf.write("make -j 4 && ./bin/FreeCAD --run-test 'TestFemApp'\n")
    cf.write("\n")
    cf.write("make -j 4 && ./bin/FreeCADCmd --run-test 'TestFemApp'\n")
    cf.write("\n")
    cf.write("\n")
    cf.write("'''\n")
    cf.write("\n")
    cf.write("# modules\n")
    for m in collected_test_modules:
        cf.write(f"make -j 4 && ./bin/FreeCADCmd -t {m}\n")
    cf.write("\n")
    cf.write("\n")
    cf.write("# classes\n")
    for m in collected_test_classes:
        cf.write(f"make -j 4 && ./bin/FreeCADCmd -t {m}\n")
    cf.write("\n")
    cf.write("\n")
    cf.write("# methods\n")
    for m in collected_test_methods:
        cf.write(f"make -j 4 && ./bin/FreeCADCmd -t {m}\n")
    cf.write("\n")
    cf.write("\n")
    cf.write("# methods in FreeCAD\n")
    for m in collected_test_methods:
        cf.write(
            "\nimport unittest\n"
            "unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(\n"
            "    '{}'\n"
            "))\n".format(m)
        )
    cf.close()
    print(f"The file was saved in:{file_path}")


def try_converting_to_float(line: str) -> Union[None, List[float]]:
    """Does its best to split a line and convert its elements to float

    Has 3 strategies of splitting:
        * by comma - mainly in CalculiX .inp files
        * by space - other solvers
        * by space and ignoring the 1st word - other solvers
            If there was only 1 word, the line will fail, no compromises

    Single characters always pass

    :param line: line to split and convert to floats
    :return: None if conversion failed, else list of floats
    """
    strategies = [
        lambda _line: _line[1:].split(","),
        lambda _line: _line[1:].split(),
        lambda _line: (lambda split_line: len(split_line) > 1 and split_line or "fail")(
            _line[1:].split()[1:]
        ),
    ]
    for strategy in strategies:
        try:
            return list(map(float, strategy(line)))
        except ValueError:
            pass
    return None


def are_floats_equal(
    orig_floats: Union[None, List[float]], floats_to_compare: Union[None, List[float]]
) -> bool:
    """Check if floats in lists are equal with some tolerance

    :param orig_floats: list of floats - left operands of comparison
    :param floats_to_compare: list of floats - right operands of comparison
    :return: True if both lists are equal with some tolerance element-wise, else False
    """
    if any(floats is None for floats in (orig_floats, floats_to_compare)):
        return False
    for orig_float, float_to_compare in zip(orig_floats, floats_to_compare):
        if not math.isclose(orig_float, float_to_compare, abs_tol=1e-10):
            return False
    return True


def parse_diff(diff_lines: Iterator[str]) -> List[str]:
    """Parses lines from `united_diff`

    Tries to split a line and convert its contents to float, to
    compare float numbers with some tolerance

    Recognizes blocks of changes, that start with `@@` and end either at EOF or
    at the beginning of another block. Only bad lines are added to block.
    All lines, which didn't pass the element-wise check, are bad lines.
    :param diff_lines: lines produced by `united_diff`
    :return: list of bad lines with respect to their block of change
    """
    bad_lines = []
    changes_block = []
    while True:
        try:
            first = next(diff_lines)
            if first.startswith("@@"):
                if len(changes_block) > 1:
                    bad_lines.extend(changes_block)
                changes_block = [first]
                continue
            second = next(diff_lines)
        except StopIteration:
            break
        if first.startswith("---"):
            continue
        if not are_floats_equal(*map(try_converting_to_float, (first, second))):
            changes_block.extend((first, second))
    # check if we have any remaining block
    if len(changes_block) > 1:
        bad_lines.extend(changes_block)
    return bad_lines


def compare_inp_files(file_name1, file_name2):
    file1 = open(file_name1)
    f1 = file1.readlines()
    file1.close()
    # l.startswith("17671.0,1") is a temporary workaround
    # for python3 problem with 1DFlow input
    # TODO as soon as the 1DFlow result reading is fixed
    # this should be triggered in the 1DFlow unit test
    lf1 = [
        li
        for li in f1
        if not (
            li.startswith("**   written ")
            or li.startswith("**   file ")
            or li.startswith("17671.0,1")
        )
    ]
    lf1 = force_unix_line_ends(lf1)
    file2 = open(file_name2)
    f2 = file2.readlines()
    file2.close()
    # TODO see comment on file1
    lf2 = [
        li
        for li in f2
        if not (
            li.startswith("**   written ")
            or li.startswith("**   file ")
            or li.startswith("17671.0,1")
        )
    ]
    lf2 = force_unix_line_ends(lf2)
    import difflib

    diff_lines = difflib.unified_diff(lf1, lf2, n=0)

    bad_lines = parse_diff(diff_lines)
    if bad_lines:
        return f"Comparing {file_name1} to {file_name2} failed!\n{''.join(bad_lines)}"


def compare_files(file_name1, file_name2):
    file1 = open(file_name1)
    f1 = file1.readlines()
    file1.close()

    # TODO: add support for variable values in the reference file
    # instead of using this workaround

    # workaround to compare geos of elmer test and temporary file path
    # (not only names change, path changes with operating system)
    lf1 = [
        li
        for li in f1
        if not (
            li.startswith('Merge "')
            or li.startswith('Save "')
            or li.startswith("// ")
            or li.startswith("General.NumThreads")
        )
    ]
    lf1 = force_unix_line_ends(lf1)
    file2 = open(file_name2)
    f2 = file2.readlines()
    file2.close()
    lf2 = [
        li
        for li in f2
        if not (
            li.startswith('Merge "')
            or li.startswith('Save "')
            or li.startswith("// ")
            or li.startswith("General.NumThreads")
        )
    ]
    lf2 = force_unix_line_ends(lf2)
    import difflib

    diff = difflib.unified_diff(lf1, lf2, n=0)
    result = ""
    for li in diff:
        result += li
    if result:
        result = f"Comparing {file_name1} to {file_name2} failed!\n" + result
    return result


def compare_stats(fea, stat_file, res_obj_name, loc_stat_types=None):
    import femresult.resulttools as resulttools

    # get the stat types which should be compared
    stat_types = [
        "U1",
        "U2",
        "U3",
        "Uabs",
        "Sabs",
        "MaxPrin",
        "MidPrin",
        "MinPrin",
        "MaxShear",
        "Peeq",
        "Temp",
        "MFlow",
        "NPress",
    ]
    if not loc_stat_types:
        loc_stat_types = stat_types

    # get stats from result obj which should be compared
    obj = fea.analysis.Document.getObject(res_obj_name)
    # fcc_print(obj)
    if obj:
        # fcc_print(obj.Name)
        stats = []
        for s in loc_stat_types:
            statval = resulttools.get_stats(obj, s)
            stats.append(f"{s}: ({statval[0]:.10f}, {statval[1]:.10f})\n")
    else:
        fcc_print(f"Result object not found. Name: {res_obj_name}")
        return True

    # get stats to compare with, the expected ones
    sf = open(stat_file)
    sf_content = []
    for li in sf.readlines():
        for st in loc_stat_types:
            if li.startswith(st):
                sf_content.append(li)
    sf.close()
    sf_content = force_unix_line_ends(sf_content)
    if sf_content == []:
        return True

    # compare stats
    if stats != sf_content:
        fcc_print(f"Stats read from {fea.base_name}.frd file")
        fcc_print("!=")
        fcc_print(f"Expected stats from {stat_file}")
        for i in range(len(stats)):
            if stats[i] != sf_content[i]:
                fcc_print(f"{stats[i].rstrip()} != {sf_content[i].rstrip()}")
        return True

    return False


def force_unix_line_ends(line_list):
    new_line_list = []
    for ln in line_list:
        if ln.endswith("\r\n"):
            ln = ln[:-2] + "\n"
        new_line_list.append(ln)
    return new_line_list


def collect_python_modules(femsubdir=None):
    if not femsubdir:
        pydir = join(FreeCAD.ConfigGet("AppHomePath"), "Mod", "Fem")
    else:
        pydir = join(FreeCAD.ConfigGet("AppHomePath"), "Mod", "Fem", femsubdir)
    collected_modules = []
    fcc_print(pydir)
    for pyfile in sorted(os.listdir(pydir)):
        if pyfile.endswith(".py") and not pyfile.startswith("Init"):
            if not femsubdir:
                collected_modules.append(os.path.splitext(os.path.basename(pyfile))[0])
            else:
                collected_modules.append(
                    femsubdir.replace("/", ".")
                    + "."
                    + os.path.splitext(os.path.basename(pyfile))[0]
                )
    return collected_modules


def all_test_files():
    # open all files
    cube_frequency()
    cube_static()
    Flow1D_thermomech()
    multimat()
    spine_thermomech()


# run the specific test case of the file
# open the file in FreeCAD GUI and return the doc identifier
def cube_frequency():
    testname = "femtest.testccxtools.TestCcxTools.test_3_freq_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    doc = FreeCAD.open(join(get_fem_test_tmp_dir(), "FEM_ccx_frequency", "cube_frequency.FCStd"))
    return doc


def cube_static():
    testname = "femtest.testccxtools.TestCcxTools.test_1_static_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    doc = FreeCAD.open(join(get_fem_test_tmp_dir(), "FEM_ccx_static", "cube_static.FCStd"))
    return doc


def Flow1D_thermomech():
    testname = "femtest.testccxtools.TestCcxTools.test_5_Flow1D_thermomech_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    doc = FreeCAD.open(
        join(get_fem_test_tmp_dir(), "FEM_ccx_Flow1D_thermomech", "Flow1D_thermomech.FCStd")
    )
    return doc


def multimat():
    testname = "femtest.testccxtools.TestCcxTools.test_2_static_multiple_material"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    doc = FreeCAD.open(join(get_fem_test_tmp_dir(), "FEM_ccx_multimat", "multimat.FCStd"))
    return doc


def spine_thermomech():
    testname = "femtest.testccxtools.TestCcxTools.test_4_thermomech_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    doc = FreeCAD.open(join(get_fem_test_tmp_dir(), "FEM_ccx_thermomech", "spine_thermomech.FCStd"))
    return doc
