/***************************************************************************
 *   (c) Juergen Riegel (juergen.riegel@web.de) 2008                       *
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/
#include <FCConfig.h>

#ifdef _PreComp_
# undef _PreComp_
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

#include <stdio.h>
#include <sstream>


// FreeCAD Base header
#include <Base/Exception.h>
#include <App/Application.h>


#if defined(FC_OS_WIN32)
# include <windows.h>

/** DllMain is called when DLL is loaded
 */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
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

    int    argc=1;
    char** argv;
    argv = (char**)malloc(sizeof(char*)* (argc+1));

#if defined(FC_OS_WIN32)
    argv[0] = (char*)malloc(MAX_PATH);
    strncpy(argv[0],App::Application::Config()["AppHomePath"].c_str(),MAX_PATH);
    argv[0][MAX_PATH-1] = '\0'; // ensure null termination
#elif defined(FC_OS_CYGWIN)
    HMODULE hModule = GetModuleHandle("FreeCAD.dll");
    char szFileName [MAX_PATH];
    GetModuleFileNameA(hModule, szFileName, MAX_PATH-1);
    argv[0] = (char*)malloc(MAX_PATH);
    strncpy(argv[0],szFileName,MAX_PATH);
    argv[0][MAX_PATH-1] = '\0'; // ensure null termination
#elif defined(FC_OS_LINUX) || defined(FC_OS_BSD)
    putenv("LANG=C");
    putenv("LC_ALL=C");
    // get whole path of the library
    Dl_info info;
#if PY_MAJOR_VERSION >= 3
    int ret = dladdr((void*)PyInit_FreeCAD, &info);
#else
    int ret = dladdr((void*)initFreeCAD, &info);
#endif
    if ((ret == 0) || (!info.dli_fname)) {
        free(argv);
        PyErr_SetString(PyExc_ImportError, "Cannot get path of the FreeCAD module!");
#if PY_MAJOR_VERSION >= 3
        return 0;
#else
        return;
#endif
    }

    argv[0] = (char*)malloc(PATH_MAX);
    strncpy(argv[0], info.dli_fname,PATH_MAX);
    argv[0][PATH_MAX-1] = '\0'; // ensure null termination
    // this is a workaround to avoid a crash in libuuid.so
#elif defined(FC_OS_MACOSX)

    // The MacOS approach uses the Python sys.path list to find the path
    // to FreeCAD.so - this should be OS-agnostic, except these two
    // strings, and the call to access().
    const static char libName[] = "/FreeCAD.so";
    const static char upDir[] = "/../";

    char *buf = NULL;

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

#if PY_MAJOR_VERSION >= 3
            if ( PyUnicode_Check(pyPath) ) {
                // Python 3 string
                basePath = PyUnicode_AsUTF8AndSize(pyPath, &sz);
            }
#else
            if ( PyString_Check(pyPath) ) {
                // Python 2 string type
                PyString_AsStringAndSize(pyPath, &basePath, &sz);
            }
            else if ( PyUnicode_Check(pyPath) ) {
                // Python 2 unicode type - explicitly use UTF-8 codec
                PyObject *fromUnicode = PyUnicode_AsUTF8String(pyPath);
                PyString_AsStringAndSize(fromUnicode, &basePath, &sz);
                Py_XDECREF(fromUnicode);
            }
#endif // #if/else PY_MAJOR_VERSION >= 3
            else {
                continue;
            }

            if (sz + sizeof(libName) > PATH_MAX) {
                continue;
            }

            // buf gets assigned to argv[0], which is free'd at the end
            buf = (char *)malloc(sz + sizeof(libName));
            if (buf == NULL) {
                break;
            }

            strcpy(buf, basePath);

            // append libName to buf
            strcat(buf, libName);
            if (access(buf, R_OK | X_OK) == 0) {

                // The FreeCAD "home" path is one level up from
                // libName, so replace libName with upDir.
                strcpy(buf + sz, upDir);
                buf[sz + sizeof(upDir)] = '\0';
                break;
            }
        } // end for (i = PyList_Size(pySysPath) - 1; i >= 0 ; --i) {
    } // end if ( PyList_Check(pySysPath) ) {

    if (buf == NULL) {
        PyErr_SetString(PyExc_ImportError, "Cannot get path of the FreeCAD module!");
#if PY_MAJOR_VERSION >= 3
        return 0;
#else
        return;
#endif
    }

    argv[0] = buf;
#else
# error "Implement: Retrieve the path of the module for your platform."
#endif
    argv[argc] = 0;

    try {
        // Inits the Application
        App::Application::init(argc,argv);
    }
    catch (const Base::Exception& e) {
        std::string appName = App::Application::Config()["ExeName"];
        std::stringstream msg;
        msg << "While initializing " << appName << " the following exception occurred: '"
            << e.what() << "'\n\n";
        msg << "\nPlease contact the application's support team for more information.\n\n";
        printf("Initialization of %s failed:\n%s", appName.c_str(), msg.str().c_str());
    }

    free(argv[0]);
    free(argv);

#if PY_MAJOR_VERSION >= 3
    //PyObject* module = _PyImport_FindBuiltin("FreeCAD");
    PyObject* modules = PyImport_GetModuleDict();
    PyObject* module = PyDict_GetItemString(modules, "FreeCAD");
    if (!module) {
        PyErr_SetString(PyExc_ImportError, "Failed to load FreeCAD module!");
    }
    return module;
#endif
}

