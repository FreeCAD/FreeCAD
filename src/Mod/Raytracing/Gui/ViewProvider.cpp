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
        QStringList items;
        auto addTemplates = [&items](const std::string& path) {
            QString dataDir = QString::fromUtf8(path.c_str());
            QDir dir(dataDir);
            QFileInfoList files = dir.entryInfoList(QStringList() << QString::fromLatin1("*.lxs"));
            for (int i=0; i<files.count(); i++ ) {
                QFileInfo fi(files[i]);
                items << fi.absoluteFilePath();
            }
        };

        std::string path = App::Application::getResourceDir();
        path += "Mod/Raytracing/Templates/";
        addTemplates(path);

        path = App::Application::getUserAppDataDir();
        path += "data/Mod/Raytracing/Templates/";
        addTemplates(path);

        QFileInfo cfi(QString::fromUtf8(static_cast<Raytracing::LuxProject*>(getObject())->Template.getValue()));
        int current = items.indexOf(cfi.absoluteFilePath());

        bool ok;
        QString file = QInputDialog::getItem(Gui::getMainWindow(), tr("LuxRender template"), tr("Select a LuxRender template"), items, current, false, &ok, Qt::MSWindowsFixedSizeDialogHint);
        if (ok) {
            App::Document* doc  = getObject()->getDocument();
            doc->openTransaction("Edit LuxRender project");
            static_cast<Raytracing::LuxProject*>(getObject())->Template.setValue((const char*)file.toUtf8());
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
        // Do nothing here
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
        QStringList items;
        auto addTemplates = [&items](const std::string& path) {
            QString dataDir = QString::fromUtf8(path.c_str());
            QDir dir(dataDir);
            QFileInfoList files = dir.entryInfoList(QStringList() << QString::fromLatin1("*.pov"));
            for (int i=0; i<files.count(); i++ ) {
                QFileInfo fi(files[i]);
                items << fi.absoluteFilePath();
            }
        };

        std::string path = App::Application::getResourceDir();
        path += "Mod/Raytracing/Templates/";
        addTemplates(path);

        path = App::Application::getUserAppDataDir();
        path += "data/Mod/Raytracing/Templates/";
        addTemplates(path);

        QFileInfo cfi(QString::fromUtf8(static_cast<Raytracing::RayProject*>(getObject())->Template.getValue()));
        int current = items.indexOf(cfi.absoluteFilePath());

        bool ok;
        QString file = QInputDialog::getItem(Gui::getMainWindow(), tr("Povray template"), tr("Select a Povray template"), items, current, false, &ok, Qt::MSWindowsFixedSizeDialogHint);
        if (ok) {
            App::Document* doc  = getObject()->getDocument();
            doc->openTransaction("Edit Povray project");
            static_cast<Raytracing::RayProject*>(getObject())->Template.setValue((const char*)file.toUtf8());
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
        // Do nothing here
    }
    else {
        ViewProviderDocumentObjectGroup::unsetEdit(ModNum);
    }
}
