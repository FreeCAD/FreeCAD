/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
# include <stdexcept>
# include <QSessionManager>
#endif

#include "GuiApplication.h"
#include "SpaceballEvent.h"
#include "MainWindow.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include <App/Application.h>

using namespace Gui;

GUIApplication::GUIApplication(int & argc, char ** argv, int exitcode)
    : GUIApplicationNativeEventAware(argc, argv), systemExit(exitcode)
{
}

bool GUIApplication::notify (QObject * receiver, QEvent * event)
{
    if (!receiver && event) {
        Base::Console().Log("GUIApplication::notify: Unexpected null receiver, event type: %d\n",
            (int)event->type());
    }
    try {
        if (event->type() == Spaceball::ButtonEvent::ButtonEventType || 
            event->type() == Spaceball::MotionEvent::MotionEventType)
            return processSpaceballEvent(receiver, event);
        else
            return QApplication::notify(receiver, event);
    }
    catch (const Base::SystemExitException&) {
        qApp->exit(systemExit);
        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Unhandled Base::Exception caught in GUIApplication::notify.\n"
                              "The error message is: %s\n", e.what());
    }
    catch (const std::exception& e) {
        Base::Console().Error("Unhandled std::exception caught in GUIApplication::notify.\n"
                              "The error message is: %s\n", e.what());
    }
    catch (...) {
        Base::Console().Error("Unhandled unknown exception caught in GUIApplication::notify.\n");
    }

    // Print some more information to the log file (if active) to ease bug fixing
    if (receiver && event) {
        try {
            std::stringstream dump;
            dump << "The event type " << (int)event->type() << " was sent to "
                 << receiver->metaObject()->className() << "\n";
            dump << "Object tree:\n";
            if (receiver->isWidgetType()) {
                QWidget* w = qobject_cast<QWidget*>(receiver);
                while (w) {
                    dump << "\t";
                    dump << w->metaObject()->className();
                    QString name = w->objectName();
                    if (!name.isEmpty())
                        dump << " (" << (const char*)name.toUtf8() << ")";
                    w = w->parentWidget();
                    if (w)
                        dump << " is child of\n";
                }
                std::string str = dump.str();
                Base::Console().Log("%s",str.c_str());
            }
        }
        catch (...) {
            Base::Console().Log("Invalid recipient and/or event in GUIApplication::notify\n");
        }
    }

    return true;
}

void GUIApplication::commitData(QSessionManager &manager)
{
    if (manager.allowsInteraction()) {
        if (!Gui::getMainWindow()->close()) {
            // cancel the shutdown
            manager.release();
            manager.cancel();
        }
    }
    else {
        // no user interaction allowed, thus close all documents and
        // the main window
        App::GetApplication().closeAllDocuments();
        Gui::getMainWindow()->close();
    }
}

#include "moc_GuiApplication.cpp"
