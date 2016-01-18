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

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>

#include "DrawView.h"
#include "DrawViewPart.h"

namespace Measure {
class Measurement;
}
namespace TechDraw
{

class TechDrawExport DrawViewDimension : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewDimension);

public:
    /// Constructor
    DrawViewDimension();
    virtual ~DrawViewDimension();

    App::PropertyEnumeration ProjectionType;                           //True/Projected
    App::PropertyVector ProjDirection;                                 //??why would dim have different projDir from View?
    App::PropertyLinkSubList References;                               //Points to Projection SubFeatures
    App::PropertyEnumeration Type;                                     //DistanceX,DistanceY,Diameter, etc
    App::PropertyVector XAxisDirection;                                //??always equal to View??

    /// Properties for Visualisation
    App::PropertyInteger Precision;
    App::PropertyString  Font;
    App::PropertyFloat   Fontsize;
    App::PropertyBool    CentreLines;
    App::PropertyString  FormatSpec;

    short mustExecute() const;
    bool hasReferences(void) const;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderDimension";
    }

    virtual std::string getFormatedValue() const;
    virtual double getDimValue() const;
    DrawViewPart* getViewPart() const;

protected:
    void onChanged(const App::Property* prop);
    int getIndexFromName(std::string geomName) const;
    int getRefType() const;                                                     //Vertex-Vertex, Edge, Edge-Edge
    int get3DRef(int refIndex, std::string geomType) const;

protected:
    Measure::Measurement *measurement;
    double dist2Segs(Base::Vector2D s1,
                     Base::Vector2D e1,
                     Base::Vector2D s2,
                     Base::Vector2D e2) const;
private:
    static const char* TypeEnums[];
    static const char* ProjTypeEnums[];
    void dumpRefs(char* text) const;
};

} //namespace TechDraw
#endif
