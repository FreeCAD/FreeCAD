/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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
#include "SheetObserver.h"
#include "PropertySheet.h"

using namespace Spreadsheet;
using namespace App;

/**
  * The SheetObserver constructor.
  *
  * @param document The Document we are observing
  * @param _sheet   The sheet owning this observer.
  *
  */

SheetObserver::SheetObserver(App::Document * document, PropertySheet *_sheet)
    : DocumentObserver(document)
    , sheet(_sheet)
    , refCount(1)
{
}

/**
  * Unimplemented.
  *
  */

void SheetObserver::slotCreatedDocument(const App::Document &Doc)
{
}

/**
  * Unimplemented.
  *
  */

void SheetObserver::slotDeletedDocument(const App::Document &Doc)
{
}

/**
  * Invalidate cells that depend on this document object.
  *
  */

void SheetObserver::slotCreatedObject(const DocumentObject &Obj)
{
    sheet->invalidateDependants(&Obj);
}

/**
  * Invalidate cells that depend on this document object.
  *
  */

void SheetObserver::slotDeletedObject(const DocumentObject &Obj)
{
    sheet->invalidateDependants(&Obj);
}

/**
  * Invoke the sheets recomputeDependants when a change to a Property occurs.
  *
  */

void SheetObserver::slotChangedObject(const DocumentObject &Obj, const Property &Prop)
{
    const char * name = Obj.getPropertyName(&Prop);
    assert(name != 0);

    if (&Prop == &Obj.Label)
        sheet->renamedDocumentObject(&Obj);
    else {

        if (isUpdating.find(name) != isUpdating.end())
            return;

        isUpdating.insert(name);
        sheet->recomputeDependants(&Prop);
        isUpdating.erase(name);
    }
}

/**
  * Increase reference count.
  *
  */

void SheetObserver::ref()
{
    refCount++;
}

/**
  * Decrease reference count.
  *
  */

bool SheetObserver::unref()
{
    refCount--;
    return refCount;
}

