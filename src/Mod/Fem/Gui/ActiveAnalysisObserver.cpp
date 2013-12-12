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
#include "ActiveAnalysisObserver.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/Fem/App/FemAnalysis.h>

using namespace FemGui;


ActiveAnalysisObserver* ActiveAnalysisObserver::inst = 0;

ActiveAnalysisObserver* ActiveAnalysisObserver::instance()
{
    if (!inst)
        inst = new ActiveAnalysisObserver();
    return inst;
}

ActiveAnalysisObserver::ActiveAnalysisObserver()
    : activeObject(0), activeView(0), activeDocument(0)
{
}

ActiveAnalysisObserver::~ActiveAnalysisObserver()
{
}

void ActiveAnalysisObserver::setActiveObject(Fem::FemAnalysis* fem)
{
    if (fem) {
        activeObject = fem;
        App::Document* doc = fem->getDocument();
        activeDocument = Gui::Application::Instance->getDocument(doc);
        activeView = static_cast<Gui::ViewProviderDocumentObject *>(activeDocument->getViewProvider(activeObject));
        attachDocument(doc);
    }
    else {
        activeObject = 0;
        activeView = 0;
    }
}

Fem::FemAnalysis* ActiveAnalysisObserver::getActiveObject() const
{
    return activeObject;
}

bool ActiveAnalysisObserver::hasActiveObject() const
{
    return activeObject != 0;
}

void ActiveAnalysisObserver::highlightActiveObject(const Gui::HighlightMode& mode, bool on)
{
    if (activeDocument && activeView)
        activeDocument->signalHighlightObject(*activeView, mode, on);
}

void ActiveAnalysisObserver::slotCreatedDocument(const App::Document& Doc)
{
}

void ActiveAnalysisObserver::slotDeletedDocument(const App::Document& Doc)
{
    App::Document* d = getDocument();
    if (d == &Doc) {
        activeObject = 0;
        activeDocument = 0;
        activeView = 0;
        detachDocument();
    }
}

void ActiveAnalysisObserver::slotCreatedObject(const App::DocumentObject& Obj)
{
}

void ActiveAnalysisObserver::slotDeletedObject(const App::DocumentObject& Obj)
{
    if (activeObject == &Obj) {
        activeObject = 0;
        activeView = 0;
    }
}

void ActiveAnalysisObserver::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
}
