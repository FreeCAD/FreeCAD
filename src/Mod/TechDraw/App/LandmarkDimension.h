/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com                 *
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

#ifndef _TechDraw_LandmarkDimension_h_
#define _TechDraw_LandmarkDimension_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>

#include "DrawViewDimension.h"

class TopoDS_Shape;
class gp_Ax2;

namespace Measure {
class Measurement;
}
namespace TechDraw
{
class DrawViewPart;

class TechDrawExport LandmarkDimension : public TechDraw::DrawViewDimension
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::LandmarkDimension);

public:
    /// Constructor
    LandmarkDimension();
    virtual ~LandmarkDimension();

    App::PropertyStringList  ReferenceTags;     //tags of 2d vertices in DVP
    
    virtual App::DocumentObjectExecReturn *execute(void) override;
    short mustExecute() const override;
    virtual void unsetupObject() override;

    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderDimension"; }
/*    virtual PyObject *getPyObject(void) override;*/

    virtual bool checkReferences2D() const override;
    virtual bool has2DReferences(void) const override;
    virtual pointPair getPointsTwoVerts() override;
    std::vector<Base::Vector3d> get2DPoints(void) const;
    virtual DrawViewPart* getViewPart() const override;
    virtual int getRefType() const override;

    gp_Ax2 getProjAxis(void) const;

protected:
    virtual void onChanged(const App::Property* prop) override;
    virtual void onDocumentRestored() override;

    Base::Vector3d projectPoint(const Base::Vector3d& pt, DrawViewPart* dvp) const;

private:
};

} //namespace TechDraw
#endif
