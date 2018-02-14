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

#ifndef _TechDraw_DrawViewDimension_h_
#define _TechDraw_DrawViewDimension_h_
#include <tuple>

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>

#include "DrawView.h"

namespace Measure {
class Measurement;
}
namespace TechDraw
{
class DrawViewPart;

struct DimRef {
    DrawViewPart* part;
    std::string   sub;
};

class DrawViewPart;

class TechDrawExport DrawViewDimension : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewDimension);

public:
    /// Constructor
    DrawViewDimension();
    virtual ~DrawViewDimension();

    App::PropertyEnumeration       MeasureType;                        //True/Projected
    App::PropertyLinkSubList       References2D;                       //Points to Projection SubFeatures
    App::PropertyLinkSubList       References3D;                       //Points to 3D Geometry SubFeatures
    App::PropertyEnumeration       Type;                               //DistanceX,DistanceY,Diameter, etc
    App::PropertyString            FormatSpec;

    short mustExecute() const;
    bool has2DReferences(void) const;
    bool has3DReferences(void) const;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderDimension";
    }
    //return PyObject as DrawViewDimensionPy
    virtual PyObject *getPyObject(void);

    virtual std::string getFormatedValue();
    virtual double getDimValue();
    DrawViewPart* getViewPart() const;
    virtual QRectF getRect() const { return QRectF(0,0,1,1);}                   //pretend dimensions always fit!
    static int getRefType1(const std::string s);
    static int getRefType2(const std::string s1, const std::string s2);
    int getRefType() const;                                                     //Vertex-Vertex, Edge, Edge-Edge
    void setAll3DMeasurement();
    void clear3DMeasurements(void);
    bool checkReferences2D(void) const;

protected:
    void onChanged(const App::Property* prop);
    virtual void onDocumentRestored();
    bool showUnits() const;
    bool useDecimals() const;
    std::string getPrefix() const;
    std::string getDefaultFormatSpec() const;

protected:
    Measure::Measurement *measurement;
    double dist2Segs(Base::Vector2d s1,
                     Base::Vector2d e1,
                     Base::Vector2d s2,
                     Base::Vector2d e2) const;
private:
    static const char* TypeEnums[];
    static const char* MeasureTypeEnums[];
    void dumpRefs2D(char* text) const;
};

} //namespace TechDraw
#endif
