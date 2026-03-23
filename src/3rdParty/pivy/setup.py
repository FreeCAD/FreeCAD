#!/usr/bin/env python

###
# Copyright (c) 2002-2009 Systems in Motion
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

###
# Pivy distutils setup script.
#

"""Pivy is a Coin binding for Python. Coin is a high-level 3D graphics
library with a C++ Application Programming Interface. Coin uses
scene-graph data structures to render real-time graphics suitable for
mostly all kinds of scientific and engineering visualization
applications.
"""
from __future__ import print_function

###
# Setup file for the Pivy distribution.
#
import glob
import os
import shutil
import subprocess
import sys

from distutils.command.build import build
from distutils.command.clean import clean
from distutils.command.install import install
from distutils.core import setup
from distutils.extension import Extension
from distutils import sysconfig

# if we are on a Gentoo box salute the chap and output stuff in nice colors
# Gentoo is Python friendly, so be especially friendly to them! ;)
try:
    from portage.output import green, blue, turquoise, red, yellow
    print(red("Oooh, it's a Gentoo! Nice nice! tuhtah salutes you! :)"))
except:
    try:
        from colorama import Fore, Style

        def red(text): return Fore.RED + text + Style.RESET_ALL

        def green(text): return Fore.GREEN + text + Style.RESET_ALL

        def blue(text): return Fore.BLUE + text + Style.RESET_ALL

        def turquoise(text): return Fore.CYAN + text + Style.RESET_ALL

        def yellow(text): return Fore.YELLOW + text + Style.RESET_ALL
    except:
        def red(text): return text

        def green(text): return text

        def blue(text): return text

        def turquoise(text): return text

        def yellow(text): return text

PIVY_CLASSIFIERS = """\
Development Status :: 5 - Production/Stable
Intended Audience :: Developers
License :: OSI Approved :: BSD License
Operating System :: MacOS :: MacOS X
Operating System :: Microsoft :: Windows
Operating System :: Unix
Programming Language :: Python
Topic :: Multimedia :: Graphics
Topic :: Multimedia :: Graphics :: 3D Modeling
Topic :: Multimedia :: Graphics :: 3D Rendering
Topic :: Software Development :: Libraries :: Python Modules
"""

__dir_name__ = os.path.dirname(__file__)
pivy_dir = os.path.join(__dir_name__, "pivy")
sys.path.append(pivy_dir)
import pivy_meta
PIVY_VERSION = pivy_meta.__version__
sys.path.pop(-1)

