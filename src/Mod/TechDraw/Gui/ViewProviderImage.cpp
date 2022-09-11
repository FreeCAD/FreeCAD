/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <App/DocumentObject.h>

#include "ViewProviderImage.h"
#include "QGIView.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderImage, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderImage::ViewProviderImage()
{
    sPixmap = "actions/TechDraw_Image";

    ADD_PROPERTY_TYPE(Crop ,(false), "Image", App::Prop_None, "Crop image to Width x Height");
}

ViewProviderImage::~ViewProviderImage()
{
}

void ViewProviderImage::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->Width)  ||
        prop == &(getViewObject()->Height)  ||
        prop == &(getViewObject()->Scale) ) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->QGIView::updateView(true);
        }
    }

    ViewProviderDrawingView::updateData(prop);
}

void ViewProviderImage::onChanged(const App::Property *prop)
{
    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
            Gui::ViewProviderDocumentObject::onChanged(prop);
            return;
    }

    if (prop == &Crop) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    Gui::ViewProviderDocumentObject::onChanged(prop);
}


TechDraw::DrawViewImage* ViewProviderImage::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewImage*>(pcObject);
}


