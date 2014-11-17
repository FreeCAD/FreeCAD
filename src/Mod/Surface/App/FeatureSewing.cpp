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
#include <Precision.hxx>
#endif

#include "FeatureSewing.h"
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <Base/Tools.h>
#include <Base/Exception.h>

using namespace Surface;

PROPERTY_SOURCE(Surface::Sewing, Part::Feature)

//Initial values

Sewing::Sewing()
{
    ADD_PROPERTY(aShapeList,(0,"TopoDS_Shape"));

    App::PropertyFloat tol;
    App::PropertyBool sewopt;         //Option for sewing (if false only control)
    App::PropertyBool degenshp;       //Option for analysis of degenerated shapes
    App::PropertyBool cutfreeedges;   //Option for cutting of free edges
    App::PropertyBool nonmanifold;    //Option for non-manifold processing

    ADD_PROPERTY(tol,(0.0000001));
    ADD_PROPERTY(sewopt,(true));
    ADD_PROPERTY(degenshp,(true));
    ADD_PROPERTY(cutfreeedges,(true));
    ADD_PROPERTY(nonmanifold,(false));

}

//Function Definitions

void addshape(BRepBuilderAPI_Sewing& builder,const App::PropertyLinkSubList& aShapeList);

//Check if any components of the surface have been modified

short Sewing::mustExecute() const
{
    if (aShapeList.isTouched() ||
        tol.isTouched() ||
        sewopt.isTouched() ||
        degenshp.isTouched() ||
        cutfreeedges.isTouched() ||
        nonmanifold.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Sewing::execute(void)
{

    //Assign Variables

    double atol = tol.getValue();
    bool opt1 = sewopt.getValue();
    bool opt2 = degenshp.getValue();
    bool opt3 = cutfreeedges.getValue();
    bool opt4 = nonmanifold.getValue();

    //Perform error checking


    bool res;

    //Begin Construction
    try{

        BRepBuilderAPI_Sewing builder(atol,opt1,opt2,opt3,opt4);

        addshape(builder,aShapeList);

        builder.Perform(); //Perform Sewing

        TopoDS_Shape aShape = builder.SewedShape(); //Get Shape
        
        printf("number of degenerated shapes: %i\n",builder.NbDegeneratedShapes());
        printf("number of deleted faces: %i\n",builder.NbDeletedFaces());
        printf("number of free edges: %i\n",builder.NbFreeEdges());
        printf("number of multiple edges: %i\n",builder.NbMultipleEdges());
        printf("number of continuous edges: %i\n",builder.NbContigousEdges());

        if (aShape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        this->Shape.setValue(aShape);


    } //End Try
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    } //End Catch

} //End execute

void addshape(BRepBuilderAPI_Sewing& builder,const App::PropertyLinkSubList& aShapeList){

    for(int i=0; i<aShapeList.getSize(); i++) {

        printf("Processing shape %i\n",i);

        Part::TopoShape ts;
//        Part::TopoShape sub;
        TopoDS_Shape sub;
        std::vector< const char * > temp;
       
       //the subset has the documentobject and the element name which belongs to it,
       // in our case for example the cube object and the "Edge1" string
        App::PropertyLinkSubList::SubSet set = aShapeList[i];

        //set.obj should be our box, but just to make sure no one set something stupid
        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
            //we get the shape of the document object which resemble the whole box
            ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
           
            //we want only the subshape which is linked
            sub = ts.getSubShape(set.sub);
        }
        else{Standard_Failure::Raise("Shape item not from Part::Feature");return;}

        builder.Add(sub);

    }
}
