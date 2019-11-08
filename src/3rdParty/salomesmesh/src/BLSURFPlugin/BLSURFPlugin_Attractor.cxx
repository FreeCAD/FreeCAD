// Copyright (C) 2007-2016  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// ---
// File    : BLSURFPlugin_Attractor.cxx
// Authors : Renaud Nédélec (OCC)
// ---
// 
// The idea of the algorithm used to calculate the distance on a 
// non-euclidian parametric surface has been found in the ref. below:
//
// Ref:"Accurate Anisotropic Fast Marching for Diffusion-Based Geodesic Tractography"
// S. Jbabdi, P. Bellec, R. Toro, Daunizeau, M. Pélégrini-Issac, and H. Benali1
//

#include "BLSURFPlugin_Attractor.hxx"
#include <utilities.h>
#include <algorithm>
#include <cmath>

// cascade include
#include <ShapeAnalysis.hxx>
#include <ShapeConstruct_ProjectCurveOnSurface.hxx>
#include <Precision.hxx>
#include <GeomLib_IsPlanarSurface.hxx>

BLSURFPlugin_Attractor::BLSURFPlugin_Attractor ()
  : _face(),
  _attractorShape(),
  _attEntry(),
  _vectU(),
  _vectV(),
  _DMap(),
  _known(),
  _trial(),
  _type(-1),
  _gridU(0),
  _gridV(0),
  _u1 (0.),
  _u2 (0.),
  _v1 (0.),
  _v2 (0.),
  _startSize(-1),
  _endSize(-1),
  _actionRadius(-1),
  _constantRadius(-1),
  _isMapBuilt(false),
  _isEmpty(true){ MESSAGE("construction of a void attractor"); }

BLSURFPlugin_Attractor::BLSURFPlugin_Attractor (const TopoDS_Face& Face, const TopoDS_Shape& Attractor, const std::string& attEntry) 
  : _face(),
  _attractorShape(),
  _attEntry(attEntry),
  _vectU(),
  _vectV(),
  _DMap(),
  _known(),
  _trial(),
  _type(0),
  _gridU(),
  _gridV(),
  _u1 (0.),
  _u2 (0.),
  _v1 (0.),
  _v2 (0.),
  _startSize(-1),
  _endSize(-1),
  _actionRadius(-1),
  _constantRadius(-1),
  _isMapBuilt(false),
  _isEmpty(false)
{
  _face = Face;
  _attractorShape = Attractor;
  
  init();
}

bool BLSURFPlugin_Attractor::init(){ 
  Standard_Real u0,v0;
  int i,j,i0,j0 ;
  _known.clear();
  _trial.clear();
  Handle(Geom_Surface) aSurf = BRep_Tool::Surface(_face);

  _distance = &BLSURFPlugin_Attractor::_distanceFromMap;

  if ( GeomLib_IsPlanarSurface( aSurf ).IsPlanar() &&
       _attractorShape.ShapeType() == TopAbs_VERTEX )
  {
    // a specific case, the map is not needed
    gp_Pnt P = BRep_Tool::Pnt( TopoDS::Vertex( _attractorShape ));
    GeomAPI_ProjectPointOnSurf projector( P, aSurf );
    if ( projector.IsDone() && projector.NbPoints() == 1 )
    {
      projector.LowerDistanceParameters(u0,v0);

      _attractorPnt = aSurf->Value( u0,v0 );
      _plane        = aSurf;
      _distance     = &BLSURFPlugin_Attractor::_distanceFromPoint;
      _isMapBuilt   = true;
      _isEmpty      = false;
      return true;
    }
  }
  
  // Calculation of the bounds of the face
  ShapeAnalysis::GetFaceUVBounds(_face,_u1,_u2,_v1,_v2);

  _gridU = 300;
  _gridV = 300;

  for (i=0; i<=_gridU; i++){
    _vectU.push_back(_u1+i*(_u2-_u1)/_gridU) ;
  }
  for (j=0; j<=_gridV; j++){
    _vectV.push_back(_v1+j*(_v2-_v1)/_gridV) ;
  }
  
  // Initialization of _DMap and _known
  std::vector<double> temp(_gridV+1,std::numeric_limits<double>::infinity());  // Set distance of all "far" points to Infinity 
  for (i=0; i<=_gridU; i++){
    _DMap.push_back(temp);
  }
  std::vector<bool> temp2(_gridV+1,false);
  for (i=0; i<=_gridU; i++){
    _known.push_back(temp2);
  }
  
  
  // Determination of the starting points
  TopExp_Explorer anEdgeExp(_attractorShape, TopAbs_EDGE, TopAbs_FACE);
  TopExp_Explorer aVertExp(_attractorShape, TopAbs_VERTEX, TopAbs_EDGE);
  
  for(; anEdgeExp.More(); anEdgeExp.Next()){
    const TopoDS_Edge& anEdge = TopoDS::Edge(anEdgeExp.Current());
    edgeInit(aSurf, anEdge);
  }
  
  for(; aVertExp.More(); aVertExp.Next()){
    const TopoDS_Vertex& aVertex = TopoDS::Vertex(aVertExp.Current());
    Trial_Pnt TPnt(3,0); 
    gp_Pnt P = BRep_Tool::Pnt(aVertex);
    GeomAPI_ProjectPointOnSurf projector( P, aSurf );
    projector.LowerDistanceParameters(u0,v0);
    i0 = floor ( (u0 - _u1) * _gridU / (_u2 - _u1) + 0.5 );
    j0 = floor ( (v0 - _v1) * _gridV / (_v2 - _v1) + 0.5 );
    TPnt[0]=0.;                                                                // Set the distance of the starting point to 0.
    TPnt[1]=i0;
    TPnt[2]=j0;
    _DMap[i0][j0] = 0.;
    _trial.insert(TPnt);                                                       // Move starting point to _trial
  }

  return true;
}

