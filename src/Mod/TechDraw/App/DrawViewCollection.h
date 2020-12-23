/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
 *   Copyright (c) 2013 Luke Parry        <l.parry@warwick.ac.uk>          *
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

#ifndef _TECHDRAW_FEATUREVIEWCOLLECTION_h_
#define _TECHDRAW_FEATUREVIEWCOLLECTION_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include "DrawView.h"

namespace TechDraw
{

/** Base class for collection of view objects
 */
class TechDrawExport DrawViewCollection : public DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewCollection);

public:
    App::PropertyLinkList Views;
public:
    /// Constructor
    DrawViewCollection();
    virtual ~DrawViewCollection();
    short mustExecute() const;

    int addView(DrawView *view);
    int removeView(DrawView *view);
    void rebuildViewList(void);
    bool isUnsetting(void) { return nowUnsetting; }

    int countChildren();
    void lockChildren(void);

    virtual void onDocumentRestored();
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void unsetupObject();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewCollection";
    }
    virtual QRectF getRect(void) const;

protected:
    void onChanged(const App::Property* prop);
    bool nowUnsetting;
};

} //namespace TechDraw

#endif // _TECHDRAW_FEATUREVIEWCOLLECTION_h_
