# ***************************************************************************
# *   Copyright (c) 2018 - FreeCAD Developers                               *
# *   Author: Bernd Hahnebach <bernd@bimstatik.org>                         *
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

__title__ = "Tools for FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import os
import unittest
import tempfile
import FreeCAD
from os.path import join


def get_fem_test_home_dir():
    return join(FreeCAD.getHomePath(), 'Mod', 'Fem', 'femtest', 'testfiles')


def get_fem_test_tmp_dir():
    temp_dir = join(tempfile.gettempdir(), 'FEM_unittests')
    if not os.path.exists(temp_dir):
        os.makedirs(temp_dir)
    return(temp_dir)


def get_unit_test_tmp_dir(temp_dir, unittestdir):
    testdir = join(temp_dir, unittestdir)
    if not os.path.exists(testdir):
        os.makedirs(testdir)
    return testdir


def fcc_print(message):
    FreeCAD.Console.PrintMessage('{} \n'.format(message))


def get_defmake_count(fem_vtk_post=True):
    '''
    count the def make in module ObjectsFem
    could also be done in bash with
    grep -c  "def make" src/Mod/Fem/ObjectsFem.py
    '''
    name_modfile = join(FreeCAD.getHomePath(), 'Mod', 'Fem', 'ObjectsFem.py')
    modfile = open(name_modfile, 'r')
    lines_modefile = modfile.readlines()
    modfile.close()
    lines_defmake = [l for l in lines_modefile if l.startswith('def make')]
    if not fem_vtk_post:  # FEM VTK post processing is disabled, we are not able to create VTK post objects
        new_lines = []
        for l in lines_defmake:
            if "PostVtk" not in l:
                new_lines.append(l)
        lines_defmake = new_lines
    return len(lines_defmake)


def get_fem_test_defs(inout='out'):
    test_path = join(FreeCAD.getHomePath(), 'Mod', 'Fem', 'femtest')
    collected_test_modules = []
    collected_test_methods = []
    for tfile in sorted(os.listdir(test_path)):
        if tfile.startswith("test") and tfile.endswith(".py"):
            collected_test_modules.append(join(test_path, tfile))
    for f in collected_test_modules:
        tfile = open(f, 'r')
        module_name = os.path.splitext(os.path.basename(f))[0]
        class_name = ''
        for ln in tfile:
            ln = ln.lstrip()
            ln = ln.rstrip()
            if ln.startswith('class '):
                ln = ln.lstrip('class ')
                ln = ln.split('(')[0]
                class_name = ln
            if ln.startswith('def test'):
                ln = ln.lstrip('def ')
                ln = ln.split('(')[0]
                collected_test_methods.append('femtest.{}.{}.{}'.format(module_name, class_name, ln))
        tfile.close()
    print('')
    for m in collected_test_methods:
        run_outside_fc = './bin/FreeCADCmd --run-test "{}"'.format(m)
        run_inside_fc = 'unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("{}"))'.format(m)
        if inout == 'in':
            print('\nimport unittest')
            print(run_inside_fc)
        else:
            print(run_outside_fc)


def compare_inp_files(file_name1, file_name2):
    file1 = open(file_name1, 'r')
    f1 = file1.readlines()
    file1.close()
    # l.startswith('17671.0,1') is a temporary workaround for python3 problem with 1DFlow input
    # TODO as soon as the 1DFlow result reading is fixed, this should be triggered in the 1DFlow unit test
    lf1 = [l for l in f1 if not (l.startswith('**   written ') or l.startswith('**   file ') or l.startswith('17671.0,1'))]
    lf1 = force_unix_line_ends(lf1)
    file2 = open(file_name2, 'r')
    f2 = file2.readlines()
    file2.close()
    # TODO see comment on file1
    lf2 = [l for l in f2 if not (l.startswith('**   written ') or l.startswith('**   file ') or l.startswith('17671.0,1'))]
    lf2 = force_unix_line_ends(lf2)
    import difflib
    diff = difflib.unified_diff(lf1, lf2, n=0)
    result = ''
    for l in diff:
        result += l
    if result:
        result = "Comparing {} to {} failed!\n".format(file_name1, file_name2) + result
    return result


