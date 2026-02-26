/***************************************************************************
 *   Copyright (c) 2012 Yorik van Havre <yorik@uncreated.net>              *
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#pragma once

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"

// ?? this (and DrawViewCollection) could use App::GroupExtension instead??

namespace TechDraw
{

class TechDrawExport DrawViewClip: public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewClip);

public:
    /// Constructor
    DrawViewClip();
    ~DrawViewClip() override = default;

    App::PropertyLength Width;
    App::PropertyLength Height;
    App::PropertyBool ShowFrame;
    App::PropertyLinkList Views;

    void addView(App::DocumentObject* docObj);
    void removeView(App::DocumentObject* docObj);
    short mustExecute() const override;

    std::vector<App::DocumentObject*> getViews() const;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderViewClip";
    }
    //return PyObject as DrawViewClipPy
    PyObject *getPyObject() override;

    std::vector<std::string> getChildViewNames();
    bool isViewInClip(App::DocumentObject* view);
    QRectF getRect() const override { return { 0, 0, Width.getValue(), Height.getValue() };  }


protected:
    void onChanged(const App::Property* prop) override;
};

using DrawViewClipPython = App::FeaturePythonT<DrawViewClip>;

} //namespace TechDraw