/***************************************************************************
 *   Copyright (c) 2011 Jrgen Riegel (juergen.riegel@web.de)               *
 *   Copyright (c) 2013 Eivind Kvedalen (eivind@kvedalen.name)             *
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
# include <QFile>
# include <QFileInfo>
# include <QImage>
# include <QString>
# include <QMenu>
#endif

#include "ViewProviderSpreadsheet.h"
#include "SpreadsheetView.h"

#include <Mod/Spreadsheet/App/Sheet.h>
#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Console.h>
#include <sstream>

using namespace Gui;
using namespace SpreadsheetGui;
using namespace Spreadsheet;


PROPERTY_SOURCE(SpreadsheetGui::ViewProviderSheet, Gui::ViewProviderDocumentObject)

ViewProviderSheet::ViewProviderSheet()
    : Gui::ViewProviderDocumentObject()
{
}

ViewProviderSheet::~ViewProviderSheet()
{
}

void ViewProviderSheet::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderSheet::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Spreadsheet");
    return StrList;
}

bool ViewProviderSheet::doubleClicked()
{
    if (!this->view) {
        showSpreadsheetView();
        view->viewAll();
    }
    Gui::getMainWindow()->setActiveWindow(this->view);
    return true;
}

void ViewProviderSheet::setupContextMenu(QMenu * menu, QObject *receiver, const char *member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Show spreadsheet"), receiver, member);
}

Spreadsheet::Sheet *ViewProviderSheet::getSpreadsheetObject() const
{
    return dynamic_cast<Spreadsheet::Sheet*>(pcObject);
}

bool ViewProviderSheet::onDelete(const std::vector<std::string> &)
{
    return view.isNull();
}

SheetView *ViewProviderSheet::showSpreadsheetView()
{
    if (!view){
        Gui::Document* doc = Gui::Application::Instance->getDocument
            (this->pcObject->getDocument());
        view = new SheetView(this->pcObject, Gui::getMainWindow());
        view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/spreadsheet"));
        view->setWindowTitle(QObject::tr("Spreadsheet") + QString::fromAscii("[*]"));
        Gui::getMainWindow()->addWindow(view);
        startEditing();
    }

    return view;
}

void ViewProviderSheet::updateData(const App::Property* prop)
{
    if (view)
        view->updateCell(prop);
}
