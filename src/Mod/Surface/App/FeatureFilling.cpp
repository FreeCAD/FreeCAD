/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com> *
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
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <Precision.hxx>
#endif

#include "FeatureFilling.h"
#include <BRepFill_Filling.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <string.h>

using namespace Surface;

PROPERTY_SOURCE(Surface::Filling, Part::Feature)

//Initial values

Filling::Filling()
{
    ADD_PROPERTY(Border,(0,"TopoDS_Edge"));

    ADD_PROPERTY(Curves,(0,"TopoDS_Edge"));

    ADD_PROPERTY(BFaces,(0,"TopoDS_Face"));
    ADD_PROPERTY(orderB,(-1));
    ADD_PROPERTY(CFaces,(0,"TopoDS_Face"));
    ADD_PROPERTY(orderC,(-1));

    ADD_PROPERTY(Points,(0,"TopoDS_Vertex"));

    ADD_PROPERTY(initFace,(0,"TopoDS_Face"));

    ADD_PROPERTY(Degree,(3));
    ADD_PROPERTY(NbPtsOnCur,(3));
    ADD_PROPERTY(NbIter,(2));
    ADD_PROPERTY(Anisotropie,(false));
    ADD_PROPERTY(Tol2d,(0.00001));
    ADD_PROPERTY(Tol3d,(0.0001));
    ADD_PROPERTY(TolAng,(0.001));
    ADD_PROPERTY(TolCurv,(0.01));
    ADD_PROPERTY(MaxDeg,(8));
    ADD_PROPERTY(MaxSegments,(10000));
}

//Define Functions

void appconstr_crv(BRepFill_Filling& builder,const App::PropertyLinkSubList& anEdge, Standard_Boolean bnd);
void appconstr_bface(BRepFill_Filling& builder,const App::PropertyLinkSubList& aFace, const App::PropertyIntegerList& Order);
void appconstr_crvface(BRepFill_Filling& builder, const App::PropertyLinkSubList& anEdge, const App::PropertyLinkSubList& aFace, const App::PropertyIntegerList& Order,  Standard_Boolean bnd);
void appconstr_pt(BRepFill_Filling& builder,const App::PropertyLinkSubList& aVertex);
void appinitface(BRepFill_Filling& builder,const App::PropertyLinkSubList& aFace);

//Check if any components of the surface have been modified

