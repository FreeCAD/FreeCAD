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
#endif

#include "ViewProviderBoolean.h"
#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/FeaturePartBoolean.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/FeaturePartCommon.h>

using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderBoolean,PartGui::ViewProviderPart)

ViewProviderBoolean::ViewProviderBoolean()
{
}

ViewProviderBoolean::~ViewProviderBoolean()
{
}

std::vector<App::DocumentObject*> ViewProviderBoolean::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<Part::Boolean*>(getObject())->Base.getValue());
    temp.push_back(static_cast<Part::Boolean*>(getObject())->Tool.getValue());

    return temp;
}

QIcon ViewProviderBoolean::getIcon(void) const
{
    App::DocumentObject* obj = getObject();
    if (obj) {
        Base::Type type = obj->getTypeId();
        if (type == Base::Type::fromName("Part::Common"))
            return Gui::BitmapFactory().pixmap("Part_Common");
        else if (type == Base::Type::fromName("Part::Fuse"))
            return Gui::BitmapFactory().pixmap("Part_Fuse");
        else if (type == Base::Type::fromName("Part::Cut"))
            return Gui::BitmapFactory().pixmap("Part_Cut");
        else if (type == Base::Type::fromName("Part::Section"))
            return Gui::BitmapFactory().pixmap("Part_Section");
    }

    return ViewProviderPart::getIcon();
}


PROPERTY_SOURCE(PartGui::ViewProviderMultiFuse,PartGui::ViewProviderPart)

ViewProviderMultiFuse::ViewProviderMultiFuse()
{
}

ViewProviderMultiFuse::~ViewProviderMultiFuse()
{
}

std::vector<App::DocumentObject*> ViewProviderMultiFuse::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<Part::MultiFuse*>(getObject())->Shapes.getValues());
}

QIcon ViewProviderMultiFuse::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Fuse");
}


PROPERTY_SOURCE(PartGui::ViewProviderMultiCommon,PartGui::ViewProviderPart)

ViewProviderMultiCommon::ViewProviderMultiCommon()
{
}

ViewProviderMultiCommon::~ViewProviderMultiCommon()
{
}

std::vector<App::DocumentObject*> ViewProviderMultiCommon::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<Part::MultiCommon*>(getObject())->Shapes.getValues());
}

QIcon ViewProviderMultiCommon::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Common");
}
