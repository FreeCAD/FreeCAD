/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <strstream>
#include <Base/Console.h>
#include <Base/Writer.h>

#include "DrawProjGroupItem.h"

using namespace TechDraw;

const char* DrawProjGroupItem::TypeEnums[] = {"Front",
                                             "Left",
                                             "Right",
                                             "Rear",
                                             "Top",
                                             "Bottom",
                                             "FrontTopLeft",
                                             "FrontTopRight",
                                             "FrontBottomLeft",
                                             "FrontBottomRight",
                                             NULL};


PROPERTY_SOURCE(TechDraw::DrawProjGroupItem, TechDraw::DrawViewPart)

DrawProjGroupItem::DrawProjGroupItem(void)
{
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Type, ((long)0));

    // Set Hidden
    //Direction.StatusBits.set(3);
    Direction.setStatus(App::Property::Hidden,true);

    // Set Hidden
    //XAxisDirection.StatusBits.set(3);
    XAxisDirection.setStatus(App::Property::Hidden,true);

    // Scale and ScaleType are Readonly
    //Scale.StatusBits.set(2);
    //ScaleType.StatusBits.set(2);
    Scale.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::ReadOnly,true);
}

short DrawProjGroupItem::mustExecute() const
{
    if (Type.isTouched())
        return 1;
    return TechDraw::DrawViewPart::mustExecute();
}

void DrawProjGroupItem::onChanged(const App::Property *prop)
{
    TechDraw::DrawViewPart::onChanged(prop);

    //TODO: Should we allow changes to the Type here?  Seems that should be handled through DrawProjGroup
    if (prop == &Type && Type.isTouched()) {
        if (!isRestoring()) {
            execute();
        }
    }

}

DrawProjGroupItem::~DrawProjGroupItem()
{
}

void DrawProjGroupItem::onDocumentRestored()
{
    // Rebuild the view
    execute();
}

/*
//TODO: Perhaps we don't need this anymore?
App::DocumentObjectExecReturn *DrawProjGroupItem::execute(void)
{
    if(Type.isTouched()) {
        Type.purgeTouched();
    }

    return TechDraw::DrawViewPart::execute();
}*/

