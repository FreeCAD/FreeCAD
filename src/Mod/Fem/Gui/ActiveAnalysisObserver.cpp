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

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/Fem/App/FemAnalysis.h>

#include "ActiveAnalysisObserver.h"


using namespace FemGui;

ActiveAnalysisObserver* ActiveAnalysisObserver::inst = nullptr;

ActiveAnalysisObserver* ActiveAnalysisObserver::instance()
{
    if (!inst) {
        inst = new ActiveAnalysisObserver();
    }
    return inst;
}

ActiveAnalysisObserver::ActiveAnalysisObserver() = default;

ActiveAnalysisObserver::~ActiveAnalysisObserver() = default;

void ActiveAnalysisObserver::setActiveObject(Fem::FemAnalysis* fem)
{
    if (fem) {
        activeObject = fem;
        App::Document* doc = fem->getDocument();
        activeDocument = Gui::Application::Instance->getDocument(doc);
        activeView = static_cast<Gui::ViewProviderDocumentObject*>(
            activeDocument->getViewProvider(activeObject));
        attachDocument(doc);
    }
    else {
        activeObject = nullptr;
        activeView = nullptr;
    }
}

Fem::FemAnalysis* ActiveAnalysisObserver::getActiveObject() const
{
    return activeObject;
}

bool ActiveAnalysisObserver::hasActiveObject() const
{
    return activeObject != nullptr;
}

void ActiveAnalysisObserver::highlightActiveObject(const Gui::HighlightMode& mode, bool on)
{
    if (activeDocument && activeView) {
        activeDocument->signalHighlightObject(*activeView, mode, on, 0, 0);
    }
}

void ActiveAnalysisObserver::slotDeletedDocument(const App::Document& Doc)
{
    App::Document* d = getDocument();
    if (d == &Doc) {
        activeObject = nullptr;
        activeDocument = nullptr;
        activeView = nullptr;
        detachDocument();
    }
}

void ActiveAnalysisObserver::slotDeletedObject(const App::DocumentObject& Obj)
{
    if (activeObject == &Obj) {
        activeObject = nullptr;
        activeView = nullptr;
    }
}
