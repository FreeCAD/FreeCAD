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
#include "ViewProviderFemPostFunction.h"
#include <Mod/Fem/App/FemPostPipeline.h>
#include <Base/Console.h>
#include <Gui/Application.h>

using namespace FemGui;


PROPERTY_SOURCE(FemGui::ViewProviderFemPostPipeline, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostPipeline::ViewProviderFemPostPipeline()
{
    sPixmap = "FEM_PostPipelineFromResult";
}

ViewProviderFemPostPipeline::~ViewProviderFemPostPipeline()
{
}

std::vector< App::DocumentObject* > ViewProviderFemPostPipeline::claimChildren(void) const {

    Fem::FemPostPipeline* pipeline = static_cast<Fem::FemPostPipeline*>(getObject());
    std::vector<App::DocumentObject*> children;

    if(pipeline->Functions.getValue())
        children.push_back(pipeline->Functions.getValue());

    children.insert(children.end(), pipeline->Filter.getValues().begin(), pipeline->Filter.getValues().end());
    return children;
}

std::vector< App::DocumentObject* > ViewProviderFemPostPipeline::claimChildren3D(void) const {

    return claimChildren();
}

void ViewProviderFemPostPipeline::updateData(const App::Property* prop) {
    FemGui::ViewProviderFemPostObject::onChanged(prop);

    if(strcmp(prop->getName(), "Function") == 0) {
        updateFunctionSize();
    }

}

void ViewProviderFemPostPipeline::updateFunctionSize() {

    //we need to get the bounding box and set the function provider size
    Fem::FemPostPipeline* obj = static_cast<Fem::FemPostPipeline*>(getObject());

    if(!obj->Functions.getValue() || !obj->Functions.getValue()->isDerivedFrom(Fem::FemPostFunctionProvider::getClassTypeId()))
        return;

    //get the function provider
    FemGui::ViewProviderFemPostFunctionProvider* vp = static_cast<FemGui::ViewProviderFemPostFunctionProvider*>(
                                                    Gui::Application::Instance->getViewProvider(obj->Functions.getValue()));

    if(obj->Data.getValue() && obj->Data.getValue()->IsA("vtkDataSet")) {
        vtkBoundingBox box = obj->getBoundingBox();

        vp->SizeX.setValue(box.GetLength(0)*1.2);
        vp->SizeY.setValue(box.GetLength(1)*1.2);
        vp->SizeZ.setValue(box.GetLength(2)*1.2);
    }
}
