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
# include <sstream>
#endif

#include <iomanip>
#include <iterator>
#include <boost/regex.hpp>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>

#include "DrawViewImage.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewImage
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewImage, TechDraw::DrawView)


DrawViewImage::DrawViewImage(void)
{
    static const char *vgroup = "Image";

    ADD_PROPERTY_TYPE(ImageFile,(""),vgroup,App::Prop_None,"The file containing this bitmap");
    ADD_PROPERTY_TYPE(Width      ,(100),vgroup,App::Prop_None,"The width of the image view");
    ADD_PROPERTY_TYPE(Height     ,(100),vgroup,App::Prop_None,"The height of the view");
    ScaleType.setValue("Custom");

    std::string imgFilter("Image files (*.jpg *.jpeg *.png);;All files (*)");
    ImageFile.setFilter(imgFilter);
}

DrawViewImage::~DrawViewImage()
{
}

void DrawViewImage::onChanged(const App::Property* prop)
{
    if (prop == &ImageFile) {
        if (!isRestoring()) {
            requestPaint();
        }
    }
    TechDraw::DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewImage::execute(void)
{
    requestPaint();
    return DrawView::execute();
}

QRectF DrawViewImage::getRect() const
{
    QRectF result(0.0,0.0,Width.getValue(),Height.getValue());
    return result;
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewImagePython, TechDraw::DrawViewImage)
template<> const char* TechDraw::DrawViewImagePython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderImage";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewImage>;
}
