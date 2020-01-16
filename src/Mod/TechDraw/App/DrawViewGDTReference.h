/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Ludovic Mercier, lidiriel <ludovic@scilink.net>    *
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

#ifndef _TechDraw_DrawViewReference_h_
#define _TechDraw_DrawViewReference_h_
#include <tuple>

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>
# include <App/PropertyUnits.h>

#include "DrawView.h"

class TopoDS_Shape;

namespace Measure {
class Measurement;
}
namespace TechDraw {

class DrawViewPart;

//TODO merge with CommandCreateDims
typedef enum EdgeTypeUtil {
    isInvalid = 0,
    isHorizontal,
    isVertical,
    isDiagonal,
    isCircle,
    isEllipse,
    isBSplineCircle,
    isBSpline,
    isAngle,
    isAngle3Pt
} EdgeTypeUtil;

typedef std::pair<Base::Vector3d, Base::Vector3d> PointPair;

class TechDrawExport DrawViewGDTReference : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewGDTReference);

public:
    /// Constructor
    DrawViewGDTReference();
    virtual ~DrawViewGDTReference();

    App::PropertyLinkSubList References2D;
    App::PropertyEnumeration Type;// Edge, Cosmetic, Feature frame
    App::PropertyString Text;
    App::PropertyFloat SymbolScale;

    short mustExecute() const;
    PointPair getLinearPoints(void) {return m_linearPoints;}

    //TODO add python wrapper
    //virtual PyObject *getPyObject(void);

    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderGDTReference";
    }
    DrawViewPart* getViewPart() const;

protected:
    static const char* TypeEnums[];
    void onChanged(const App::Property* prop);
    virtual void onDocumentRestored();
    PointPair getPointsOneEdge();
    bool has2DReferences(void) const;
    bool checkReferences2D(void) const;

private:
    PointPair m_linearPoints;
};

}
 //namespace TechDraw
#endif
