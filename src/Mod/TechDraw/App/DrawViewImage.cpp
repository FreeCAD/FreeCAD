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
#ifndef _PreComp_
# include <iomanip>
# include <sstream>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>

#include "DrawUtil.h"
#include "DrawViewImage.h"


using namespace TechDraw;

//===========================================================================
// DrawViewImage
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewImage, TechDraw::DrawView)


DrawViewImage::DrawViewImage()
{
    static const char* vgroup = "Image";

    ADD_PROPERTY_TYPE(ImageFile, (""), vgroup, App::Prop_None, "The file containing this bitmap");
    ADD_PROPERTY_TYPE(ImageIncluded, (""), vgroup, App::Prop_None,
                      "Embedded image file. System use only.");// n/a to end users
    ADD_PROPERTY_TYPE(Width, (100), vgroup, App::Prop_None, "The width of cropped image");
    ADD_PROPERTY_TYPE(Height, (100), vgroup, App::Prop_None, "The height of cropped image");

    ScaleType.setValue("Custom");
    Scale.setStatus(App::Property::Hidden, false);
    Scale.setStatus(App::Property::ReadOnly, false);

    std::string imgFilter("Image files (*.jpg *.jpeg *.png *.bmp);;All files (*)");
    ImageFile.setFilter(imgFilter);
}

void DrawViewImage::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        TechDraw::DrawView::onChanged(prop);
        return;
    }

    if (prop == &ImageFile) {
        replaceImageIncluded(ImageFile.getValue());
        requestPaint();
    }

    TechDraw::DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn* DrawViewImage::execute()
{
    requestPaint();
    return DrawView::execute();
}

QRectF DrawViewImage::getRect() const { return {0.0, 0.0, Width.getValue(), Height.getValue()}; }

void DrawViewImage::replaceImageIncluded(std::string newImageFile)
{
    //    Base::Console().Message("DVI::replaceImageIncluded(%s)\n", newImageFile.c_str());
    if (newImageFile.empty()) {
        return;
    }

    Base::FileInfo tfi(newImageFile);
    if (tfi.isReadable()) {
        ImageIncluded.setValue(newImageFile.c_str());
    }
    else {
        throw Base::RuntimeError("Could not read the new image file");
    }
}

void DrawViewImage::setupObject()
{
    //    Base::Console().Message("DVI::setupObject()\n");
    replaceImageIncluded(ImageFile.getValue());
}

// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewImagePython, TechDraw::DrawViewImage)
template<> const char* TechDraw::DrawViewImagePython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderImage";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewImage>;
}// namespace App
