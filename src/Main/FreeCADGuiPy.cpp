// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <FCConfig.h>

#if HAVE_CONFIG_H
# include <config.h>
#endif  // HAVE_CONFIG_H

#include <Build/Version.h>  // For FCCopyrightYear

#ifdef _MSC_VER
# pragma warning(disable : 4005)
#endif

#include <QApplication>

// FreeCAD Base header
#include <App/Application.h>
#include <Base/Exception.h>
#include <Base/Factory.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/ApplicationPy.h>
#include <Gui/FreeCADGuiModulePy.h>
#include <Gui/SoFCDB.h>
#include <Gui/Quarter/Quarter.h>


PyMOD_INIT_FUNC(FreeCADGui)
{
    try {
        // clang-format off
        Base::Interpreter().loadModule("FreeCAD");
        App::Application::Config()["AppIcon"] = "freecad";
        App::Application::Config()["SplashScreen"] = "freecadsplash";
        App::Application::Config()["CopyrightInfo"] = fmt::format("\xc2\xa9 Juergen Riegel, Werner Mayer, Yorik van Havre and others 2001-{}\n", FCCopyrightYear);
        App::Application::Config()["LicenseInfo"] = "FreeCAD is free and open-source software licensed under the terms of LGPL2+ license.\n";
        App::Application::Config()["CreditsInfo"] = "FreeCAD would not be possible without the FreeCAD community.\n";
        // clang-format on

        // it's possible that the GUI is already initialized when the Gui version of the executable
        // is started in command mode
        if (Base::Type::fromName("Gui::BaseView").isBad()) {
            Gui::Application::initApplication();
        }
        // Console imports only expose the bootstrap-safe FreeCADGui surface.
        static struct PyModuleDef FreeCADGuiModuleDef = {
            PyModuleDef_HEAD_INIT,
            "FreeCADGui",
            Gui::FreeCADGuiModulePy::moduleDocumentation(),
            -1,
            Gui::FreeCADGuiModulePy::BootstrapMethods,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        };
        PyObject* module = PyModule_Create(&FreeCADGuiModuleDef);
        return module;
    }
    catch (const Base::Exception& e) {
        PyErr_Format(PyExc_ImportError, "%s\n", e.what());
    }
    catch (...) {
        PyErr_SetString(PyExc_ImportError, "Unknown runtime error occurred");
    }
    return nullptr;
}
