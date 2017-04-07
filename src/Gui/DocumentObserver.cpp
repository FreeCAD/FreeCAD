/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include <boost/signals.hpp>
#include <boost/bind.hpp>

#include "Application.h"
#include "Document.h"
#include "ViewProviderDocumentObject.h"
#include "DocumentObserver.h"

using namespace Gui;


DocumentObserver::DocumentObserver()
{
}

DocumentObserver::~DocumentObserver()
{
}

void DocumentObserver::attachDocument(Document* doc)
{
    detachDocument();

    if (doc == nullptr)
        return;

    this->connectDocumentCreatedObject = doc->signalNewObject.connect(boost::bind
        (&DocumentObserver::slotCreatedObject, this, _1));
    this->connectDocumentDeletedObject = doc->signalDeletedObject.connect(boost::bind
        (&DocumentObserver::slotDeletedObject, this, _1));
    this->connectDocumentChangedObject = doc->signalChangedObject.connect(boost::bind
        (&DocumentObserver::slotChangedObject, this, _1, _2));
    this->connectDocumentRelabelObject = doc->signalRelabelObject.connect(boost::bind
        (&DocumentObserver::slotRelabelObject, this, _1));
    this->connectDocumentActivateObject = doc->signalActivatedObject.connect(boost::bind
        (&DocumentObserver::slotActivatedObject, this, _1));
    this->connectDocumentEditObject = doc->signalInEdit.connect(boost::bind
        (&DocumentObserver::slotEnterEditObject, this, _1));
    this->connectDocumentResetObject = doc->signalResetEdit.connect(boost::bind
        (&DocumentObserver::slotResetEditObject, this, _1));
    this->connectDocumentUndo = doc->signalUndoDocument.connect(boost::bind
        (&DocumentObserver::slotUndoDocument, this, _1));
    this->connectDocumentRedo = doc->signalRedoDocument.connect(boost::bind
        (&DocumentObserver::slotRedoDocument, this, _1));
    this->connectDocumentDelete = doc->signalDeleteDocument.connect(boost::bind
        (&DocumentObserver::slotDeleteDocument, this, _1));
}

void DocumentObserver::detachDocument()
{
    this->connectDocumentCreatedObject.disconnect();
    this->connectDocumentDeletedObject.disconnect();
    this->connectDocumentChangedObject.disconnect();
    this->connectDocumentRelabelObject.disconnect();
    this->connectDocumentActivateObject.disconnect();
    this->connectDocumentEditObject.disconnect();
    this->connectDocumentResetObject.disconnect();
    this->connectDocumentUndo.disconnect();
    this->connectDocumentRedo.disconnect();
    this->connectDocumentDelete.disconnect();
}

void DocumentObserver::enableNotifications(DocumentObserver::Notifications value)
{
    this->connectDocumentCreatedObject.block(!(value & Create));
    this->connectDocumentDeletedObject.block(!(value & Delete));
    this->connectDocumentChangedObject.block(!(value & Change));
    this->connectDocumentRelabelObject.block(!(value & Relabel));
    this->connectDocumentActivateObject.block(!(value & Activate));
    this->connectDocumentEditObject.block(!(value & Edit));
    this->connectDocumentResetObject.block(!(value & Reset));
    this->connectDocumentUndo.block(!(value & Undo));
    this->connectDocumentRedo.block(!(value & Redo));
}

void DocumentObserver::slotUndoDocument(const Document& /*Doc*/)
{
}

void DocumentObserver::slotRedoDocument(const Document& /*Doc*/)
{
}

void DocumentObserver::slotDeleteDocument(const Document& /*Doc*/)
{
}

void DocumentObserver::slotCreatedObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotDeletedObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotChangedObject(const ViewProviderDocumentObject& /*Obj*/,
                                         const App::Property& /*Prop*/)
{
}

void DocumentObserver::slotRelabelObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotActivatedObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotEnterEditObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotResetEditObject(const ViewProviderDocumentObject& /*Obj*/)
{
}
