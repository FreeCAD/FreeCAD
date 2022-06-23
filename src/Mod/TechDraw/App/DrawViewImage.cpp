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
#include <App/Document.h>

#include "DrawUtil.h"
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
    ADD_PROPERTY_TYPE(ImageIncluded, (""), vgroup,App::Prop_None,
                                            "Embedded image file. System use only.");   // n/a to end users
    ADD_PROPERTY_TYPE(Width      ,(100),vgroup,App::Prop_None,"The width of cropped image");
    ADD_PROPERTY_TYPE(Height     ,(100),vgroup,App::Prop_None,"The height of cropped image");
    ScaleType.setValue("Custom");

    std::string imgFilter("Image files (*.jpg *.jpeg *.png);;All files (*)");
    ImageFile.setFilter(imgFilter);
}

DrawViewImage::~DrawViewImage()
{
}

void DrawViewImage::onChanged(const App::Property* prop)
{
    App::Document* doc = getDocument();
    if (!isRestoring()) {
        if ((prop == &ImageFile) &&
            (doc != nullptr) ) {
            if (!ImageFile.isEmpty()) {
                replaceImageIncluded(ImageFile.getValue());
            }
            requestPaint();
        } else if (prop == &Scale) {
            requestPaint();
        }
    }

    TechDraw::DrawView::onChanged(prop);
}

short DrawViewImage::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Height.isTouched() ||
                    Width.isTouched());
    }

    if (result) {
        return result;
    }
    return App::DocumentObject::mustExecute();
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

void DrawViewImage::replaceImageIncluded(std::string newFileName)
{
    Base::Console().Message("DVI::replaceImageIncluded(%s)\n", newFileName.c_str());
    if (ImageIncluded.isEmpty()) {
        setupImageIncluded();
    } else {
        std::string tempName = ImageIncluded.getExchangeTempFile();
        DrawUtil::copyFile(newFileName, tempName);
        ImageIncluded.setValue(tempName.c_str());
    }
}

void DrawViewImage::setupImageIncluded(void)
{
    Base::Console().Message("DVI::setupImageIncluded()\n");
    App::Document* doc = getDocument();
    std::string dir = doc->TransientDir.getValue();
    std::string special = getNameInDocument();
    special += "Image.bitmap";
    std::string imageName = dir + "/" + special;

    //setupImageIncluded is only called when ImageIncluded is empty, so
    //just set up the empty file first
    DrawUtil::copyFile(std::string(), imageName);
    ImageIncluded.setValue(imageName.c_str());

    if (!ImageFile.isEmpty()) {
        Base::FileInfo fi(ImageFile.getValue());
        if (fi.isReadable()) {
            std::string exchName = ImageIncluded.getExchangeTempFile();
            DrawUtil::copyFile(ImageFile.getValue(), exchName);
            ImageIncluded.setValue(exchName.c_str(), special.c_str());
        }
    }
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
