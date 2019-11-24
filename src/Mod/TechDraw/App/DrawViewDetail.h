/***************************************************************************
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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

#ifndef _DrawViewDetail_h_
#define _DrawViewDetail_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>
#include <App/Material.h>

#include <Base/Vector3D.h>

#include <TopoDS_Compound.hxx>

#include "DrawViewPart.h"

class gp_Pln;
class gp_Ax2;
class TopoDS_Face;

namespace TechDraw
{
class Face;
}

namespace TechDraw
{


class TechDrawExport DrawViewDetail : public DrawViewPart
{
    PROPERTY_HEADER(Part::DrawViewDetail);

public:
    /// Constructor
    DrawViewDetail(void);
    virtual ~DrawViewDetail();

    App::PropertyLink   BaseView;
    App::PropertyVector AnchorPoint;
    App::PropertyFloat  Radius;
    App::PropertyString Reference;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onChanged(const App::Property* prop);
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewPart";
    }

public:
    double getFudgeRadius(void);

protected:
    Base::Vector3d toR3(const gp_Ax2 fromSystem, const Base::Vector3d fromPoint);
    void getParameters(void);
    double m_fudge;

};

typedef App::FeaturePythonT<DrawViewDetail> DrawViewDetailPython;

} //namespace TechDraw

#endif
