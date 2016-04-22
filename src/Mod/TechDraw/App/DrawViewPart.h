/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
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

#ifndef _DrawViewPart_h_
#define _DrawViewPart_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include "DrawView.h"
#include <App/FeaturePython.h>

#include <Base/BoundBox.h>
#include "GeometryObject.h"

namespace TechDraw {
class DrawHatch;
}

namespace TechDraw
{

/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawViewPart : public DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewPart);

public:
    /// Constructor
    DrawViewPart(void);
    virtual ~DrawViewPart();

    App::PropertyLink   Source;                                        //Part Feature
    App::PropertyVector Direction;  //TODO: Rename to YAxisDirection or whatever this actually is  (ProjectionDirection)
    App::PropertyVector XAxisDirection;
    App::PropertyBool   ShowHiddenLines;
    App::PropertyBool   ShowSmoothLines;
    App::PropertyBool   ShowSeamLines;
    App::PropertyFloat  LineWidth;
    App::PropertyFloat  HiddenWidth;
    App::PropertyFloatConstraint  Tolerance;
//    App::PropertyLinkList   HatchAreas;                                //DrawHatch

    //int addHatch(App::DocumentObject *docObj);
    //int removeHatch(App::DocumentObject *docObj);
    std::vector<TechDraw::DrawHatch*> getHatches(void) const;

    //TODO: are there use-cases for Python access to TechDrawGeometry???

    const std::vector<TechDrawGeometry::Vertex *> & getVertexGeometry() const;
    const std::vector<TechDrawGeometry::BaseGeom  *> & getEdgeGeometry() const;
    const std::vector<TechDrawGeometry::Face *> & getFaceGeometry() const;
    bool hasGeometry(void) const;

    TechDrawGeometry::BaseGeom* getProjEdgeByIndex(int idx) const;               //get existing geom for edge idx in projection
    TechDrawGeometry::Vertex* getProjVertexByIndex(int idx) const;               //get existing geom for vertex idx in projection

    virtual Base::BoundBox3d getBoundingBox() const;

    short mustExecute() const;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewPart";
    }
    //return PyObject as DrawViewPartPy
    virtual PyObject *getPyObject(void);

    void dumpVertexes(const char* text, const TopoDS_Shape& s);

protected:
    TechDrawGeometry::GeometryObject *geometryObject;
    Base::BoundBox3d bbox;

    void onChanged(const App::Property* prop);
    Base::Vector3d getValidXDir() const;
    void buildGeometryObject(TopoDS_Shape shape, gp_Pnt& center);
    void extractFaces();
    std::vector<TopoDS_Wire> connectEdges (std::vector<TopoDS_Edge>& edges);
    std::vector<TopoDS_Wire> sortWiresBySize(std::vector<TopoDS_Wire>& w, bool reverse = false);
    class wireCompare;

private:
    static App::PropertyFloatConstraint::Constraints floatRange;

};

typedef App::FeaturePythonT<DrawViewPart> DrawViewPartPython;

} //namespace TechDraw

#endif  // #ifndef _DrawViewPart_h_
