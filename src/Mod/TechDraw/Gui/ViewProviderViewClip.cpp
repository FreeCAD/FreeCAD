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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
#endif

#include <App/DocumentObject.h>
#include "ViewProviderViewClip.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderViewClip, TechDrawGui::ViewProviderDrawingView)

ViewProviderViewClip::ViewProviderViewClip()
{
    sPixmap = "actions/TechDraw_ClipGroup";

    // Do not show in property editor   why? wf  WF: because DisplayMode applies only to coin and we
    // don't use coin.
    DisplayMode.setStatus(App::Property::Hidden, true);
}

ViewProviderViewClip::~ViewProviderViewClip()
{
}

std::vector<App::DocumentObject*> ViewProviderViewClip::claimChildren() const
{
    // Collect any child views
    // for Clip, valid children are any View in Views
    const std::vector<App::DocumentObject *> &views = getObject()->Views.getValues();
    return views;
}

void ViewProviderViewClip::show()
{
    //TODO: not sure that clip members need to be touched when hiding clip group
    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring())
        return;
    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it)
            (*it)->touch();
    }
    ViewProviderDrawingView::show();

}

void ViewProviderViewClip::hide()
{
    //TODO: not sure that clip members need to be touched when hiding clip group
    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring())
        return;
    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it)
            (*it)->touch();
    }
    ViewProviderDrawingView::hide();
}

bool ViewProviderViewClip::canDelete(App::DocumentObject *obj) const
{
    // deletions of Clip objects don't destroy anything
    // thus we can pass this action
    Q_UNUSED(obj)
    return true;
}

TechDraw::DrawViewClip* ViewProviderViewClip::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewClip*>(pcObject);
}

TechDraw::DrawViewClip* ViewProviderViewClip::getObject() const
{
    return getViewObject();
}
