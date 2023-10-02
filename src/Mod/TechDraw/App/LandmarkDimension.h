/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TechDraw_LandmarkDimension_h_
#define TechDraw_LandmarkDimension_h_

# include <App/DocumentObject.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

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
    ~LandmarkDimension() override;

    App::PropertyStringList  ReferenceTags;     //tags of 2d vertices in DVP

    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    void unsetupObject() override;

    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderDimension"; }

    DrawViewPart* getViewPart() const override;
    int getRefType() const override;

    gp_Ax2 getProjAxis() const;

protected:
    void onChanged(const App::Property* prop) override;
    void onDocumentRestored() override;

    Base::Vector3d projectPoint(const Base::Vector3d& pt, DrawViewPart* dvp) const;

private:
};

} //namespace TechDraw
#endif
