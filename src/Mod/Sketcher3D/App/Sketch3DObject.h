// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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


#ifndef SKETCHER3D_SKETCH3DOBJECT_H
#define SKETCHER3D_SKETCH3DOBJECT_H

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include "PropertyConstraint3DList.h"


namespace Sketcher3D
{

/** 3D sketch document object.
 *  Inherited Part::Feature so downstream part operations can use 
 *  it directly.
 */
class Sketcher3DExport Sketch3DObject: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Sketcher3D::Sketch3DObject);

public:
    Sketch3DObject();
    ~Sketch3DObject() override;

    PropertyConstraint3DList Constraints;

    const char* getViewProviderName() const override
    {
        return "Sketcher3DGui::ViewProviderSketch3D";
    }

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
};

}  // namespace Sketcher3D

#endif  // SKETCHER3D_SKETCH3DOBJECT_H
