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

#ifndef SURFACE_FEATUREFILLING_H
#define SURFACE_FEATUREFILLING_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include "Mod/Part/App/PartFeature.h"

namespace Surface
{

class SurfaceExport Filling :  public Part::Feature
{
    PROPERTY_HEADER(Surface::Filling);

public:
    Filling();

    //Properties of Curves

    App::PropertyLinkSubList Border;   // Border Edges (C0 is required for edges without a corresponding face)  
    App::PropertyLinkSubList Curves;   // Other Constraint Curves (C0 is required for edges without a corresponding face)

    App::PropertyLinkSubList BFaces;   // Border Faces (C0, G1 and G2 are possible)
    App::PropertyIntegerList orderB;   // Order of constraint on border faces
    App::PropertyLinkSubList CFaces;   // Curve Faces (C0, G1 and G2 are possible)
    App::PropertyIntegerList orderC;   // Order of constraint on curve faces

    App::PropertyLinkSubList Points;   // Constraint Points (on Surface)

    App::PropertyLinkSubList initFace; // Initial Face to use 

    //Algorithm Variables
    App::PropertyInteger Degree;      //Starting degree
    App::PropertyInteger NbPtsOnCur;  //Number of points on an edge for constraint
    App::PropertyInteger NbIter;      //Number of iterations
    App::PropertyBool Anisotropie;    //?
    App::PropertyFloat Tol2d;         //2D Tolerance
    App::PropertyFloat Tol3d;         //3D Tolerance
    App::PropertyFloat TolAng;        //G1 tolerance
    App::PropertyFloat TolCurv;       //G2 tolerance
    App::PropertyInteger MaxDeg;      //Maximum curve degree
    App::PropertyInteger MaxSegments; //Maximum number of segments

    // recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
//    const char* getViewProviderName(void) const {
//        return "PartGui::ViewProviderFilling";
//    }

};
} //Namespace Surface
#endif
