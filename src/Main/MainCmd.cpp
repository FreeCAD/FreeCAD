/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2008                        *
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
#include "../FCConfig.h"

#ifdef _PreComp_
# undef _PreComp_
#endif

#ifdef FC_OS_LINUX
# include <unistd.h>
#endif

#if HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <sstream>
#include <iostream>

// FreeCAD Base header
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Factory.h>

// FreeCAD doc header
#include <App/Application.h>


using Base::Console;
using App::Application;

const char sBanner[] = "(c) Juergen Riegel, Werner Mayer, Yorik van Havre 2001-2019\n"\
                       "  #####                 ####  ###   ####  \n" \
                       "  #                    #      # #   #   # \n" \
                       "  #     ##  #### ####  #     #   #  #   # \n" \
                       "  ####  # # #  # #  #  #     #####  #   # \n" \
                       "  #     #   #### ####  #    #     # #   # \n" \
                       "  #     #   #    #     #    #     # #   #  ##  ##  ##\n" \
                       "  #     #   #### ####   ### #     # ####   ##  ##  ##\n\n" ;



int main( int argc, char ** argv )
{
    // Make sure that we use '.' as decimal point
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");

    // Name and Version of the Application
    App::Application::Config()["ExeName"] = "FreeCAD";
    App::Application::Config()["ExeVendor"] = "FreeCAD";
    App::Application::Config()["AppDataSkipVendor"] = "true";

    // set the banner (for logging and console)
    App::Application::Config()["CopyrightInfo"] = sBanner;

    try {
        // Init phase ===========================================================
        // sets the default run mode for FC, starts with command prompt if not overridden in InitConfig...
        App::Application::Config()["RunMode"] = "Exit";
        App::Application::Config()["LoggingConsole"] = "1";

        // Inits the Application
        App::Application::init(argc,argv);
    }
    catch (const Base::UnknownProgramOption& e) {
        std::cerr << e.what();
        exit(1);
    }
    catch (const Base::ProgramInformation& e) {
        std::cout << e.what();
        exit(0);
    }
    catch (const Base::Exception& e) {
        std::string appName = App::Application::Config()["ExeName"];
        std::stringstream msg;
        msg << "While initializing " << appName << " the following exception occurred: '" << e.what() << "'\n\n";
        msg << "Python is searching for its runtime files in the following directories:\n" << Py_GetPath() << "\n\n";
        msg << "Python version information:\n" << Py_GetVersion() << "\n";
        const char* pythonhome = getenv("PYTHONHOME");
        if ( pythonhome ) {
            msg << "\nThe environment variable PYTHONHOME is set to '" << pythonhome << "'.";
            msg << "\nSetting this environment variable might cause Python to fail. Please contact your administrator to unset it on your system.\n\n";
        }
        else {
            msg << "\nPlease contact the application's support team for more information.\n\n";
        }

        printf("Initialization of %s failed:\n%s", appName.c_str(), msg.str().c_str());
        exit(100);
    }
    catch (...) {
        std::string appName = App::Application::Config()["ExeName"];
        std::stringstream msg;
        msg << "Unknown runtime error occurred while initializing " << appName <<".\n\n";
        msg << "Please contact the application's support team for more information.\n\n";
        printf("Initialization of %s failed:\n%s", appName.c_str(), msg.str().c_str());
        exit(101);
    }

    // Run phase ===========================================================
    try {
        Application::runApplication();
    }
    catch (const Base::SystemExitException &e) {
        exit(e.getExitCode());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
        exit(1);
    }
    catch (...) {
        Console().Error("Application unexpectedly terminated\n");
        exit(1);
    }

    // Destruction phase ===========================================================
    Console().Log("FreeCAD terminating...\n");

    try {
        // close open documents
        App::GetApplication().closeAllDocuments();
    }
    catch(...) {
    }

    // cleans up
    Application::destruct();

    Console().Log("FreeCAD completely terminated\n");

    return 0;
}