class pivy_build(build):
    PIVY_SNAKES = r"""
                            _____
                        .-'`     '.                                     
                     __/  __       \                                   
                    /  \ /  \       |    ___                          
                   | /`\| /`\|      | .-'  /^\/^\                   
                   | \(/| \(/|      |/     |) |)|                     
                  .-\__/ \__/       |      \_/\_/__..._             
          _...---'-.                /   _              '.               
         /,      ,             \   '|  `\                \           
        | ))     ))           /`|   \    `.       /)  /) |             
        | `      `          .'       |     `-._         /               
        \                 .'         |     ,_  `--....-'               
         `.           __.' ,         |     / /`'''`                     
           `'-.____.-' /  /,         |    / /                           
               `. `-.-` .'  \        /   / |                           
                 `-.__.'|    \      |   |  |-.                         
                    _.._|     |     /   |  |  `'.                       
              .-''``    |     |     |   /  |     `-.                    
           .'`         /      /     /  |   |        '.                  
         /`           /      /     |   /   |\         \               
        /            |      |      |   |   /\          |               
       ||            |      /      |   /     '.        |                
       |\            \      |      /   |       '.      /              
       \ `.           '.    /      |    \        '---'/               
        \  '.           `-./        \    '.          /                
         '.  `'.            `-._     '.__  '-._____.'--'''''--.         
           '-.  `'--._          `.__     `';----`              \       
              `-.     `-.          `.''```                     ;        
                 `'-..,_ `-.         `'-.                     /         
                        '.  '.           '.                 .'          


                            ~~~ HISSSSSSSSSS ~~~
                           Welcome to Pivy %s!
                 Building Pivy has never been so much fun!

    """ % PIVY_VERSION

    pivy_header_include = """\
#ifdef __PIVY__
%%include %s
#endif

"""

    SWIG = ((sys.platform == "win32" and "swig.exe") or "swig")

    SWIG_SUPPRESS_WARNINGS = "-w302,306,307,312,314,325,361,362,467,389,503,509,510"
    SWIG_PARAMS = "-c++ -python -includeall -modern -D__PIVY__ " + \
                  "-I. -Ifake_headers -I\"%s\" %s -o %s_wrap.cpp " + \
                  "interfaces" + os.sep + "%s.i"
    if sys.version_info.major >= 3:
        SWIG_PARAMS = '-py3 ' + SWIG_PARAMS


    # TODO: add command line arguments to tell distutils which library should be build
    SOGUI = ['soqt']  # stop supporting soxt, sogtk, and sowin to simplify the setup
    coin_interface = 'coin'
    soqt_interface = 'soqt'

    if sys.version_info.major < 3:
        coin_interface = 'coin2'
        soqt_interface = 'soqt2'

    MODULES = {
        'coin': ['_coin', 'COIN', 'pivy.', coin_interface],
        'soqt': ['gui._soqt', 'SOQT', 'pivy.gui.', soqt_interface]
    }

    SUPPORTED_SWIG_VERSIONS = ['3.0.8', '3.0.10', '3.0.12', '4.0.0']
    SWIG_VERSION = ""
    SWIG_COND_SYMBOLS = []
    CXX_INCS = "-Iinterfaces "
    CXX_LIBS = ""

    ext_modules = []
    py_modules = ['pivy.quarter.ContextMenu',
                  'pivy.quarter.ImageReader',
                  'pivy.quarter.QuarterWidget',
                  'pivy.quarter.SensorManager',
                  'pivy.quarter.SignalThread',
                  'pivy.quarter.devices.DeviceHandler',
                  'pivy.quarter.devices.DeviceManager',
                  'pivy.quarter.devices.KeyboardHandler',
                  'pivy.quarter.devices.MouseHandler',
                  'pivy.quarter.eventhandlers.DragDropHandler',
                  'pivy.quarter.eventhandlers.EventHandler',
                  'pivy.quarter.eventhandlers.EventManager',
                  'pivy.quarter.plugins.designer.python.PyQuarterWidgetPlugin',
                  'pivy.utils',
                  'pivy.interactive',
                  'pivy.interactive.colors',
                  'pivy.interactive.mesh',
                  'pivy.interactive.plot',
                  'pivy.interactive.viewer']

    def check_with_cmake(self):
        dirname = os.path.join(os.path.dirname(__file__), "distutils_cmake")
        cmake_command = ['cmake', dirname]
        try:
            cmake_command += ['-G', os.environ['GENERATOR']]
        except KeyError:
            pass
        print(yellow('calling: ' + cmake_command[0] + ' ' + cmake_command[1]))
        cmake = subprocess.Popen(cmake_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        cmake_out, cmake_err = cmake.communicate()
        coin_vars = ['COIN_FOUND', 'COIN_VERSION', 'COIN_INCLUDE_DIR', 'COIN_LIB_DIR']
        soqt_vars = ['SOQT_FOUND', 'SOQT_VERSION', 'SOQT_INCLUDE_DIR', 'SOQT_LIB_DIR']
        config_dict = {}
        print(yellow(cmake_out.decode("utf-8")))
        print(red(cmake_err.decode("utf-8")))
        if cmake.returncode == 0:
            for line in cmake_out.decode("utf-8").split("\n"):
                for var in coin_vars + soqt_vars:
                    if var in line:
                        line = (line
                                .replace('-- ' + var, '')
                                .replace(': ', '')
                                .replace('\n', ''))
                        if "INCLUDE" in var:
                            line = line.replace(';', '\" -I\"')
                        config_dict[var] = line

        print(yellow('\nchecking for COIN via cmake'))
        for key in coin_vars:
            if key in config_dict:
                print(blue(key + ': ' + config_dict[key]))

        print(yellow('\nchecking for SOQT via cmake'))
        for key in soqt_vars:
            if key in config_dict:
                print(blue(key + ': ' + config_dict[key]))

        if config_dict.get('SOQT_FOUND', 'false') == 'false':
            pivy_build.MODULES.pop('soqt')
            print(red("\ndisable soqt, because cmake couldn't find it"))
        else:
            try:
                import qtinfo
                self.QTINFO = qtinfo.QtInfo()
            except Exception as e:
                import traceback
                print(red("\ndisable soqt, because there was a problem running qtinfo (needs qmake)"))
                print(red("-" * 60))
                print(red(traceback.print_exc()))
                print(red("-" * 60))
                pivy_build.MODULES.pop('soqt')

        self.cmake_config_dict = config_dict
        if self.cmake_config_dict.get('COIN_FOUND', 'false') == 'false':
            raise(RuntimeError('coin was not found, but you need coin to build pivy'))

    def do_os_popen(self, cmd):
        "return the output of a command in a single line"
        fd = os.popen(cmd)
        lines = fd.readlines()
        for i in range(len(lines)):
            lines[i] = lines[i].strip()
        lines = " ".join(lines)
        fd.close()
        return lines

    def check_cmd_exists(self, cmd):
        "return the path of the specified command if it exists"
        print(blue("Checking for %s..." % cmd))
        for path in os.environ['PATH'].split(os.path.pathsep):
            if os.path.exists(os.path.join(path, cmd)):
                print(blue("'%s'" % os.path.join(path, cmd)))
                return 1
        print(red("not found."))
        return 0

    def check_python_version(self):
        "check the Python version"
        print(blue("Python version...%s" % sys.version.split(" ")[0]))
        if int(sys.version[0]) < 2:
            print(red("Pivy only works with Python versions >= 2.0."))
            sys.exit(1)

    def check_coin_version(self):
        '''
        check the Coin version
        '''
        print(yellow('\ncheck_coin_version is not supported in this version'))
        print(yellow('coin-bindings are build by default'))
        print(yellow('checks have been disabled because of missing config files'))
        print(yellow('make sure you have installed the coin library + headers!'))
        return #TODO

        if sys.platform == "win32":
            return
        if not self.check_cmd_exists("coin-config"):
            sys.exit(1)
        print(blue("Coin version..."))
        version = self.do_os_popen("coin-config --version")
        print(blue("%s" % version))
        # if not version.startswith('3'):
        #     print(yellow("** Warning: Pivy has only been tested with Coin "
        #                  "versions Coin-dev 3."))

    def check_simvoleon_version(self):
        '''return if SIMVoleon is available and check the version'''
        print(yellow('\ncheck_simvoleon_version is not supported in this version'))
        return #TODO

        if sys.platform == "win32" or not self.check_cmd_exists("simvoleon-config"):
            self.MODULES.pop('simvoleon', None)
            return False

        print(blue("SIMVoleon version..."))
        version = self.do_os_popen("simvoleon-config --version")
        print(blue("%s" % version))
        if not version.startswith('2.0'):
            print(yellow("** Warning: Pivy has only been tested with SIMVoleon "
                         "versions 2.0.x."))
        return True

    def check_gui_bindings(self):
        '''check for availability of SoGui bindings and removes the not available ones'''

        print(yellow('\ncheck_gui_bindings is not supported in this version'))
        print(yellow('soqt is build by default'))
        print(yellow('make sure you have installed the soqt library + headers\n'))
        return #TODO

        if sys.platform == "_win32":
            self.MODULES.pop('soxt', None)
            self.MODULES.pop('sogtk', None)
            print(blue("Checking for SoWin..."))
            if not os.path.exists(os.path.join(os.getenv("COINDIR"), "include", "Inventor", "Win", "SoWin.h")):
                self.MODULES.pop('sowin', None)
                print(red("COINDIR\\include\\Inventor\\Win\\SoWin.h not found. (SoWin bindings won't be built)"))
            print(blue("Checking for QTDIR environment variable..."))
            if os.getenv("QTDIR"):
                print(blue(os.getenv("QTDIR")))
            else:
                self.MODULES.pop('soqt', None)
                print(red("not set. (SoQt bindings won't be built)"))
        else:
            for gui in self.SOGUI:
                if gui not in self.MODULES:
                    continue
                gui_config_cmd = self.MODULES[gui][1]
                if not self.check_cmd_exists(gui_config_cmd):
                    self.MODULES.pop(gui, None)
                else:
                    print(blue("Checking for %s version..." % gui))
                    version = self.do_os_popen("%s --version" % gui_config_cmd)
                    print(blue("%s" % version))

    def get_coin_features(self):
        '''
        set the global variable SWIG_COND_SYMBOLS needed for conditional wrapping
        '''
        print(yellow('\ncoin-features are not supported in this version'))
        return #TODO

        if sys.platform == "win32":
            return
        print(blue("Checking for Coin features..."))
        if not os.system("coin-config --have-feature 3ds_import"):
            self.SWIG_COND_SYMBOLS.append("-DHAVE_FEATURE_3DS_IMPORT")
            print(green("3ds import "))

        if not os.system("coin-config --have-feature vrml97"):
            self.SWIG_COND_SYMBOLS.append("-DHAVE_FEATURE_VRML97")
            print(green("vrml97 "))

        if not os.system("coin-config --have-feature sound"):
            self.SWIG_COND_SYMBOLS.append("-DHAVE_FEATURE_SOUND")
            print(green("sound "))

        if not os.system("coin-config --have-feature superglu"):
            self.SWIG_COND_SYMBOLS.append("-DHAVE_FEATURE_SUPERGLUE")
            print(green("superglu "))

        if not os.system("coin-config --have-feature threads"):
            self.SWIG_COND_SYMBOLS.append("-DHAVE_FEATURE_THREADS")
            print(green("threads "))

        if not os.system("coin-config --have-feature threadsafe"):
            self.SWIG_COND_SYMBOLS.append("-DHAVE_FEATURE_THREADSAFE")
            print(green("threadsafe "))

        print()

    def check_swig_version(self, swig):
        "check for the swig version"
        global SWIG_VERSION
        if not self.check_cmd_exists(swig):
            # on some systems there is only a swig3.0 so check for this and
            # set SWIG to "swig3.0"
            swig = "swig3.0"
            if not self.check_cmd_exists(swig):
                sys.exit(1)
            else:
                self.SWIG = swig
        print(blue("Checking for SWIG version..."))
        p = subprocess.Popen("%s -version" % swig,
                             shell=True, stdout=subprocess.PIPE)
        version = str(p.stdout.readlines()[1].strip()).split(" ")[2]
        if version[-1] == "'":
            version = version[:-1]
        p.stdout.close()
        print(blue("%s" % version))
        SWIG_VERSION = version
        if version not in self.SUPPORTED_SWIG_VERSIONS:
            print(yellow("Warning: Pivy has only been tested with the following " +
                         "SWIG versions: %s." % " ".join(self.SUPPORTED_SWIG_VERSIONS)))

    def copy_and_swigify_headers(self, includedir, dirname, files):
        """Copy the header files to the local include directories. Add an
        #include line at the beginning for the SWIG interface files..."""
        for file in files:
            if not os.path.isfile(os.path.join(dirname, file)):
                continue

            if file[-2:] == ".i":
                file_i = os.path.join(dirname, file)
                file_h = os.path.join(dirname, file)[:-2] + ".h"

                if (not os.path.exists(file_h) and os.path.exists(os.path.join(includedir, file_h))):
                    shutil.copyfile(os.path.join(includedir, file_h), file_h)
                    sys.stdout.write(' ' + turquoise(file_h))
                    fd = open(file_h, 'r+')
                    contents = fd.readlines()

                    ins_line_nr = -1
                    for line in contents:
                        ins_line_nr += 1
                        if line.find("#include ") != -1:
                            break

                    if ins_line_nr != -1:
                        contents.insert(ins_line_nr, self.pivy_header_include % (file_i))
                        fd.seek(0)
                        fd.writelines(contents)
                    else:
                        print(blue("[") + red("failed") + blue("]"))
                        sys.exit(1)
                    fd.close
            # fixes for SWIG 1.3.21 and upwards
            # (mostly workarounding swig's preprocessor "function like macros"
            # preprocessor bug when no parameters are provided which then results
            # in no constructors being created in the wrapper)
            elif file[-4:] == ".fix":
                sys.stdout.write(' ' + red(os.path.join(dirname, file)[:-4]))
                shutil.copyfile(os.path.join(dirname, file),
                                os.path.join(dirname, file)[:-4])
            # had to introduce this because windows is a piece of crap
            elif sys.platform == "win32" and file[-6:] == ".win32":
                sys.stdout.write(' ' + red(os.path.join(dirname, file)[:-6]))
                shutil.copyfile(os.path.join(dirname, file),
                                os.path.join(dirname, file)[:-6])

    def pivy_configure(self):
        '''
        configure pivy
        '''
        print(turquoise(self.PIVY_SNAKES))
        print(blue("Platform...%s" % sys.platform))
        self.check_python_version()
        self.check_swig_version(self.SWIG)
        # self.check_coin_version()

        self.check_with_cmake()

        # TODO: find a way to enable coin-features
        self.get_coin_features()
        # if self.SOGUI:
        #     self.check_gui_bindings()

        if 'simvoleon' in self.MODULES and self.check_simvoleon_version():
            if sys.platform == "win32":
                INCLUDE_DIR = os.getenv("SIMVOLEONDIR") + "\\include"
            else:
                INCLUDE_DIR = self.do_os_popen("simvoleon-config --includedir")

            sys.stdout.write(blue("Preparing") + green(" VolumeViz ") + blue("headers:"))
            dir_gen = os.walk("VolumeViz", INCLUDE_DIR)
            for _dir, _, names in dir_gen:
                self.copy_and_swigify_headers(INCLUDE_DIR, _dir, names)
            print(green("."))

        # if sys.platform == "win32":
        #     INCLUDE_DIR = os.path.join(os.getenv("COINDIR"), "include")
        # else:
            # INCLUDE_DIR = sysconfig.get_config_var("INCLUDEDIR")

        # TODO: Check on win
        INCLUDE_DIR = self.cmake_config_dict['COIN_INCLUDE_DIR']

        sys.stdout.write(blue("Preparing") + green(" Inventor ") + blue("headers:"))
        dir_gen = os.walk("Inventor", INCLUDE_DIR)
        for _dir, _, names in dir_gen:
            self.copy_and_swigify_headers(INCLUDE_DIR, _dir, names)
        print(green("."))

    def swig_generate(self):
        "build all available modules"

        def quote(s): return '"' + s + '"'

        def win_quote(s):
            if sys.platform == 'win32':
                return '"' + s + '"'
            return s

        for module in self.MODULES:
            module_name = self.MODULES[module][0]
            config_cmd = self.MODULES[module][1]
            module_pkg_name = self.MODULES[module][2]
            mod_hack_name = self.MODULES[module][3]
            mod_out_prefix = module_pkg_name.replace('.', os.sep) + module
            try:
                CPP_FLAGS = os.environ["PIVY_CPP_FLAGS"]
            except KeyError:
                CPP_FLAGS = ""

            if sys.platform == "_win32":  # this should never happen
                INCLUDE_DIR = os.path.join(os.getenv("COINDIR"), "include")
                CPP_FLAGS += "-I" + quote(INCLUDE_DIR) + " " + \
                            "-I" + quote(os.path.join(os.getenv("COINDIR"), "include", "Inventor", "annex")) + \
                            " /DCOIN_DLL /wd4244 /wd4049"
                # acquire highest non-debug Coin library version
                try:
                    LDFLAGS_LIBS = quote(
                        max(glob.glob(os.path.join(os.getenv("COINDIR"), "lib", "coin?.lib")))) + " "
                # with cmake the coin library is named Coin4.lib
                except ValueError:
                    LDFLAGS_LIBS = quote(
                        max(glob.glob(os.path.join(os.getenv("COINDIR"), "lib", "Coin?.lib")))) + " "

                if module == "sowin":
                    CPP_FLAGS += " /DSOWIN_DLL"
                    LDFLAGS_LIBS += quote(os.path.join(os.getenv("COINDIR"), "lib", "sowin1.lib"))
                elif module == "soqt":
                    CPP_FLAGS += " -I" + '"' + os.getenv("QTDIR") + "\\include\"  /DSOQT_DLL"
                    if os.path.isdir(os.getenv("QTDIR") + "\\include\Qt\""):
                        CPP_FLAGS += " -I" + '"' + os.getenv("QTDIR") + "\\include\Qt\""
                        LDFLAGS_LIBS += os.path.join(os.getenv("COINDIR"), "lib", "soqt1.lib") + " "
                    else:
                        # workaround for conda qt4:
                        CPP_FLAGS += " -I" + '"' + os.getenv("QTDIR") + "\\include\qt\Qt\""
                        CPP_FLAGS += " -I" + '"' + os.getenv("QTDIR") + "\\include\qt\""
                        LDFLAGS_LIBS += os.path.join(os.getenv("COINDIR"), "lib", "SoQt.lib") + " "
            else:
                INCLUDE_DIR = self.cmake_config_dict[config_cmd + '_INCLUDE_DIR']
                LIB_DIR = self.cmake_config_dict[config_cmd + '_LIB_DIR']
                if sys.platform == 'win32':
                    _INCLUDE_DIR = INCLUDE_DIR
                else:
                    # replace all quotes from INCLUDE_DIR
                    _INCLUDE_DIR = INCLUDE_DIR.replace('"', "")
                CPP_FLAGS += ' -I' + _INCLUDE_DIR
                CPP_FLAGS += ' -I' + os.path.join(_INCLUDE_DIR, 'Inventor', 'annex')
                if sys.platform == 'win32': 
                    CPP_FLAGS += " /DCOIN_DLL /wd4244 /wd4049"
                    LDFLAGS_LIBS = quote(max(glob.glob(os.path.join(LIB_DIR, "Coin?.lib")))) + " "
                else:
                    CPP_FLAGS += " -Wno-unused -Wno-maybe-uninitialized"
                    LDFLAGS_LIBS = ' -L' + self.cmake_config_dict[config_cmd + '_LIB_DIR']

                if module == "soqt":
                    CPP_FLAGS += ' -I' + win_quote(self.QTINFO.getHeadersPath())
                    CPP_FLAGS += ' -I' + win_quote(os.path.join(self.QTINFO.getHeadersPath(), 'QtCore'))
                    CPP_FLAGS += ' -I' + win_quote(os.path.join(self.QTINFO.getHeadersPath(), 'QtGui'))
                    CPP_FLAGS += ' -I' + win_quote(os.path.join(self.QTINFO.getHeadersPath(), 'QtOpenGL'))
                    CPP_FLAGS += ' -I' + win_quote(os.path.join(self.QTINFO.getHeadersPath(), 'QtWidgets'))
                    if sys.platform == 'win32':
                        LDFLAGS_LIBS += " " + quote(max(glob.glob(os.path.join(LIB_DIR, "SoQt?.lib")))) + " "
                        CPP_FLAGS += " /DSOQT_DLL"
                    else:
                        LDFLAGS_LIBS += ' -lSoQt'
                
                if module == "coin":
                    if sys.platform == 'win32':
                        pass
                    else:
                        LDFLAGS_LIBS += ' -lCoin'

            if not os.path.isfile(mod_out_prefix + "_wrap.cpp"):
                print(red("\n=== Generating %s_wrap.cpp for %s ===\n" %
                          (mod_out_prefix, module)))
                print(blue(self.SWIG + " " + self.SWIG_SUPPRESS_WARNINGS + " " + self.SWIG_PARAMS %
                           (INCLUDE_DIR,
                            self.CXX_INCS,
                            mod_out_prefix, module)))
                if os.system(self.SWIG + " " + self.SWIG_SUPPRESS_WARNINGS + " " + self.SWIG_PARAMS %
                             (INCLUDE_DIR,
                              self.CXX_INCS,
                              mod_out_prefix, mod_hack_name)):
                    print(red("SWIG did not generate wrappers successfully! ** Aborting **"))
                    sys.exit(1)
            else:
                print(red("=== %s_wrap.cpp for %s already exists! ===" %
                          (mod_out_prefix, module_pkg_name + module)))

            self.ext_modules.append(Extension(module_name, [mod_out_prefix + "_wrap.cpp"],
                                              extra_compile_args=(
                                                  self.CXX_INCS + CPP_FLAGS).split(),
                                              extra_link_args=(self.CXX_LIBS + LDFLAGS_LIBS).split()))

    def run(self):
        "the entry point for the distutils build class"
        # if sys.platform == "win32" and not os.getenv("COINDIR"):
        #     print("Please set the COINDIR environment variable to your Coin root directory! ** Aborting **")
        #     sys.exit(1)

        self.pivy_configure()
        self.swig_generate()

        for cmd_name in self.get_sub_commands():
            self.run_command(cmd_name)


class pivy_clean(clean):
    pivy_path = 'pivy' + os.sep
    gui_path = 'pivy' + os.sep + 'gui' + os.sep
    REMOVE_FILES = (pivy_path + '__init__.pyc', gui_path + '__init__.pyc',
                    pivy_path + 'coin_wrap.cpp', pivy_path + 'coin2_wrap.cpp', pivy_path + 'coin.py', pivy_path + 'coin.pyc',
                    pivy_path + 'simvoleon_wrap.cpp', pivy_path + 'simvoleon.py', pivy_path + 'simvoleon.pyc',
                    gui_path + 'soqt_wrap.cpp', gui_path + 'soqt.py', gui_path + 'soqt.pyc',
                    gui_path + 'sogtk_wrap.cpp', gui_path + 'sogtk.py', gui_path + 'sogtk.py',
                    gui_path + 'soxt_wrap.cpp', gui_path + 'soxt.py', gui_path + 'soxt.pyc',
                    gui_path + 'sowin_wrap.cpp', gui_path + 'sowin.py', gui_path + 'sowin.pyc',
                    pivy_path + 'sogui.pyc')

    def remove_headers(self, arg, dirname, files):
        "remove the coin headers from the pivy Inventor directory"
        for file in files:
            if not os.path.isfile(os.path.join(dirname, file)) or file[-2:] != ".h":
                continue
            sys.stdout.write(' ' + turquoise(os.path.join(dirname, file)))
            os.remove(os.path.join(dirname, file))

    def run(self):
        "the entry point for the distutils clean class"
        sys.stdout.write(blue("Cleaning headers:"))
        dir_gen = os.walk("Inventor")
        for _dir, _, names in dir_gen:
            self.remove_headers(None, _dir, names)

        dir_gen = os.walk("VolumeViz")
        for _dir, _, names in dir_gen:
            self.remove_headers(None, _dir, names)

        self.remove_cmake()

        # remove the SWIG generated wrappers
        for wrapper_file in self.REMOVE_FILES:
            if os.path.isfile(wrapper_file):
                sys.stdout.write(' ' + turquoise(wrapper_file))
                os.remove(wrapper_file)
        print(green("."))

        clean.run(self)

    def remove_cmake(self):
        if os.path.isdir('CMakeFiles'):
            shutil.rmtree('CMakeFiles')
        if os.path.isfile('CMakeCache.txt'):
            os.remove('CMakeCache.txt')


for i in reversed(list(range(len(sys.argv)))):
    if sys.argv[i][:10] == "--without-":
        pivy_build.MODULES.pop(sys.argv[i][10:], None)
        del sys.argv[i]

 
setup(name="Pivy",
      version=PIVY_VERSION,
      description="A Python binding for Coin",
      long_description=__doc__,
      author="Tamer Fahmy",
      author_email="tamer@sim.no",
      download_url="https://github.com/coin3d/pivy/releases",
      url="http://pivy.coin3d.org/",
      cmdclass={'build': pivy_build,
                  'clean': pivy_clean},
      ext_package='pivy',
      ext_modules=pivy_build.ext_modules,
      py_modules=pivy_build.py_modules,
      packages=['pivy', 'pivy.gui'],
      classifiers=[_f for _f in PIVY_CLASSIFIERS.split("\n") if _f],
      license="BSD License",
      platforms=['Any']
      )
