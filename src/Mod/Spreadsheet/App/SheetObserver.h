// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#pragma once

#include <App/DocumentObserver.h>

namespace Spreadsheet
{

class PropertySheet;

// SheetObserver is obsolete as PropertySheet is now derived from PropertyLinkBase
class SheetObserver: public App::DocumentObserver
{
public:
    SheetObserver(App::Document* document, PropertySheet* _sheet);
    ~SheetObserver() override = default;
    void slotCreatedObject(const App::DocumentObject& Obj) override;
    void slotDeletedObject(const App::DocumentObject& Obj) override;
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) override;
    void ref();
    bool unref();
    App::Document* getDocument() const
    {
        return App::DocumentObserver::getDocument();
    }

private:
    std::set<std::string> isUpdating;
    unsigned int refCount {1};
    PropertySheet* sheet;
};

}  // namespace Spreadsheet
