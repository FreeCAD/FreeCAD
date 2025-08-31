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

#include "../FCConfig.h"

#ifdef _PreComp_
#undef _PreComp_
#endif

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <cstdio>
#include <ostream>
#include <QString>

// FreeCAD Base header
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>

// FreeCAD doc header
#include <App/Application.h>


using App::Application;
using Base::Console;

const char sBanner[] =
    "(C) 2001-2025 FreeCAD contributors\n"
    "FreeCAD is free and open-source software licensed under the terms of LGPL2+ license.\n\n";

int main(int argc, char** argv)
{
    // Make sure that we use '.' as decimal point
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");

#if defined(__MINGW32__)
    const char* mingw_prefix = getenv("MINGW_PREFIX");
    const char* py_home = getenv("PYTHONHOME");
    if (!py_home && mingw_prefix) {
        _putenv_s("PYTHONHOME", mingw_prefix);
    }
#endif

    // Name and Version of the Application
    App::Application::Config()["ExeName"] = "FreeCAD";
    App::Application::Config()["ExeVendor"] = "FreeCAD";
    App::Application::Config()["AppDataSkipVendor"] = "true";

    // set the banner (for logging and console)
    App::Application::Config()["CopyrightInfo"] = sBanner;

    try {
        // Init phase ===========================================================
        // sets the default run mode for FC, starts with command prompt if not overridden in
        // InitConfig...
        App::Application::Config()["RunMode"] = "Exit";
        App::Application::Config()["LoggingConsole"] = "1";

        // Inits the Application
        App::Application::init(argc, argv);
    }
    catch (const Base::UnknownProgramOption& e) {
        std::cerr << e.what();
        exit(1);
    }
    catch (const Base::ProgramInformation& e) {
        if (std::strcmp(e.what(), App::Application::verboseVersionEmitMessage) == 0) {
            QString data;
            QTextStream str(&data);
            const std::map<std::string, std::string> config = App::Application::Config();

            App::Application::getVerboseCommonInfo(str, config);
            App::Application::getVerboseAddOnsInfo(str, config);

            std::cout << data.toStdString();
            exit(0);
        }
        std::cout << e.what();
        exit(0);
    }
    catch (const Base::Exception& e) {
        std::string appName = App::Application::Config()["ExeName"];
        std::cout << "While initializing " << appName << " the following exception occurred: '"
                  << e.what() << "'\n\n";
        std::cout << "Python is searching for its runtime files in the following directories:\n"
                  << Base::Interpreter().getPythonPath() << "\n\n";
        std::cout << "Python version information:\n" << Py_GetVersion() << "\n";
        const char* pythonhome = getenv("PYTHONHOME");
        if (pythonhome) {
            std::cout << "\nThe environment variable PYTHONHOME is set to '" << pythonhome << "'.";
            std::cout << "\nSetting this environment variable might cause Python to fail. "
                         "Please contact your administrator to unset it on your system.";
        }
        else {
            std::cout << "\nPlease contact the application's support team for more information.";
        }
        std::cout << std::endl;
        exit(100);
    }
    catch (...) {
        std::string appName = App::Application::Config()["ExeName"];
        std::cout << "Unknown runtime error occurred while initializing " << appName << ".\n\n";
        std::cout << "Please contact the application's support team for more information.";
        std::cout << std::endl;
        exit(101);
    }

    // Run phase ===========================================================
    try {
        Application::runApplication();
    }
    catch (const Base::SystemExitException& e) {
        exit(e.getExitCode());
    }
    catch (const Base::Exception& e) {
        e.reportException();
        exit(1);
    }
    catch (...) {
        Console().error("Application unexpectedly terminated\n");
        exit(1);
    }

    // Destruction phase ===========================================================
    Console().log("FreeCAD terminating...\n");

    try {
        // close open documents
        App::GetApplication().closeAllDocuments();
    }
    catch (...) {
    }

    // cleans up
    Application::destruct();

    Console().log("FreeCAD completely terminated\n");

    return 0;
}
