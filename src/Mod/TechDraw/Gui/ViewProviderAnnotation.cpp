/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <App/DocumentObject.h>
#include "ViewProviderAnnotation.h"
#include "QGIView.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderAnnotation, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderAnnotation::ViewProviderAnnotation()
{
    sPixmap = "actions/techdraw-annotation";
}

ViewProviderAnnotation::~ViewProviderAnnotation()
{
}

void ViewProviderAnnotation::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderAnnotation::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderAnnotation::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

void ViewProviderAnnotation::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->Text)   ||
        prop == &(getViewObject()->Font)   ||
        prop == &(getViewObject()->TextColor)   ||
        prop == &(getViewObject()->TextSize)   ||
        prop == &(getViewObject()->LineSpace)   ||
        prop == &(getViewObject()->TextStyle)   ||
        prop == &(getViewObject()->MaxWidth) ) {
        // redraw QGIVP
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    ViewProviderDrawingView::updateData(prop);
}

TechDraw::DrawViewAnnotation* ViewProviderAnnotation::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewAnnotation*>(pcObject);
}