void BLSURFPlugin_Attractor::edgeInit(Handle(Geom_Surface) theSurf, const TopoDS_Edge& anEdge){
  gp_Pnt2d P2;
  double first;
  double last;
  int i,i0,j0;
  Trial_Pnt TPnt(3,0);
  Handle(Geom2d_Curve) aCurve2d; 
  Handle(Geom_Curve) aCurve3d = BRep_Tool::Curve (anEdge, first, last);
  ShapeConstruct_ProjectCurveOnSurface curveProjector;
  curveProjector.Init(theSurf, Precision::Confusion());
  curveProjector.Perform (aCurve3d, first, last, aCurve2d);
  
  int N = 1200;
  for (i=0; i<=N; i++){
    P2 = aCurve2d->Value(first + i * (last-first) / N);
    i0 = floor( (P2.X() - _u1) * _gridU / (_u2 - _u1) + 0.5 );
    j0 = floor( (P2.Y() - _v1) * _gridV / (_v2 - _v1) + 0.5 );
    TPnt[0] = 0.;
    TPnt[1] = i0;
    TPnt[2] = j0;
    _DMap[i0][j0] = 0.;
    _trial.insert(TPnt);
  }
}  


void BLSURFPlugin_Attractor::SetParameters(double Start_Size, double End_Size, double Action_Radius, double Constant_Radius){
  _startSize = Start_Size;
  _endSize = End_Size;
  _actionRadius = Action_Radius;
  _constantRadius = Constant_Radius;
}

double BLSURFPlugin_Attractor::_distanceFromPoint(double u, double v)
{
  return _attractorPnt.Distance( _plane->Value( u, v ));
}

double BLSURFPlugin_Attractor::_distanceFromMap(double u, double v){
  
  //   MG-CADSurf seems to perform a linear interpolation so it's sufficient to give it a non-continuous distance map
  int i = floor ( (u - _u1) * _gridU / (_u2 - _u1) + 0.5 );
  int j = floor ( (v - _v1) * _gridV / (_v2 - _v1) + 0.5 );
  
  return _DMap[i][j];
}

double BLSURFPlugin_Attractor::GetSize(double u, double v)
{
  const double attrDist = (this->*_distance)(u,v);
  const double myDist = 0.5 * (attrDist - _constantRadius + fabs(attrDist - _constantRadius));
  switch(_type)
  {
    case TYPE_EXP:
      if (fabs(_actionRadius) <= std::numeric_limits<double>::epsilon()){ 
        if (myDist <= std::numeric_limits<double>::epsilon()){
          return _startSize;
        }
        else {
          return _endSize;
        }
      }
      else{
        return _endSize - (_endSize - _startSize) * exp(- myDist * myDist / (_actionRadius * _actionRadius) );
      }
      break;
    case TYPE_LIN:
        return _startSize + ( 0.5 * (attrDist - _constantRadius + fabs(attrDist - _constantRadius)) ) ;
      break;
  }
  return -1;
}


