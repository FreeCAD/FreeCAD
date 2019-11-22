/***************************************************************************
 *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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


#ifndef APP_TEXTDOCUMENT_H
#define APP_TEXTDOCUMENT_H


#include <string>
#include <boost/signals2.hpp>

#include "DocumentObject.h"
#include "PropertyStandard.h"


namespace App
{

class AppExport TextDocument : public App::DocumentObject {
    PROPERTY_HEADER(App::TextDocument);
public:
    using TextSignal = boost::signals2::signal<void ()>;
    using TextSlot = TextSignal::slot_type;

    PropertyString Text;
    PropertyBool ReadOnly;

    TextDocument();
    ~TextDocument() {};

    void onChanged(const Property* prop);
    const char* getViewProviderName() const;

    boost::signals2::connection connect(const TextSlot &sub);
private:
    TextSignal textChanged;
};

}


#endif
