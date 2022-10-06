/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef TECHDRAW_FEATUREVIEWCOLLECTION_h_
#define TECHDRAW_FEATUREVIEWCOLLECTION_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"


namespace TechDraw
{

/** Base class for collection of view objects
 */
class TechDrawExport DrawViewCollection : public DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewCollection);

public:
    App::PropertyLinkList Views;
public:
    /// Constructor
    DrawViewCollection();
    ~DrawViewCollection() override;
    short mustExecute() const override;

    int addView(DrawView *view);
    int removeView(DrawView *view);
    void rebuildViewList();
    bool isUnsetting() { return nowUnsetting; }

    int countChildren();
    void lockChildren();

    void onDocumentRestored() override;
    App::DocumentObjectExecReturn *execute() override;
    void unsetupObject() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderViewCollection";
    }
    QRectF getRect() const override;

protected:
    void onChanged(const App::Property* prop) override;
    bool nowUnsetting;
};

} //namespace TechDraw

#endif // TECHDRAW_FEATUREVIEWCOLLECTION_h_