def compare_files(file_name1, file_name2):
    file1 = open(file_name1, 'r')
    f1 = file1.readlines()
    file1.close()
    # workaround to compare geos of elmer test and temporary file path
    # (not only names change, path changes with operating system)
    lf1 = [l for l in f1 if not (l.startswith('Merge "') or l.startswith('Save "') or l.startswith('// '))]
    lf1 = force_unix_line_ends(lf1)
    file2 = open(file_name2, 'r')
    f2 = file2.readlines()
    file2.close()
    lf2 = [l for l in f2 if not (l.startswith('Merge "') or l.startswith('Save "') or l.startswith('// '))]
    lf2 = force_unix_line_ends(lf2)
    import difflib
    diff = difflib.unified_diff(lf1, lf2, n=0)
    result = ''
    for l in diff:
        result += l
    if result:
        result = "Comparing {} to {} failed!\n".format(file_name1, file_name2) + result
    return result


def compare_stats(fea, stat_file=None, loc_stat_types=None, res_obj_name=None):
    import femresult.resulttools as resulttools
    stat_types = ["U1", "U2", "U3", "Uabs", "Sabs", "MaxPrin", "MidPrin", "MinPrin", "MaxShear", "Peeq", "Temp", "MFlow", "NPress"]
    if not loc_stat_types:
        loc_stat_types = stat_types
    if stat_file:
        sf = open(stat_file, 'r')
        sf_content = []
        for l in sf.readlines():
            for st in loc_stat_types:
                if l.startswith(st):
                    sf_content.append(l)
        sf.close()
        sf_content = force_unix_line_ends(sf_content)
    stats = []
    for s in loc_stat_types:
        if res_obj_name:
            statval = resulttools.get_stats(FreeCAD.ActiveDocument.getObject(res_obj_name), s)
        else:
            print('No result object name given')
            return False
        stats.append("{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n".format(s, statval[0], statval[1], statval[2]))
    if sf_content != stats:
        fcc_print("Expected stats from {}".format(stat_file))
        fcc_print(sf_content)
        fcc_print("Stats read from {}.frd file".format(fea.base_name))
        fcc_print(stats)
        return True
    return False


def force_unix_line_ends(line_list):
    new_line_list = []
    for ln in line_list:
        if ln.endswith("\r\n"):
            ln = ln[:-2] + '\n'
        new_line_list.append(ln)
    return new_line_list


def collect_python_modules(femsubdir=None):
    if not femsubdir:
        pydir = join(FreeCAD.ConfigGet("AppHomePath"), 'Mod', 'Fem')
    else:
        pydir = join(FreeCAD.ConfigGet("AppHomePath"), 'Mod', 'Fem', femsubdir)
    collected_modules = []
    fcc_print(pydir)
    for pyfile in sorted(os.listdir(pydir)):
        if pyfile.endswith(".py") and not pyfile.startswith('Init'):
            if not femsubdir:
                collected_modules.append(os.path.splitext(os.path.basename(pyfile))[0])
            else:
                collected_modules.append(femsubdir.replace('/', '.') + '.' + os.path.splitext(os.path.basename(pyfile))[0])
    return collected_modules


# open all files
def all_test_files():
    cube_frequency()
    cube_static()
    Flow1D_thermomech()
    multimat()
    spine_thermomech()


# run the specific test case of the file, open the file in FreeCAD GUI and return the doc identifier
def cube_frequency():
    testname = "femtest.testccxtools.TestCcxTools.test_3_freq_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    return FreeCAD.open(join(get_fem_test_tmp_dir(), 'FEM_ccx_frequency', 'cube_frequency.FCStd'))


def cube_static():
    testname = "femtest.testccxtools.TestCcxTools.test_1_static_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    return FreeCAD.open(join(get_fem_test_tmp_dir(), 'FEM_ccx_static', 'cube_static.FCStd'))


def Flow1D_thermomech():
    testname = "femtest.testccxtools.TestCcxTools.test_5_Flow1D_thermomech_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    return FreeCAD.open(join(get_fem_test_tmp_dir(), 'FEM_ccx_Flow1D_thermomech', 'Flow1D_thermomech.FCStd'))


def multimat():
    testname = "femtest.testccxtools.TestCcxTools.test_2_static_multiple_material"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    return FreeCAD.open(join(get_fem_test_tmp_dir(), 'FEM_ccx_multimat', 'multimat.FCStd'))


def spine_thermomech():
    testname = "femtest.testccxtools.TestCcxTools.test_4_thermomech_analysis"
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(testname))
    return FreeCAD.open(join(get_fem_test_tmp_dir(), 'FEM_ccx_thermomech', 'spine_thermomech.FCStd'))
