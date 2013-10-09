/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QAction>
# include <QDir>
# include <QFileInfo>
# include <QMenu>
# include <QInputDialog>
#endif

#include "ViewProvider.h"
#include <Mod/Raytracing/App/LuxProject.h>
#include <Mod/Raytracing/App/RayProject.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>


using namespace RaytracingGui;


/* TRANSLATOR RaytracingGui::ViewProviderLux */

PROPERTY_SOURCE(RaytracingGui::ViewProviderLux, Gui::ViewProviderDocumentObjectGroup)

ViewProviderLux::ViewProviderLux()
{
}

ViewProviderLux::~ViewProviderLux()
{
}

bool ViewProviderLux::doubleClicked(void)
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

void ViewProviderLux::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(tr("Edit LuxRender project"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    ViewProviderDocumentObjectGroup::setupContextMenu(menu, receiver, member);
}

bool ViewProviderLux::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        std::string path = App::Application::getResourceDir();
        path += "Mod/Raytracing/Templates/";
        QString dataDir = QString::fromUtf8(path.c_str());
        QDir dir(dataDir, QString::fromAscii("*.lxs"));
        QStringList items;
        int current = 0;
        QFileInfo cfi(QString::fromUtf8(static_cast<Raytracing::LuxProject*>(getObject())->Template.getValue()));
        for (unsigned int i=0; i<dir.count(); i++ ) {
            QFileInfo fi(dir[i]);
            items << fi.baseName();
            if (fi.baseName() == cfi.baseName()) {
                current = i;
            }
        }

        bool ok;
        QString file = QInputDialog::getItem(Gui::getMainWindow(), tr("LuxRender template"), tr("Select a LuxRender template"), items, current, false, &ok);
        if (ok) {
            App::Document* doc  = getObject()->getDocument();
            doc->openTransaction("Edit LuxRender project");
            QString fn = QString::fromAscii("%1%2.lxs").arg(dataDir).arg(file);
            static_cast<Raytracing::LuxProject*>(getObject())->Template.setValue((const char*)fn.toUtf8());
            doc->commitTransaction();
            doc->recompute();
        }
        return false;
    }
    else {
        return ViewProviderDocumentObjectGroup::setEdit(ModNum);
    }
}

void ViewProviderLux::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
    }
    else {
        ViewProviderDocumentObjectGroup::unsetEdit(ModNum);
    }
}

// ---------------------------------------------------------------------

/* TRANSLATOR RaytracingGui::ViewProviderPovray */

PROPERTY_SOURCE(RaytracingGui::ViewProviderPovray, Gui::ViewProviderDocumentObjectGroup)

ViewProviderPovray::ViewProviderPovray()
{
}

ViewProviderPovray::~ViewProviderPovray()
{
}

bool ViewProviderPovray::doubleClicked(void)
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

void ViewProviderPovray::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(tr("Edit Povray project"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    ViewProviderDocumentObjectGroup::setupContextMenu(menu, receiver, member);
}

bool ViewProviderPovray::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        std::string path = App::Application::getResourceDir();
        path += "Mod/Raytracing/Templates/";
        QString dataDir = QString::fromUtf8(path.c_str());
        QDir dir(dataDir, QString::fromAscii("*.pov"));
        QStringList items;
        int current = 0;
        QFileInfo cfi(QString::fromUtf8(static_cast<Raytracing::RayProject*>(getObject())->Template.getValue()));
        for (unsigned int i=0; i<dir.count(); i++ ) {
            QFileInfo fi(dir[i]);
            items << fi.baseName();
            if (fi.baseName() == cfi.baseName()) {
                current = i;
            }
        }

        bool ok;
        QString file = QInputDialog::getItem(Gui::getMainWindow(), tr("Povray template"), tr("Select a Povray template"), items, current, false, &ok);
        if (ok) {
            App::Document* doc  = getObject()->getDocument();
            doc->openTransaction("Edit Povray project");
            QString fn = QString::fromAscii("%1%2.pov").arg(dataDir).arg(file);
            static_cast<Raytracing::RayProject*>(getObject())->Template.setValue((const char*)fn.toUtf8());
            doc->commitTransaction();
            doc->recompute();
        }
        return false;
    }
    else {
        return ViewProviderDocumentObjectGroup::setEdit(ModNum);
    }
}

void ViewProviderPovray::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
    }
    else {
        ViewProviderDocumentObjectGroup::unsetEdit(ModNum);
    }
}
