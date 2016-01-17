/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

#include <Base/Console.h>
#include <Base/Exception.h>

#include "DrawPage.h"
#include "DrawViewCollection.h"

using namespace TechDraw;

//===========================================================================
// DrawViewCollection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewCollection, TechDraw::DrawView)

DrawViewCollection::DrawViewCollection()
{
    static const char *group = "Drawing view";
    ADD_PROPERTY_TYPE(Source    ,(0), group, App::Prop_None,"Shape to view");
    ADD_PROPERTY_TYPE(Views     ,(0), group, App::Prop_None,"Attached Views");

}

DrawViewCollection::~DrawViewCollection()
{
}

int DrawViewCollection::addView(DrawView *view)
{
    // Add the new view to the collection
    std::vector<App::DocumentObject *> newViews(Views.getValues());
    newViews.push_back(view);
    Views.setValues(newViews);

    touch();
//TODO: also have to touch the parent page's views to get repaint??
    DrawPage* page = findParentPage();
    if (page) {
        page->Views.touch();
    }
    return Views.getSize();
}

short DrawViewCollection::mustExecute() const
{
    // If Tolerance Property is touched
    if (Views.isTouched() ||
        Source.isTouched()) {
        return 1;
    } else {
        return TechDraw::DrawView::mustExecute();
    }
}

int DrawViewCollection::countChildren()
{
    //Count the children recursively if needed
    int numChildren = 0;

    const std::vector<App::DocumentObject *> &views = Views.getValues();
    for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {

        App::DocumentObject *docObj = dynamic_cast<App::DocumentObject *>(*it);
        if(docObj->getTypeId().isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
            TechDraw::DrawViewCollection *viewCollection = dynamic_cast<TechDraw::DrawViewCollection *>(*it);
            numChildren += viewCollection->countChildren() + 1;
        } else {
            numChildren += 1;
        }
    }
    return numChildren;
}

void DrawViewCollection::onDocumentRestored()
{
    // Rebuild the view
    execute();
}

/// get called by the container when a Property was changed
void DrawViewCollection::onChanged(const App::Property* prop)
{
    TechDraw::DrawView::onChanged(prop);

    if (prop == &Source ||
        prop == &Views){
        if (!isRestoring()) {
            std::vector<App::DocumentObject*> parent = getInList();
            for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
                if ((*it)->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
                    TechDraw::DrawPage *page = static_cast<TechDraw::DrawPage *>(*it);
                    page->Views.touch();
                }
            }
        }
    }
}

App::DocumentObjectExecReturn *DrawViewCollection::execute(void)
{
    if (ScaleType.isValue("Document")) {
        // Recalculate scale
        //Scale.StatusBits.set(App::Prop_ReadOnly);
        Scale.setStatus(App::Property::ReadOnly,true);


        const std::vector<App::DocumentObject *> &views = Views.getValues();
        for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
            App::DocumentObject *docObj = *it;
            if(docObj->getTypeId().isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(*it);

                // Set scale factor of each view
                view->ScaleType.setValue("Document");
                view->touch();
            }
        }
    } else if(strcmp(ScaleType.getValueAsString(), "Custom") == 0) {
        // Rebuild the view
        //Scale.StatusBits.set(App::Prop_ReadOnly, false);
        Scale.setStatus(App::Property::ReadOnly,false);

        const std::vector<App::DocumentObject *> &views = Views.getValues();
        for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
            App::DocumentObject *docObj = *it;
            if(docObj->getTypeId().isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(*it);

                view->ScaleType.setValue("Custom");
                // Set scale factor of each view
                view->Scale.setValue(Scale.getValue());
                view->touch();
            }
        }
    }

    return DrawView::execute();
}
