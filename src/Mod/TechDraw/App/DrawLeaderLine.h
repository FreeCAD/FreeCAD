/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TechDraw_DrawLeaderLine_h_
#define TechDraw_DrawLeaderLine_h_

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"


namespace TechDraw
{

class TechDrawExport DrawLeaderLine : public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawLeaderLine);

public:
    DrawLeaderLine();
    ~DrawLeaderLine() override = default;

    App::PropertyLink         LeaderParent;
    App::PropertyVectorList   WayPoints;
    App::PropertyEnumeration  StartSymbol;
    App::PropertyEnumeration  EndSymbol;

/*    App::PropertyInteger      StartSymbol;          //see Gui/QGIArrow for values*/
/*    App::PropertyInteger      EndSymbol;*/

    App::PropertyBool         Scalable;
    App::PropertyBool         AutoHorizontal;

    short mustExecute() const override;
    App::DocumentObjectExecReturn *execute() override;

    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderLeader";
    }
    PyObject *getPyObject() override;
    QRectF getRect() const override { return { 0, 0,1, 1}; }

    Base::Vector3d getAttachPoint();
    DrawView* getBaseView() const;
    virtual App::DocumentObject* getBaseObject() const;
    bool keepUpdated() override;
    double getScale() const override;
    double getBaseScale() const;
    void adjustLastSegment();
    bool getDefAuto() const;

    Base::Vector3d getTileOrigin() const;
    Base::Vector3d getKinkPoint() const;
    Base::Vector3d getTailPoint() const;

protected:
    void onChanged(const App::Property* prop) override;

private:


};

using DrawLeaderLinePython = App::FeaturePythonT<DrawLeaderLine>;

} //namespace TechDraw
#endif
