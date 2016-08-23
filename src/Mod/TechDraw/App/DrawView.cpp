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
#include <App/Application.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>

#include "DrawView.h"
#include "DrawPage.h"
#include "DrawViewCollection.h"
#include "DrawViewClip.h"

#include <Mod/TechDraw/App/DrawViewPy.h>  // generated from DrawViewPy.xml

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
  : autoPos(true)
{
    static const char *group = "Drawing view";
    ADD_PROPERTY_TYPE(X ,(0),group,App::Prop_None,"X position of the view on the page in modelling units (mm)");
    ADD_PROPERTY_TYPE(Y ,(0),group,App::Prop_None,"Y position of the view on the page in modelling units (mm)");
    ADD_PROPERTY_TYPE(Rotation ,(0),group,App::Prop_None,"Rotation of the view on the page in degrees counterclockwise");

    ScaleType.setEnums(ScaleTypeEnums);
    ADD_PROPERTY_TYPE(ScaleType,((long)0),group, App::Prop_None, "Scale Type");
    ADD_PROPERTY_TYPE(Scale ,(1.0),group,App::Prop_None,"Scale factor of the view");

    if (isRestoring()) {
        autoPos = false;
    }
}

DrawView::~DrawView()
{
}

App::DocumentObjectExecReturn *DrawView::execute(void)
{
    TechDraw::DrawPage *page = findParentPage();
    if(page) {
        if (ScaleType.isValue("Document")) {
            if(std::abs(page->Scale.getValue() - Scale.getValue()) > FLT_EPSILON) {
                Scale.setValue(page->Scale.getValue());
            }
        } else if (ScaleType.isValue("Automatic")) {
            //check fit. if too big, rescale
            if (!checkFit(page)) {
                double newScale = autoScale(page->getPageWidth(),page->getPageHeight());
                if(std::abs(newScale - Scale.getValue()) > FLT_EPSILON) {           //stops onChanged/execute loop
                    Scale.setValue(newScale);
                }
            }
        }
    }
    return App::DocumentObject::execute();
}

void DrawView::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Scale) {
            execute();
        } else if (prop == &ScaleType) {
            if (ScaleType.isValue("Document")) {
                Scale.setStatus(App::Property::ReadOnly,true);
                App::GetApplication().signalChangePropertyEditor(Scale);
            } else if ( ScaleType.isValue("Custom") ) {
                Scale.setStatus(App::Property::ReadOnly,false);
                App::GetApplication().signalChangePropertyEditor(Scale);
            } else if ( ScaleType.isValue("Automatic") ) {
                Scale.setStatus(App::Property::ReadOnly,true);
                App::GetApplication().signalChangePropertyEditor(Scale);
            }
            execute();
        } else if (prop == &X ||
                   prop == &Y) {
            setAutoPos(false);
            execute();
        } else if (prop == &Rotation) {
            execute();
        }
    }

    App::DocumentObject::onChanged(prop);
}

////you must override this in derived class
QRectF DrawView::getRect() const
{
    QRectF result(0,0,1,1);
    return result;
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

double DrawView::autoScale(double w, double h) const
{
    double fudgeFactor = 0.90;
    QRectF viewBox = getRect();
    double xScale = w/viewBox.width();
    double yScale = h/viewBox.height();
    //find a standard scale that's close? 1:2, 1:10, 1:100...?
    double newScale = fudgeFactor * std::min(xScale,yScale);
    return newScale;
}

//!check if View fits on Page
bool DrawView::checkFit(TechDraw::DrawPage* p) const
{
    bool result = true;
    QRectF viewBox = getRect();
    if ( (viewBox.width() > p->getPageWidth()) ||
         (viewBox.height() > p->getPageHeight()) ) {
        result = false;
    }
    return result;
}

PyObject *DrawView::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPython, TechDraw::DrawView)
template<> const char* TechDraw::DrawViewPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawView>;
}
