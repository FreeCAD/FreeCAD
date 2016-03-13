/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
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

#ifndef _DrawViewSection_h_
#define _DrawViewSection_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/FeaturePython.h>

#include <TopoDS_Compound.hxx>

#include "DrawViewPart.h"
#include "Geometry.h"

class gp_Pln;
class TopoDS_Compound;
class TopoDS_Face;

namespace TechDraw
{


/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawViewSection : public DrawViewPart
{
    PROPERTY_HEADER(Part::DrawViewSection);

public:
    /// Constructor
    DrawViewSection(void);
    virtual ~DrawViewSection();

    App::PropertyVector SectionNormal;
    App::PropertyVector SectionOrigin;
    App::PropertyBool ShowCutSurface;

    short mustExecute() const;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
//    virtual void onChanged(const App::Property* prop);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewSection";
    }

public:
    std::vector<TechDrawGeometry::Face*> getFaceGeometry();

protected:
    TopoDS_Shape sectionShape;              //obs??
    gp_Pln getSectionPlane() const;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& shape);
    TopoDS_Face projectFace(const TopoDS_Shape &face,
                                     gp_Pnt faceCenter,
                                     const Base::Vector3d &direction,
                                     const Base::Vector3d &xaxis);
    std::vector<TopoDS_Wire> connectEdges (std::vector<TopoDS_Edge>& edges);
    std::vector<TopoDS_Wire> sortWiresBySize(std::vector<TopoDS_Wire>& w);
    TopoDS_Compound sectionFaces;
    class wireCompare;
};

typedef App::FeaturePythonT<DrawViewSection> DrawViewSectionPython;


} //namespace TechDraw

#endif
