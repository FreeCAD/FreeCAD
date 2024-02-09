/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PropertyVarSet.h"
#include "VarSet.h"
#include "Base/Console.h"

using namespace App;



FC_LOG_LEVEL_INIT("VarSet", true, true)


TYPESYSTEM_SOURCE(App::PropertyVarSet , App::PropertyXLink)

PropertyVarSet::PropertyVarSet() = default;

void PropertyVarSet::setValue(DocumentObject* obj)
{
    if (obj && obj->isDerivedFrom<VarSet>()) {
        auto varSet = dynamic_cast<VarSet*>(obj);
        PropertyXLink::setValue(obj);
    }
}

VarSet* PropertyVarSet::getValue() const {
    return dynamic_cast<App::VarSet*>(PropertyXLink::getValue());
}
