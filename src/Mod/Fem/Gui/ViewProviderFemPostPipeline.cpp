/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include "ViewProviderFemPostPipeline.h"
#include <Mod/Fem/App/FemPostPipeline.h>
#include <Base/Console.h>

using namespace FemGui;


PROPERTY_SOURCE(FemGui::ViewProviderFemPostPipeline, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostPipeline::ViewProviderFemPostPipeline()
{
}

ViewProviderFemPostPipeline::~ViewProviderFemPostPipeline()
{
}

std::vector< App::DocumentObject* > ViewProviderFemPostPipeline::claimChildren(void) const {

    Fem::FemPostPipeline* pipeline = static_cast<Fem::FemPostPipeline*>(getObject());
    std::vector<App::DocumentObject*> children;
    
    if(pipeline->Function.getValue())
        children.push_back(pipeline->Function.getValue());
    
    children.insert(children.end(), pipeline->Filter.getValues().begin(), pipeline->Filter.getValues().end());   
    Base::Console().Message("claim children pipeline: %i\n", children.size());
    return children;
}

std::vector< App::DocumentObject* > ViewProviderFemPostPipeline::claimChildren3D(void) const {

    return claimChildren();
}