short Filling::mustExecute() const
{
    if (Border.isTouched() ||
        Curves.isTouched() ||
        BFaces.isTouched() ||
        orderB.isTouched() ||
        CFaces.isTouched() ||
        orderC.isTouched() ||
        Points.isTouched() ||
        initFace.isTouched() ||
        Degree.isTouched() ||
        NbPtsOnCur.isTouched() ||
        NbIter.isTouched() ||
        Anisotropie.isTouched() ||
        Tol2d.isTouched() ||
        Tol3d.isTouched() ||
        TolAng.isTouched() ||
        TolCurv.isTouched() ||
        MaxDeg.isTouched() ||
        MaxSegments.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Filling::execute(void)
{

    //Assign Variables

    

    unsigned int Deg  = Degree.getValue();
    unsigned int NPOC = NbPtsOnCur.getValue();
    unsigned int NI   = NbIter.getValue();
    bool Anis = Anisotropie.getValue();
    double T2d = Tol2d.getValue();
    double T3d = Tol3d.getValue();
    double TG1 = TolAng.getValue();
    double TG2 = TolCurv.getValue();
    unsigned int Mdeg = MaxDeg.getValue();
    unsigned int Mseg = MaxSegments.getValue();

    //Perform error checking


    //Begin Construction
    try{

        //Generate Builder with Algorithm Variables

        BRepFill_Filling builder(Deg,NPOC,NI,Anis,T2d,T3d,TG1,TG2,Mdeg,Mseg);

        //Check that borders are defined
        printf("Surface Filling\n");

        if((Border.getSize())<1){return new App::DocumentObjectExecReturn("Border must have at least one curve defined.");}

        //Assign Boundaries
        if(Border.getSize()>0){appconstr_crvface(builder, Border, BFaces, orderB, Standard_True);}

        //Assign Additional Curves if available

        if(Curves.getSize()>0){appconstr_crvface(builder, Curves, CFaces, orderC, Standard_False);}

/*        //Assign Faces

        if(BFaces.getSize()>0){appconstr_bface(builder, BFaces, orderBFaces);}
*/
        //Assign Point Constraints

        if(Points.getSize()>0){appconstr_pt(builder,Points);}

        //Assign Initial Face

        if(initFace.getSize()>0){appinitface(builder,initFace);}

        printf("Building...\n");

        //Build the face
        builder.Build();
        if (!builder.IsDone()){Standard_Failure::Raise("Failed to create a face from constraints");}

        printf("Build Complete\n");

        //Return the face
        TopoDS_Face aFace = builder.Face();

        if (aFace.IsNull()){
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }
        this->Shape.setValue(aFace);

        return App::DocumentObject::StdReturn;

    } //End Try
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    } //End Catch

} //End execute
/*
void appconstr_crv(BRepFill_Filling& builder,const App::PropertyLinkSubList& anEdge, Standard_Boolean bnd){

    printf("Inside appconstr_crv\n");

    int res;

    printf("Entering for loop\n");

    for(int i=0; i<anEdge.getSize(); i++) {

        printf("Processing curve %i\n",i);

        Part::TopoShape ts;
//        Part::TopoShape sub;
        TopoDS_Shape sub;
        TopoDS_Edge etmp;
       
       //the subset has the documentobject and the element name which belongs to it,
       // in our case for example the cube object and the "Edge1" string
       
//        printf("Pre try\n");
//
//        try{anEdge[i].obj;}
//        catch(...){
//            Standard_Failure::Raise("Check Boundary or Curve Definitions.\nShould be of form [(App.ActiveDocument.object, 'Edge Name'),...");
//            return;
//        }
//
//        printf("Made it past try\n");
//
        App::PropertyLinkSubList::SubSet set = anEdge[i];

        printf("Past set = anEdge[i]\n");

        //set.obj should be our box, but just to make sure no one set something stupid
        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
            //we get the shape of the document object which resemble the whole box
            ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
               
            //we want only the subshape which is linked
            sub = ts.getSubShape(set.sub);
            
            if(sub.ShapeType() == TopAbs_EDGE) {etmp = TopoDS::Edge(sub);} //Check Shape type and assign edge
            else{Standard_Failure::Raise("Curves must be type TopoDS_Edge");return;} //Raise exception
                
        }

        else{Standard_Failure::Raise("Boundary or Curve not from Part::Feature");return;}

        res = builder.Add(etmp,GeomAbs_C0,bnd);

        printf("Result of builder.Add: %i\n",res);

    }
}
*/
/*
void appconstr_bface(BRepFill_Filling& builder,const App::PropertyLinkSubList& aFace, const App::PropertyIntegerList& Order){

    int res;

    GeomAbs_Shape ordtmp;

    std::vector<long int>::const_iterator bc = Order.getValues().begin(); //Get the order values

    for(int i=0; i<aFace.getSize(); i++){

        Part::TopoShape ts;
        TopoDS_Shape sub;
        TopoDS_Face ftmp;

        App::PropertyLinkSubList::SubSet set = aFace[i];

        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
            ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
           
            sub = ts.getSubShape(set.sub);
            
            if(sub.ShapeType() == TopAbs_FACE) {ftmp = TopoDS::Face(sub);} //Check Shape type and assign edge
            else{Standard_Failure::Raise("Faces must be type TopoDS_Face");} //Raise exception
                
        }

        else{Standard_Failure::Raise("Face not from Part::Feature");}

        //PropertyEnumerateList doesn't exist yet. Fix when implemented

        if(*bc==0){ordtmp = GeomAbs_C0;}
        else if(*bc==1){ordtmp = GeomAbs_G1;}
        else if(*bc==2){ordtmp = GeomAbs_G2;}
	else{Standard_Failure::Raise("Continuity constraint must be 0, 1 or 2 for C0, G1, and G2.");return;}

        printf("*bc: %li\n",*bc);

        res = builder.Add(ftmp,ordtmp);

        printf("res: %i\n",res);

        bc++;


    }

    return;
}
*/
void appconstr_crvface(BRepFill_Filling& builder, const App::PropertyLinkSubList& anEdge, const App::PropertyLinkSubList& aFace, const App::PropertyIntegerList& Order,  Standard_Boolean bnd){

    int res;

    GeomAbs_Shape ordtmp;

    std::vector<long int>::const_iterator bc = Order.getValues().begin(); //Get the order values
    int fconit = 0;

    for(int i=0; i<anEdge.getSize(); i++){

        Part::TopoShape ts_edge;
        TopoDS_Shape sub_edge;

        Part::TopoShape ts_face;
        TopoDS_Shape sub_face;

        TopoDS_Edge etmp;
        TopoDS_Face ftmp;

        //Get Edge

        App::PropertyLinkSubList::SubSet set_edge = anEdge[i];

        if(set_edge.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
            ts_edge = static_cast<Part::Feature*>(set_edge.obj)->Shape.getShape();
               
            //we want only the subshape which is linked
            sub_edge = ts_edge.getSubShape(set_edge.sub);
            
            if(sub_edge.ShapeType() == TopAbs_EDGE) {etmp = TopoDS::Edge(sub_edge);} //Check Shape type and assign edge
            else{Standard_Failure::Raise("Curves must be type TopoDS_Edge");return;} //Raise exception
                
        }

        else{Standard_Failure::Raise("Boundary or Curve not from Part::Feature");return;}

        /*********************************/

        //Get Order

        //PropertyEnumerateList doesn't exist yet. Fix when implemented

        if(*bc==0){ordtmp = GeomAbs_C0;}
        else if(*bc==1){ordtmp = GeomAbs_G1;}
        else if(*bc==2){ordtmp = GeomAbs_G2;}
	else{Standard_Failure::Raise("Continuity constraint must be 0, 1 or 2 for C0, G1, and G2.");return;}

        /*********************************/

        //Get Face if required

        if(ordtmp==GeomAbs_C0){

            res = builder.Add(etmp,ordtmp,bnd);

        }

        else{

//            if(aFace.getSize()<anEdge.getSize()){
//                Standard_Failure::Raise("Faces must be defined (even as null) for all edges if G1 or G2 constraints are used.");
//            }

            App::PropertyLinkSubList::SubSet set_face = aFace[fconit];

            if(set_face.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
                ts_face = static_cast<Part::Feature*>(set_face.obj)->Shape.getShape();
           
                sub_face = ts_face.getSubShape(set_face.sub);
            
                if(sub_face.ShapeType() == TopAbs_FACE) {ftmp = TopoDS::Face(sub_face);} //Check Shape type and assign edge
                else{Standard_Failure::Raise("Faces must be type TopoDS_Face");} //Raise exception
                
            }

            else{Standard_Failure::Raise("Face not from Part::Feature");}

            printf("*bc: %li\n",*bc);

            res = builder.Add(etmp,ftmp,ordtmp,bnd);

            fconit++;

        }

        printf("res: %i\n",res);

        bc++;

    }

    return;
}

void appconstr_pt(BRepFill_Filling& builder,const App::PropertyLinkSubList& aVertex){

    int res;

    for(int i=0; i<aVertex.getSize(); i++) {

        Part::TopoShape ts;
//        Part::TopoShape sub;
        TopoDS_Shape sub;
        TopoDS_Vertex vtmp;
       
       //the subset has the documentobject and the element name which belongs to it,
       // in our case for example the cube object and the "Vertex1" string
        App::PropertyLinkSubList::SubSet set = aVertex[i];

        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
            //we get the shape of the document object which resemble the whole box
            ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
           
            //we want only the subshape which is linked
            sub = ts.getSubShape(set.sub);
            
            if(sub.ShapeType() == TopAbs_VERTEX) {vtmp = TopoDS::Vertex(sub);} //Check Shape type and assign edge
            else{Standard_Failure::Raise("Curves must be type TopoDS_Vertex");} //Raise exception
                
        }

        else{Standard_Failure::Raise("Point not from Part::Feature");}

        res = builder.Add(BRep_Tool::Pnt(vtmp));

    }
    return;
}

void appinitface(BRepFill_Filling& builder,const App::PropertyLinkSubList& aFace){

    if(aFace.getSize()>1){Standard_Failure::Raise("Only one face may be used for the initial face");return;}

    Part::TopoShape ts;
//        Part::TopoShape sub;
    TopoDS_Shape sub;
    TopoDS_Face face;
       
    //the subset has the documentobject and the element name which belongs to it,
    // in our case for example the cube object and the "Vertex1" string
    App::PropertyLinkSubList::SubSet set = aFace[0];

    if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
        //we get the shape of the document object which resemble the whole box
        ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
           
        //we want only the subshape which is linked
        sub = ts.getSubShape(set.sub);
            
        if(sub.ShapeType() == TopAbs_FACE) {face = TopoDS::Face(sub);} //Check Shape type and assign edge
        else{Standard_Failure::Raise("Faces must be type TopoDS_Face");} //Raise exception
                
    }

    else{Standard_Failure::Raise("Point not from Part::Feature");}

    builder.LoadInitSurface(face);

    return;
}
