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
# include <Standard_Failure.hxx>
#endif


#include <strstream>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>

#include "DrawView.h"
#include "DrawPage.h"
#include "DrawViewCollection.h"
#include "DrawViewClip.h"

using namespace TechDraw;


//===========================================================================
// DrawView
//===========================================================================

const char* DrawView::ScaleTypeEnums[]= {"Document",
                                            "Automatic",
                                            "Custom",
                                             NULL};

PROPERTY_SOURCE(TechDraw::DrawView, App::DocumentObject)



DrawView::DrawView(void)
{
    static const char *group = "Drawing view";
    ADD_PROPERTY_TYPE(X ,(0),group,App::Prop_None,"X position of the view on the page in modelling units (mm)");
    ADD_PROPERTY_TYPE(Y ,(0),group,App::Prop_None,"Y position of the view on the page in modelling units (mm)");
    ADD_PROPERTY_TYPE(Scale ,(1.0),group,App::Prop_None,"Scale factor of the view");
    ADD_PROPERTY_TYPE(Rotation ,(0),group,App::Prop_None,"Rotation of the view on the page in degrees counterclockwise");

    // The 'Visible' property is handled by the view provider exclusively. It has the 'Output' flag set to
    // avoid to call the execute() method. The view provider touches the page object, instead.
    App::PropertyType propType = static_cast<App::PropertyType>(App::Prop_Hidden|App::Prop_Output);
    ADD_PROPERTY_TYPE(Visible, (true),group,propType,"Control whether view is visible in page object");

    ScaleType.setEnums(ScaleTypeEnums);
    ADD_PROPERTY_TYPE(ScaleType,((long)0),group, App::Prop_None, "Scale Type");
}

DrawView::~DrawView()
{
}

App::DocumentObjectExecReturn *DrawView::recompute(void)
{
    try {
        return App::DocumentObject::recompute();
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        App::DocumentObjectExecReturn* ret = new App::DocumentObjectExecReturn(e->GetMessageString());
        if (ret->Why.empty()) ret->Why = "Unknown OCC exception";
        return ret;
    }
}

App::DocumentObjectExecReturn *DrawView::execute(void)
{
    if (ScaleType.isValue("Document")) {
        Scale.setStatus(App::Property::ReadOnly,true);
        //Scale.StatusBits.set(2, true);

        TechDraw::DrawPage *page = findParentPage();
        if(page) {
            if(std::abs(page->Scale.getValue() - Scale.getValue()) > FLT_EPSILON) {
                Scale.setValue(page->Scale.getValue()); // Recalculate scale from page
                Scale.touch();
            }
        }
    } else if (ScaleType.isValue("Custom")) {
        Scale.setStatus(App::Property::ReadOnly,false);
        //Scale.StatusBits.set(2, false);
        //TODO: need to ?recompute? ?redraw? to get this to stick.  Mantis #1941
        //TODO: try Gui::Selection to force update
        //currently need to lose focus and re-get focus to make Scale editable.
        //Scale.touch();                     // causes loop
    }
    return App::DocumentObject::execute();
}

/// get called by the container when a Property was changed
void DrawView::onChanged(const App::Property* prop)
{
    if (prop == &X ||
        prop == &Y ||
        prop == &ScaleType ||
        prop == &Rotation) {
          if (!isRestoring()) {
              DrawView::execute();
          }
    }

    App::DocumentObject::onChanged(prop);
}

void DrawView::onDocumentRestored()
{
    // Rebuild the view
    execute();
}

DrawPage* DrawView::findParentPage() const
{
    // Get Feature Page
    DrawPage *page = 0;
    DrawViewCollection *collection = 0;
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
            page = dynamic_cast<TechDraw::DrawPage *>(*it);
        }

        if ((*it)->getTypeId().isDerivedFrom(DrawViewCollection::getClassTypeId())) {
            collection = dynamic_cast<TechDraw::DrawViewCollection *>(*it);
            page = collection->findParentPage();
        }

        if(page)
          break; // Found page so leave
    }

    return page;
}

bool DrawView::isInClip()
{
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewClip::getClassTypeId())) {
            return true;
        }
    }
    return false;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPython, TechDraw::DrawView)
template<> const char* TechDraw::DrawViewPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawView>;
}
