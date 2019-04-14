/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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

#ifndef _TechDraw_DrawLeaderLine_h_
#define _TechDraw_DrawLeaderLine_h_
#include <tuple>

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>

#include "DrawView.h"


namespace TechDraw
{

class TechDrawExport DrawLeaderLine : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawLeaderLine);

public:
    DrawLeaderLine();
    virtual ~DrawLeaderLine();

    App::PropertyLink         LeaderParent;
    App::PropertyVectorList   WayPoints;
    App::PropertyInteger      StartSymbol;
    App::PropertyInteger      EndSymbol;
    App::PropertyBool         Scalable;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);

    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderLeader";
    }
    virtual PyObject *getPyObject(void);
    virtual QRectF getRect() const { return QRectF(0,0,1,1);}

    Base::Vector3d getAttachPoint(void);
    DrawView* getBaseView(void) const;
    virtual App::DocumentObject* getBaseObject(void) const;
    bool keepUpdated(void);
    double getScale(void);

protected:
    virtual void onChanged(const App::Property* prop);

private:
};

typedef App::FeaturePythonT<DrawLeaderLine> DrawLeaderLinePython;

} //namespace TechDraw
#endif
