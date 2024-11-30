/***************************************************************************
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

#ifndef TechDraw_DrawViewBalloon_h_
#define TechDraw_DrawViewBalloon_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"


class TopoDS_Shape;

namespace Measure
{
class Measurement;
}
namespace TechDraw
{

class DrawViewPart;

class TechDrawExport DrawViewBalloon: public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewBalloon);

public:
    /// Constructor
    DrawViewBalloon();
    ~DrawViewBalloon() override;

    App::PropertyLink SourceView;
    App::PropertyString Text;
    App::PropertyEnumeration EndType;
    App::PropertyEnumeration BubbleShape;
    App::PropertyFloatConstraint ShapeScale;
    App::PropertyFloatConstraint EndTypeScale;
    App::PropertyDistance OriginX;
    App::PropertyDistance OriginY;
    App::PropertyFloat TextWrapLen;
    App::PropertyDistance KinkLength;

    short mustExecute() const override;

    DrawView* getParentView() const;
    QPointF origin;//WF never used??
    QPointF getOrigin();
    void setOrigin(QPointF p);

    //virtual PyObject *getPyObject(void);

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override { return "TechDrawGui::ViewProviderBalloon"; }

    static const char* balloonTypeEnums[];

    void handleXYLock() override;

    void setOrigin(Base::Vector3d newOrigin);

    Base::Vector3d getOriginOffset() const;

protected:
    void onChanged(const App::Property* prop) override;
    void handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName,
                                   App::Property* prop) override;
    void handleChangedPropertyName(Base::XMLReader& reader, const char* TypeName,
                                   const char* PropName) override;

private:
    static App::PropertyFloatConstraint::Constraints SymbolScaleRange;
};

}//namespace TechDraw
#endif
