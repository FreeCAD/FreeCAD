// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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


#ifndef APP_TEXTDOCUMENT_H
#define APP_TEXTDOCUMENT_H

#include "DocumentObject.h"
#include "PropertyStandard.h"


namespace App
{

class AppExport TextDocument: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::TextDocument);

public:
    using TextSignal = fastsignals::signal<void()>;
    using TextSlot = TextSignal::slot_type;

    PropertyString Text;

    TextDocument();
    ~TextDocument() override = default;

    void onChanged(const Property* prop) override;
    const char* getViewProviderName() const override;

    fastsignals::advanced_connection connectText(const TextSlot& sub);
    fastsignals::connection connectLabel(const TextSlot& sub);

private:
    TextSignal textChanged;
    TextSignal labelChanged;
};

}  // namespace App


#endif
