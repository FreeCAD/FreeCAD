/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif


#include <HLRBRep_Algo.hxx>
#include <TopoDS_Shape.hxx>
#include <HLRTopoBRep_OutLiner.hxx>
//#include <BRepAPI_MakeOutLine.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh.hxx>


#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Mod/Part/App/PartFeature.h>

#include "FeatureViewPart.h"
#include "ProjectionAlgos.h"

using namespace Drawing;
using namespace std;


//===========================================================================
// FeatureViewPart
//===========================================================================

App::PropertyFloatConstraint::Constraints FeatureViewPart::floatRange = {0.01,5.0,0.05};

PROPERTY_SOURCE(Drawing::FeatureViewPart, Drawing::FeatureView)


FeatureViewPart::FeatureViewPart(void) 
{
    static const char *group = "Shape view";
    static const char *vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0),group,App::Prop_None,"Projection direction");
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"Shape to view");
    ADD_PROPERTY_TYPE(ShowHiddenLines ,(false),group,App::Prop_None,"Control the appearance of the dashed hidden lines");
    ADD_PROPERTY_TYPE(ShowSmoothLines ,(false),group,App::Prop_None,"Control the appearance of the smooth lines");
    ADD_PROPERTY_TYPE(LineWidth,(0.35),vgroup,App::Prop_None,"The thickness of the resulting lines");
    ADD_PROPERTY_TYPE(Tolerance,(0.05),vgroup,App::Prop_None,"The tessellation tolerance");
    Tolerance.setConstraints(&floatRange);
}

FeatureViewPart::~FeatureViewPart()
{
}

#if 0 

App::DocumentObjectExecReturn *FeatureViewPart::execute(void)
{
    std::stringstream result;
	std::string ViewName = Label.getValue();

    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape()._Shape;
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    Handle( HLRBRep_Algo ) brep_hlr = new HLRBRep_Algo;
    brep_hlr->Add( shape );

    gp_Ax2 transform(gp_Pnt(0,0,0),gp_Dir(0,0,1));
    HLRAlgo_Projector projector( transform );
    brep_hlr->Projector( projector );
    brep_hlr->Update();
    brep_hlr->Hide();

    // extracting the result sets:
    HLRBRep_HLRToShape shapes( brep_hlr );

    TopoDS_Shape VisiblyEdges;
    VisiblyEdges = shapes.VCompound();

    TopoDS_Shape HiddenEdges;
    HiddenEdges = shapes.HCompound();

	BRepMesh::Mesh(VisiblyEdges,0.1);
    //HLRBRep_HLRToShape Tool(Hider);
    //TopoDS_Shape V  = Tool.VCompound       ();// artes vives       vues
    //TopoDS_Shape V1 = Tool.Rg1LineVCompound();// artes rgulires  vues
    //TopoDS_Shape VN = Tool.RgNLineVCompound();// artes de couture  vues
    //TopoDS_Shape VO = Tool.OutLineVCompound();// contours apparents vus
    //TopoDS_Shape VI = Tool.IsoLineVCompound();// isoparamtriques   vues
    //TopoDS_Shape H  = Tool.HCompound       ();// artes vives       caches
    //TopoDS_Shape H1 = Tool.Rg1LineHCompound();// artes rgulires  caches
    //TopoDS_Shape HN = Tool.RgNLineHCompound();// artes de couture  caches
    //TopoDS_Shape HO = Tool.OutLineHCompound();// contours apparents cachs
    //TopoDS_Shape HI = Tool.IsoLineHCompound();// isoparamtriques   caches

    result  << "<g" 
            << " id=\"" << ViewName << "\"" << endl
		    << "   stroke=\"rgb(0, 0, 0)\"" << endl 
			<< "   stroke-width=\"0.35\"" << endl
			<< "   stroke-linecap=\"butt\"" << endl
			<< "   stroke-linejoin=\"miter\"" << endl
			<< "   transform=\"translate("<< X.getValue()<<","<<Y.getValue()<<") scale("<< Scale.getValue()<<","<<Scale.getValue()<<")\"" << endl
            << "   fill=\"none\"" << endl
            << "  >" << endl;

    TopExp_Explorer edges( VisiblyEdges, TopAbs_EDGE );
    for (int i = 1 ; edges.More(); edges.Next(),i++ ) {
      TopoDS_Edge edge = TopoDS::Edge( edges.Current() );
      TopLoc_Location location;
      Handle( Poly_Polygon3D ) polygon = BRep_Tool::Polygon3D( edge, location );
      if ( !polygon.IsNull() ) {
        const TColgp_Array1OfPnt& nodes = polygon->Nodes();
         char c = 'M';
        result << "<path id= \"" << ViewName << i << "\" d=\" "; 
        for ( int i = nodes.Lower(); i<= nodes.Upper(); i++ ){
            result << c << " " << nodes(i).X() << " " << nodes(i).Y()<< " " ; 
            c = 'L';
        }
        result << "\" />" << endl;
      }
    }

    result << "</g>" << endl;

    // Apply the resulting fragment
    ViewResult.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}
#else 
App::DocumentObjectExecReturn *FeatureViewPart::execute(void)
{
    std::stringstream result;
    std::string ViewName = Label.getValue();

    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape()._Shape;
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");
    Base::Vector3d Dir = Direction.getValue();
    bool hidden = ShowHiddenLines.getValue();
    bool smooth = ShowSmoothLines.getValue();

    try {
        ProjectionAlgos Alg(ProjectionAlgos::invertY(shape),Dir);
        result  << "<g" 
                << " id=\"" << ViewName << "\"" << endl
                << "   transform=\"rotate("<< Rotation.getValue() << ","<< X.getValue()<<","<<Y.getValue()<<") translate("<< X.getValue()<<","<<Y.getValue()<<") scale("<< Scale.getValue()<<","<<Scale.getValue()<<")\"" << endl
                << "  >" << endl;

        ProjectionAlgos::ExtractionType type = ProjectionAlgos::Plain;
        if (hidden) type = (ProjectionAlgos::ExtractionType)(type|ProjectionAlgos::WithHidden);
        if (smooth) type = (ProjectionAlgos::ExtractionType)(type|ProjectionAlgos::WithSmooth);
        result << Alg.getSVG(type, this->LineWidth.getValue() / this->Scale.getValue(), this->Tolerance.getValue());

        result << "</g>" << endl;

        // Apply the resulting fragment
        ViewResult.setValue(result.str().c_str());

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

#endif 


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Drawing::FeatureViewPartPython, Drawing::FeatureViewPart)
template<> const char* Drawing::FeatureViewPartPython::getViewProviderName(void) const {
    return "DrawingGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class DrawingExport FeaturePythonT<Drawing::FeatureViewPart>;
}
