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

#include "MeasureDistance.h"

using namespace App;

PROPERTY_SOURCE(App::MeasureDistance, App::DocumentObject)


MeasureDistance::MeasureDistance()
{
    ADD_PROPERTY_TYPE(P1,(Base::Vector3d()),"Measurement",Prop_None,"First point of measurement");
    ADD_PROPERTY_TYPE(P2,(Base::Vector3d()),"Measurement",Prop_None,"Second point of measurement");
    ADD_PROPERTY_TYPE(Distance,(0.0)       ,"Measurement",App::PropertyType(Prop_ReadOnly|Prop_Output),
                                            "Distance between the points");

}

MeasureDistance::~MeasureDistance() = default;

DocumentObjectExecReturn *MeasureDistance::execute()
{
    Distance.setValue(Base::Distance(P1.getValue(), P2.getValue()));
    return DocumentObject::StdReturn;
}

void MeasureDistance::onChanged(const App::Property* prop)
{
    if (prop == &P1 || prop == &P2) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn *ret = recompute();
            delete ret;
        }
    }
    DocumentObject::onChanged(prop);
}