void BLSURFPlugin_Attractor::BuildMap() { 
  
  MESSAGE("building the map");
  int i, j, k, n;  
  //int count = 0;
  int ip, jp, kp, np;
  int i0, j0;
  gp_Pnt P;
  gp_Vec D1U,D1V;
  double Guu, Gvv, Guv;         // Components of the local metric tensor
  double du, dv;
  double D_Ref = 0.;
  double Dist = 0.;
  bool Dist_changed;
  IJ_Pnt Current_Pnt(2,0);
  Trial_Pnt TPnt(3,0);
  TTrialSet::iterator min;
  TTrialSet::iterator found;
  Handle(Geom_Surface) aSurf = BRep_Tool::Surface(_face);
  
  // While there are points in "Trial" (representing a kind of advancing front), loop on them -----------------------------------------------------------
  while (_trial.size() > 0 ) {
    min = _trial.begin();                        // Get trial point with min distance from start
    i0 = (*min)[1];
    j0 = (*min)[2];
    _known[i0][j0] = true;                       // Move it to "Known"
    _trial.erase(min);                           // Remove it from "Trial"
    
    // Loop on neighbours of the trial min --------------------------------------------------------------------------------------------------------------
    for (i=i0 - 1 ; i <= i0 + 1 ; i++){ 
      if (!aSurf->IsUPeriodic()){                          // Periodic conditions in U  
        if (i > _gridU ){
          break; }
        else if (i < 0){
          i++; }
      }
      ip = (i + _gridU + 1) % (_gridU+1);                  // We get a periodic index :
      for (j=j0 - 1 ; j <= j0 + 1 ; j++){                  //    ip=modulo(i,N+2) so that  i=-1->ip=N; i=0 -> ip=0 ; ... ; i=N+1 -> ip=0;  
        if (!aSurf->IsVPeriodic()){                        // Periodic conditions in V . 
          if (j > _gridV ){
            break; }
          else if (j < 0){
            j++;
          }
        }
        jp = (j + _gridV + 1) % (_gridV+1);
      
        if (!_known[ip][jp]){                              // If the distance is not known yet
          aSurf->D1(_vectU[ip],_vectV[jp],P,D1U,D1V);      // Calculate the metric tensor at (i,j)
          // G(i,j)  =  | ||dS/du||**2          *     | 
          //            | <dS/du,dS/dv>  ||dS/dv||**2 |
          Guu = D1U.X()*D1U.X() +  D1U.Y()*D1U.Y() + D1U.Z()*D1U.Z();    // Guu = ||dS/du||**2    
          Gvv = D1V.X()*D1V.X() +  D1V.Y()*D1V.Y() + D1V.Z()*D1V.Z();    // Gvv = ||dS/dv||**2           
          Guv = D1U.X()*D1V.X() +  D1U.Y()*D1V.Y() + D1U.Z()*D1V.Z();    // Guv = Gvu = < dS/du,dS/dv > 
          D_Ref = _DMap[ip][jp];                           // Set a ref. distance of the point to its value in _DMap 
          TPnt[0] = D_Ref;                                 // (may be infinite or uncertain)
          TPnt[1] = ip;
          TPnt[2] = jp;
          Dist_changed = false;
          
          // Loop on neighbours to calculate the min distance from them ---------------------------------------------------------------------------------
          for (k=i - 1 ; k <= i + 1 ; k++){
            if (!aSurf->IsUPeriodic()){                              // Periodic conditions in U  
              if(k > _gridU ){
                break;
              }
              else if (k < 0){
                k++; }
            }
            kp = (k + _gridU + 1) % (_gridU+1);                      // periodic index
            for (n=j - 1 ; n <= j + 1 ; n++){ 
              if (!aSurf->IsVPeriodic()){                            // Periodic conditions in V 
                if(n > _gridV){   
                  break;
                }
                else if (n < 0){
                  n++; }
              }
              np = (n + _gridV + 1) % (_gridV+1);                    
              if (_known[kp][np]){                                   // If the distance of the neighbour is known
                                                                     // Calculate the distance from (k,n)
                du = (k-i) * (_u2 - _u1) / _gridU;
                dv = (n-j) * (_v2 - _v1) / _gridV;
                Dist = _DMap[kp][np] + sqrt( Guu * du*du + 2*Guv * du*dv + Gvv * dv*dv );   // ds**2 = du'Gdu + 2*du'Gdv + dv'Gdv  (G is always symetrical)
                if (Dist < D_Ref) {                                  // If smaller than ref. distance  ->  update ref. distance
                  D_Ref = Dist;
                  Dist_changed = true;
                }
              }
            }
          } // End of the loop on neighbours --------------------------------------------------------------------------------------------------------------
          
          if (Dist_changed) {                              // If distance has been updated, update _trial 
            found=_trial.find(TPnt);
            if (found != _trial.end()){
              _trial.erase(found);                         // Erase the point if it was already in _trial
            }
            TPnt[0] = D_Ref;
            TPnt[1] = ip;
            TPnt[2] = jp;
            _DMap[ip][jp] = D_Ref;                         // Set it distance to the minimum distance found during the loop above
            _trial.insert(TPnt);                           // Insert it (or reinsert it) in _trial
          }
        } // end if (!_known[ip][jp])
      } // for
    } // for
  } // while (_trial)
  _known.clear();
  _trial.clear();
  _isMapBuilt = true;
} // end of BuildMap()



