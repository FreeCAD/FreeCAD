/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "DrawViewCollection.h"


using namespace TechDraw;

//===========================================================================
// DrawViewCollection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewCollection, TechDraw::DrawView)

DrawViewCollection::DrawViewCollection()
{
    nowUnsetting = false;
    static const char *group = "Collection";
    ADD_PROPERTY_TYPE(Views     ,(nullptr), group, App::Prop_None, "Collection Views");
    Views.setScope(App::LinkScope::Global);
}

DrawViewCollection::~DrawViewCollection()
{
}

void DrawViewCollection::onChanged(const App::Property* prop)
{
    TechDraw::DrawView::onChanged(prop);
}

short DrawViewCollection::mustExecute() const
{
    if (Views.isTouched()) {
        return 1;
    } else {
        return TechDraw::DrawView::mustExecute();
    }
}

App::DocumentObjectExecReturn *DrawViewCollection::execute()
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    lockChildren();

    overrideKeepUpdated(false);
    return DrawView::execute();
}

int DrawViewCollection::addView(App::DocumentObject* docObj)
{
    // Add the new view to the collection

    if (!docObj->isDerivedFrom<DrawView>()
        && !docObj->isDerivedFrom<App::Link>()) {
        return -1;
    }

    auto* view = freecad_cast<DrawView*>(docObj);

    if (!view) {
        auto* link = dynamic_cast<App::Link*>(docObj);
        if (!link) {
            return -1;
        }

        if (link) {
            view = freecad_cast<DrawView*>(link->getLinkedObject());
            if (!view) {
                return -1;
            }
        }
    }

    std::vector<App::DocumentObject*> newViews(Views.getValues());
    newViews.push_back(docObj);
    Views.setValues(newViews);

    return Views.getSize();
}

int DrawViewCollection::removeView(App::DocumentObject* docObj)
{
    // Remove the view from the collection
    std::vector<App::DocumentObject*> newViews;
    std::string viewName = docObj->getNameInDocument();
    for (auto* view : Views.getValues()) {
        if (viewName.compare(view->getNameInDocument()) != 0) {
            newViews.push_back(view);
        }
    }
    Views.setValues(newViews);

    return Views.getSize();
}

std::vector<App::DocumentObject*> DrawViewCollection::getViews() const
{
    std::vector<App::DocumentObject*> views = Views.getValues();
    std::vector<App::DocumentObject*> allViews;
    for (auto& v : views) {
        if (v->isDerivedFrom<App::Link>()) {
            v = static_cast<App::Link*>(v)->getLinkedObject();
        }

        if (!v->isDerivedFrom<TechDraw::DrawView>()) {
            continue;
        }

        allViews.push_back(v);
    }
    return allViews;
}

//make sure everything in View list represents a real DrawView docObj and occurs only once
void DrawViewCollection::rebuildViewList()
{
    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject*> newViews;
    for (auto* child : getOutList()) {
        if (child->isDerivedFrom<DrawView>() ||
            (child->isDerivedFrom<App::Link>()
                && static_cast<App::Link*>(child)->getLinkedObject()->isDerivedFrom<DrawView>())) {
            bool found = false;
            for (auto& v:currViews) {
                if (v == child) {
                    found = true;
                    break;
                }
            }
            if (found) {
                newViews.push_back(child);
            }
        }
    } // newViews contains only valid items, but may have duplicates
    sort( newViews.begin(), newViews.end() );
    newViews.erase( unique( newViews.begin(), newViews.end() ), newViews.end() );
    Views.setValues(newViews);
}

int DrawViewCollection::countChildren()
{
    //Count the children recursively if needed
    int numChildren = 0;

    for(auto* view : Views.getValues()) {
        if(view->isDerivedFrom<DrawViewCollection>()) {
            auto *viewCollection = static_cast<DrawViewCollection *>(view);
            numChildren += viewCollection->countChildren() + 1;
        }
        else {
            numChildren += 1;
        }
    }
    return numChildren;
}

void DrawViewCollection::onDocumentRestored()
{
    DrawView::execute();
}

void DrawViewCollection::lockChildren()
{
    for (auto& v : getViews()) {
        auto *view = dynamic_cast<DrawView *>(v);
        if (!view) {
            throw Base::ValueError("DrawViewCollection::lockChildren bad View\n");
        }
        view->handleXYLock();
    }
}

void DrawViewCollection::unsetupObject()
{
    nowUnsetting = true;

    // Remove the collection's views from document
    std::string docName = getDocument()->getName();

    for (auto* view : Views.getValues()) {
        if (view->isAttachedToDocument()) {
            std::string viewName = view->getNameInDocument();
            Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                              docName.c_str(), viewName.c_str());
        }
    }

    std::vector<App::DocumentObject*> emptyViews;
    Views.setValues(emptyViews);
}

QRectF DrawViewCollection::getRect() const
{
    QRectF result;
    for (auto& v : getViews()) {
        auto *view = freecad_cast<DrawView*>(v);
        if (!view) {
            throw Base::ValueError("DrawViewCollection::getRect bad View\n");
        }

        result = result.united(view->getRect().translated(view->X.getValue(), view->Y.getValue()));
    }
    return { 0, 0, getScale() * result.width(), getScale() * result.height()};
}
