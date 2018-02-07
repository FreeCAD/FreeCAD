# ***************************************************************************
# *   Copyright (c) 2018 - FreeCAD Developers                               *
# *   Copyright (c) 2018 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


import Fem
from femtools import ccxtools
import femresult.resulttools as resulttools
import FreeCAD
import ObjectsFem
import tempfile
import unittest
import os


mesh_name = 'Mesh'
stat_types = ["U1", "U2", "U3", "Uabs", "Sabs", "MaxPrin", "MidPrin", "MinPrin", "MaxShear", "Peeq", "Temp", "MFlow", "NPress"]

home_path = FreeCAD.getHomePath()
temp_dir = tempfile.gettempdir() + '/FEM_unittests/'
if not os.path.exists(temp_dir):
    os.makedirs(temp_dir)
test_file_dir = home_path + 'Mod/Fem/femtest/testfiles/ccx/'
test_file_dir_elmer = home_path + 'Mod/Fem/femtest/testfiles/elmer/'

# define some locations fot the analysis tests
# since they are also used in the helper def which create results they should stay global for the module
static_base_name = 'cube_static'
static_analysis_dir = temp_dir + 'FEM_ccx_static/'
static_save_fc_file = static_analysis_dir + static_base_name + '.fcstd'
static_analysis_inp_file = test_file_dir + static_base_name + '.inp'
static_expected_values = test_file_dir + "cube_static_expected_values"

frequency_base_name = 'cube_frequency'
frequency_analysis_dir = temp_dir + 'FEM_ccx_frequency/'
frequency_save_fc_file = frequency_analysis_dir + frequency_base_name + '.fcstd'
frequency_analysis_inp_file = test_file_dir + frequency_base_name + '.inp'
frequency_expected_values = test_file_dir + "cube_frequency_expected_values"

thermomech_base_name = 'spine_thermomech'
thermomech_analysis_dir = temp_dir + 'FEM_ccx_thermomech/'
thermomech_save_fc_file = thermomech_analysis_dir + thermomech_base_name + '.fcstd'
thermomech_analysis_inp_file = test_file_dir + thermomech_base_name + '.inp'
thermomech_expected_values = test_file_dir + "spine_thermomech_expected_values"

Flow1D_thermomech_base_name = 'Flow1D_thermomech'
Flow1D_thermomech_analysis_dir = temp_dir + 'FEM_ccx_Flow1D_thermomech/'
Flow1D_thermomech_save_fc_file = Flow1D_thermomech_analysis_dir + Flow1D_thermomech_base_name + '.fcstd'
Flow1D_thermomech_analysis_inp_file = test_file_dir + Flow1D_thermomech_base_name + '.inp'
Flow1D_thermomech_expected_values = test_file_dir + "Flow1D_thermomech_expected_values"


def fcc_print(message):
    FreeCAD.Console.PrintMessage('{} \n'.format(message))


def get_defmake_count():
    '''
    count the def make in module ObjectsFem
    could also be done in bash with
    grep -c  "def make" src/Mod/Fem/ObjectsFem.py
    '''
    name_modfile = home_path + 'Mod/Fem/ObjectsFem.py'
    modfile = open(name_modfile, 'r')
    lines_modefile = modfile.readlines()
    modfile.close()
    lines_defmake = [l for l in lines_modefile if l.startswith('def make')]
    return len(lines_defmake)


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
    # workaraound for compare geos of elmer test and temporary file path (not only names change, path changes with operating system)
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
    for l in line_list:
        if l.endswith("\r\n"):
            l = l[:-2] + '\n'
        new_line_list.append(l)
    return new_line_list


def collect_python_modules(femsubdir=None):
    if not femsubdir:
        pydir = FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/'
    else:
        pydir = FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/' + femsubdir + '/'
    collected_modules = []
    fcc_print(pydir)
    for pyfile in sorted(os.listdir(pydir)):
        if pyfile.endswith(".py") and not pyfile.startswith('Init'):
            if not femsubdir:
                collected_modules.append(os.path.splitext(os.path.basename(pyfile))[0])
            else:
                collected_modules.append(femsubdir.replace('/', '.') + '.' + os.path.splitext(os.path.basename(pyfile))[0])
    return collected_modules


def runTestFem():
    '''run FEM unit test
    for more information on how to run a specific test class or a test def see comment at file end
    '''
    import Test
    import sys
    current_module = sys.modules[__name__]
    Test.runTestsFromModule(current_module)


'''
for more information on how to run a specific test class or a test def see
file src/Mod/Test/__init__
https://forum.freecadweb.org/viewtopic.php?f=10&t=22190#p175546

import unittest
mytest = unittest.TestLoader().loadTestsFromName("TestFem.FemTest.test_pyimport_all_FEM_modules")
unittest.TextTestRunner().run(mytest)

import Test, TestFem
Test.runTestsFromModule(TestFem)

'''
