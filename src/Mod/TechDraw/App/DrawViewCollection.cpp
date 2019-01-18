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

#include <App/Document.h>

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
    nowUnsetting = false;
    static const char *group = "Drawing view";
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

    return Views.getSize();
}

int DrawViewCollection::removeView(DrawView *view)
{
    // Remove the view from the collection
    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject*> newViews;
    std::vector<App::DocumentObject*>::const_iterator it = currViews.begin();
    for (; it != currViews.end(); it++) {
        std::string viewName = view->getNameInDocument();
        if (viewName.compare((*it)->getNameInDocument()) != 0) {
            newViews.push_back((*it));
        }
    }
    Views.setValues(newViews);

    return Views.getSize();
}

//make sure everything in View list represents a real DrawView docObj and occurs only once
void DrawViewCollection::rebuildViewList()
{
    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject*> newViews;
    std::vector<App::DocumentObject*> children = getOutList();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawView::getClassTypeId())) {
            bool found = false;
            for (auto& v:currViews) {
                if (v == (*it)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                newViews.push_back((*it));
            }
        }
    } // newViews contains only valid items, but may have duplicates
    sort( newViews.begin(), newViews.end() );
    newViews.erase( unique( newViews.begin(), newViews.end() ), newViews.end() );
    Views.setValues(newViews);
}

short DrawViewCollection::mustExecute() const
{
    if (Views.isTouched()) {
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

        if((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
            TechDraw::DrawViewCollection *viewCollection = static_cast<TechDraw::DrawViewCollection *>(*it);
            numChildren += viewCollection->countChildren() + 1;
        } else {
            numChildren += 1;
        }
    }
    return numChildren;
}

void DrawViewCollection::onDocumentRestored()
{
    DrawView::execute();
}

void DrawViewCollection::onChanged(const App::Property* prop)
{
    TechDraw::DrawView::onChanged(prop);
}

void DrawViewCollection::unsetupObject()
{
    nowUnsetting = true;

    // Remove the collection's views from document
    App::Document* doc = getDocument();
    std::string docName = doc->getName();

    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject*> emptyViews;
    std::vector<App::DocumentObject*>::const_iterator it = currViews.begin();
    for (; it != currViews.end(); it++) {
        std::string viewName = (*it)->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                          docName.c_str(), viewName.c_str());
    }
    Views.setValues(emptyViews);
}


App::DocumentObjectExecReturn *DrawViewCollection::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    return DrawView::execute();
}


QRectF DrawViewCollection::getRect() const
{
    QRectF result;
    for (auto& v:Views.getValues()) {
        TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(v);
        if (!view) {
            throw Base::ValueError("DrawViewCollection::getRect bad View\n");
        }

        result = result.united(view->getRect().translated(view->X.getValue(),view->Y.getValue()));
    }
    return QRectF(0,0,getScale() * result.width(),getScale() * result.height());
}
