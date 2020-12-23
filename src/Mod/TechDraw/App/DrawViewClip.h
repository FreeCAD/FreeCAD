/***************************************************************************
 *   Copyright (c) Yorik van Havre <yorik@uncreated.net> 2012              *
 *   Copyright (c) WandererFan <wandererfan@gmail.com> 2015                *
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


#ifndef _DrawViewClip_h_
#define _DrawViewClip_h_

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>

#include "DrawView.h"

namespace TechDraw
{

class TechDrawExport DrawViewClip: public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewClip);

public:
    /// Constructor
    DrawViewClip(void);
    virtual ~DrawViewClip();

    App::PropertyLength Width;
    App::PropertyLength Height;
    App::PropertyBool ShowFrame;
    App::PropertyLinkList Views;

    void addView(DrawView *view);
    void removeView(DrawView *view);
    short mustExecute() const;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewClip";
    }
    //return PyObject as DrawViewClipPy
    virtual PyObject *getPyObject(void);

    std::vector<std::string> getChildViewNames();
    bool isViewInClip(App::DocumentObject* view);
    virtual QRectF getRect(void) const { return QRectF(0,0,Width.getValue(),Height.getValue()); }


protected:
    void onChanged(const App::Property* prop);
};

typedef App::FeaturePythonT<DrawViewClip> DrawViewClipPython;

} //namespace TechDraw


#endif
