/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#endif

#include "Body.h"
#include "FeatureSolid.h"
#include <App/Application.h>
#include <App/Document.h>
#include "FeatureSplit.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Solid, Part::Feature)

Solid::Solid()
{
    ADD_PROPERTY_TYPE(Active,(false),0,App::Prop_None,"Make this solid active in the parent split feature");
    ADD_PROPERTY_TYPE(Parent,(0),0,(App::PropertyType)(App::Prop_Hidden|App::Prop_ReadOnly),0);
    Placement.setStatus(App::Property::Hidden,true);
    Placement.setStatus(App::Property::Immutable,true);
}

void Solid::onChanged(const App::Property* prop) {
    if(!isRestoring()) {
        if(prop == &Active) {
            if(Parent.getValue() && !Active.testStatus(App::Property::User3)) {
                auto split = Base::freecad_dynamic_cast<Split>(Parent.getValue());
                if(split)
                    split->updateActiveSolid(this);
            }
        }
    }
    inherited::onChanged(prop);
}
