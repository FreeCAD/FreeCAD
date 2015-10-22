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

#ifdef FC_OS_LINUX
#	include <unistd.h>
#endif

#ifdef FC_OS_MACOSX
# include <mach-o/dyld.h>
# include <string>
#endif

#if HAVE_CONFIG_H
#	include <config.h>
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
        GetModuleFileName((HMODULE)hModule, szFileName, MAX_PATH-1);
        App::Application::Config()["AppHomePath"] = szFileName;
    }
    break;
    default:
        break;
    }

    return TRUE;
}
#elif defined(FC_OS_LINUX)
# ifndef GNU_SOURCE
#   define GNU_SOURCE
# endif
# include <dlfcn.h>
#elif defined(FC_OS_CYGWIN)
# include <windows.h>
#endif

#ifdef FC_OS_WIN32
#	define MainExport __declspec(dllexport)
#else
#	define MainExport
#endif

extern "C"
{
    void MainExport initFreeCAD() {

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
        GetModuleFileName(hModule, szFileName, MAX_PATH-1);
        argv[0] = (char*)malloc(MAX_PATH);
        strncpy(argv[0],szFileName,MAX_PATH);
        argv[0][MAX_PATH-1] = '\0'; // ensure null termination
#elif defined(FC_OS_LINUX)
        putenv("LANG=C");
        putenv("LC_ALL=C");
        // get whole path of the library
        Dl_info info;
        int ret = dladdr((void*)initFreeCAD, &info);
        if ((ret == 0) || (!info.dli_fname)) {
            PyErr_SetString(PyExc_ImportError, "Cannot get path of the FreeCAD module!");
            return;
        }

        argv[0] = (char*)malloc(PATH_MAX);
        strncpy(argv[0], info.dli_fname,PATH_MAX);
        argv[0][PATH_MAX-1] = '\0'; // ensure null termination
        // this is a workaround to avoid a crash in libuuid.so
#elif defined(FC_OS_MACOSX)
        uint32_t sz = 0;
        char *buf;

        _NSGetExecutablePath(NULL, &sz);
        buf = (char*) malloc(++sz);
        int err=_NSGetExecutablePath(buf, &sz);
        if (err != 0) {
            PyErr_SetString(PyExc_ImportError, "Cannot get path of the FreeCAD module!");
            return;
        }

        argv[0] = (char*)malloc(PATH_MAX);
        strncpy(argv[0], buf, PATH_MAX);
        argv[0][PATH_MAX-1] = '\0'; // ensure null termination
        free(buf);
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
            msg << "While initializing " << appName << " the  following exception occurred: '"
                << e.what() << "'\n\n";
            msg << "\nPlease contact the application's support team for more information.\n\n";
            printf("Initialization of %s failed:\n%s", appName.c_str(), msg.str().c_str());
        }

        free(argv[0]);
        free(argv);

        return;
    } //InitFreeCAD....
} // extern "C"

