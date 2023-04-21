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

#include <App/DocumentObject.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>

#include "QGIView.h"
#include "ViewProviderAnnotation.h"


using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderAnnotation, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderAnnotation::ViewProviderAnnotation() { sPixmap = "actions/TechDraw_Annotation"; }

ViewProviderAnnotation::~ViewProviderAnnotation() {}

void ViewProviderAnnotation::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->Text) || prop == &(getViewObject()->Font)
        || prop == &(getViewObject()->TextColor) || prop == &(getViewObject()->TextSize)
        || prop == &(getViewObject()->LineSpace) || prop == &(getViewObject()->TextStyle)
        || prop == &(getViewObject()->MaxWidth)) {
        // redraw QGIVP
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    ViewProviderDrawingView::updateData(prop);
}

std::vector<App::DocumentObject*> ViewProviderAnnotation::claimChildren() const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of an Annotation are:
    //    - Balloons
    //    - Leaders
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject*>& views = getViewObject()->getInList();
    try {
        for (std::vector<App::DocumentObject*>::const_iterator it = views.begin();
             it != views.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId())) {
                temp.push_back((*it));
            }
            else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
                temp.push_back((*it));
            }
        }
        return temp;
    }
    catch (...) {
        return std::vector<App::DocumentObject*>();
    }
}

TechDraw::DrawViewAnnotation* ViewProviderAnnotation::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewAnnotation*>(pcObject);
}
