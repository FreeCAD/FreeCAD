/***************************************************************************
 *   Copyright (c) 2024 Pierre-Louis Boyer <development@ondsel.com>        *
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
#include <cmath>

#include <QGuiApplication>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

#include <Inventor/events/SoKeyboardEvent.h>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "MDIViewPage.h"
#include "QGVPage.h"
#include "TechDrawHandler.h"


using namespace TechDrawGui;

/**************************** TechDrawHandler *******************************************/

TechDrawHandler::TechDrawHandler() : Gui::ToolHandler(), viewPage(nullptr)
{}

TechDrawHandler::~TechDrawHandler()
{}

void TechDrawHandler::activate(QGVPage* vp)
{
    auto* mdi = qobject_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (!mdi) {
        return;
    }
    mdi->enableContextualMenu(false);

    viewPage = vp;

    if (!Gui::ToolHandler::activate()) {
        viewPage->deactivateHandler();
    }
}

void TechDrawHandler::deactivate()
{
    Gui::ToolHandler::deactivate();

    // The context menu event of MDIViewPage comes after the tool is deactivated.
    // So to prevent the menu from appearing when the tool is cleared by right mouse click
    // we set a small timer.
    QTimer::singleShot(100, []() { // 100 milliseconds delay
        auto* mdi = qobject_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
        if (!mdi) {
            return;
        }
        mdi->enableContextualMenu(true);
    });
}

void TechDrawHandler::keyReleaseEvent(QKeyEvent* event)
{
    // the default behaviour is to quit - specific handler categories may
    // override this behaviour, for example to implement a continuous mode
    if (event->key() == Qt::Key_Escape) {
        quit();
        event->accept();
    }
}

void TechDrawHandler::mouseReleaseEvent(QMouseEvent* event)
{
    // the default behaviour is to quit - specific handler categories may
    // override this behaviour, for example to implement a continuous mode
    if (event->button() == Qt::RightButton) {
        quit();
        event->accept();
    }
}

void TechDrawHandler::quit()
{
    viewPage->deactivateHandler();
}

QWidget* TechDrawHandler::getCursorWidget()
{
    return viewPage;
}

void TechDrawHandler::setWidgetCursor(QCursor cursor)
{
    if (viewPage) {
        viewPage->setCursor(cursor);
        viewPage->viewport()->setCursor(cursor);
    }
}

TechDraw::DrawPage* TechDrawHandler::getPage()
{
    return viewPage->getDrawPage();
}