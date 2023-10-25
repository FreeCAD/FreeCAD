/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <FCConfig.h>

#ifdef _PreComp_
# undef _PreComp_
#endif

#if defined(FC_OS_WIN32)
# include <windows.h>
#endif

#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
# include <unistd.h>
#endif

#ifdef FC_OS_MACOSX
# include <mach-o/dyld.h>
# include <string>
#endif

#if HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include <cstdio>
#include <sstream>
#include <iostream>
#include <QByteArray>

// FreeCAD Base header
#include <Base/ConsoleObserver.h>
#include <Base/Exception.h>
#include <Base/PyObjectBase.h>
#include <Base/Sequencer.h>
#include <App/Application.h>

#if defined(FC_OS_WIN32)

/** DllMain is called when DLL is loaded
 */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        // This name is preliminary, we pass it to Application::init() in initFreeCAD()
        // which does the rest.
        char  szFileName [MAX_PATH];
        GetModuleFileNameA((HMODULE)hModule, szFileName, MAX_PATH-1);
        App::Application::Config()["AppHomePath"] = szFileName;
    }
    break;
    default:
        break;
    }

    return true;
}
#elif defined(FC_OS_LINUX) || defined(FC_OS_BSD)
# ifndef GNU_SOURCE
#   define GNU_SOURCE
# endif
# include <dlfcn.h>
#elif defined(FC_OS_CYGWIN)
# include <windows.h>
#endif

PyMOD_INIT_FUNC(FreeCAD)
{
    // Init phase ===========================================================
    App::Application::Config()["ExeName"] = "FreeCAD";
    App::Application::Config()["ExeVendor"] = "FreeCAD";
    App::Application::Config()["AppDataSkipVendor"] = "true";

    QByteArray path;

#if defined(FC_OS_WIN32)
    path = App::Application::Config()["AppHomePath"].c_str();
#elif defined(FC_OS_CYGWIN)
    HMODULE hModule = GetModuleHandle("FreeCAD.dll");
    char szFileName [MAX_PATH];
    GetModuleFileNameA(hModule, szFileName, MAX_PATH-1);
    path = szFileName;
#elif defined(FC_OS_LINUX) || defined(FC_OS_BSD)
    putenv("LANG=C");
    putenv("LC_ALL=C");
    // get whole path of the library
    Dl_info info;
    int ret = dladdr((void*)PyInit_FreeCAD, &info);
    if ((ret == 0) || (!info.dli_fname)) {
        PyErr_SetString(PyExc_ImportError, "Cannot get path of the FreeCAD module!");
        return nullptr;
    }

    path = info.dli_fname;
    // this is a workaround to avoid a crash in libuuid.so
#elif defined(FC_OS_MACOSX)

    // The MacOS approach uses the Python sys.path list to find the path
    // to FreeCAD.so - this should be OS-agnostic, except these two
    // strings, and the call to access().
    const static char libName[] = "/FreeCAD.so";
    const static char upDir[] = "/../";

    PyObject *pySysPath = PySys_GetObject("path");
    if ( PyList_Check(pySysPath) ) {
        int i;
        // pySysPath should be a *PyList of strings - iterate through it
        // backwards since the FreeCAD path was likely appended just before
        // we were imported.
        for (i = PyList_Size(pySysPath) - 1; i >= 0 ; --i) {
            const char *basePath;
            PyObject *pyPath = PyList_GetItem(pySysPath, i);
            long sz = 0;

            if ( PyUnicode_Check(pyPath) ) {
                // Python 3 string
                basePath = PyUnicode_AsUTF8AndSize(pyPath, &sz);
            }
            else {
                continue;
            }

            if (sz + sizeof(libName) > PATH_MAX) {
                continue;
            }

            path = basePath;

            // append libName to the path
            if (access(path + libName, R_OK | X_OK) == 0) {

                // The FreeCAD "home" path is one level up from
                // libName, so replace libName with upDir.
                path += upDir;
                break;
            }
        } // end for (i = PyList_Size(pySysPath) - 1; i >= 0 ; --i) {
    } // end if ( PyList_Check(pySysPath) ) {

    if (path.isEmpty()) {
        PyErr_SetString(PyExc_ImportError, "Cannot get path of the FreeCAD module!");
        return nullptr;
    }

#else
# error "Implement: Retrieve the path of the module for your platform."
#endif
    int argc = 1;
    std::vector<char*> argv;
    argv.push_back(path.data());

    try {
        // Inits the Application
        App::Application::init(argc, argv.data());
    }
    catch (const Base::Exception& e) {
        std::string appName = App::Application::Config()["ExeName"];
        std::stringstream msg;
        msg << "While initializing " << appName << " the following exception occurred: '"
            << e.what() << "'\n\n";
        msg << "\nPlease contact the application's support team for more information.\n\n";
        printf("Initialization of %s failed:\n%s", appName.c_str(), msg.str().c_str());
    }

    Base::EmptySequencer* seq = new Base::EmptySequencer();
    (void)seq;
    static Base::RedirectStdOutput stdcout;
    static Base::RedirectStdLog    stdclog;
    static Base::RedirectStdError  stdcerr;
    std::cout.rdbuf(&stdcout);
    std::clog.rdbuf(&stdclog);
    std::cerr.rdbuf(&stdcerr);

    //PyObject* module = _PyImport_FindBuiltin("FreeCAD");
    PyObject* modules = PyImport_GetModuleDict();
    PyObject* module = PyDict_GetItemString(modules, "FreeCAD");
    if (!module) {
        PyErr_SetString(PyExc_ImportError, "Failed to load FreeCAD module!");
    }
    return module;
}

