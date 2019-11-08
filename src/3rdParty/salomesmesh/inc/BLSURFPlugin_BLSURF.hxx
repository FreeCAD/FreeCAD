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
// File    : BLSURFPlugin_BLSURF.hxx
// Authors : Francis KLOSS (OCC) & Patrick LAUG (INRIA) & Lioka RAZAFINDRAZAKA (CEA)
//           & Aurelien ALLEAUME (DISTENE)
//           Size maps developement: Nicolas GEIMER (OCC) & Gilles DAVID (EURIWARE)
// ---
//
#ifndef _BLSURFPlugin_BLSURF_HXX_
#define _BLSURFPlugin_BLSURF_HXX_

#include <BLSURFPlugin_Defs.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#ifdef HAVE_FINITE
#undef HAVE_FINITE            // VSR: avoid compilation warning on Linux : "HAVE_FINITE" redefined
#endif
// rnv: avoid compilation warning on Linux : "_POSIX_C_SOURCE" and "_XOPEN_SOURCE" are redefined
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

// #include <Python.h>
#include <SMESH_Algo.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
// #include <SMESH_Gen_i.hxx>
#include <SMESH_Gen.hxx>
#include <StdMeshers_ViscousLayers2D.hxx>
// #include <SALOMEconfig.h>
// #include CORBA_CLIENT_HEADER(SALOMEDS)
// #include CORBA_CLIENT_HEADER(GEOM_Gen)
#include "Utils_SALOME_Exception.hxx"

extern "C"{
#include <meshgems/meshgems.h>
#include <meshgems/cadsurf.h>
}

#include <BRepClass_FaceClassifier.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_HSurface.hxx>

#include "BLSURFPlugin_Hypothesis.hxx"

class TopoDS_Shape;

class BLSURFPLUGIN_EXPORT BLSURFPlugin_BLSURF: public SMESH_2D_Algo {
  public:
    BLSURFPlugin_BLSURF(int hypId, int studyId, SMESH_Gen* gen, bool theHasGEOM);

    virtual ~BLSURFPlugin_BLSURF();

    virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                                 const TopoDS_Shape&                  aShape,
                                 SMESH_Hypothesis::Hypothesis_Status& aStatus);

    void SetParameters(const BLSURFPlugin_Hypothesis* hyp, cadsurf_session_t *css, const TopoDS_Shape& shape);

    virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);
    virtual bool Compute(SMESH_Mesh & aMesh, SMESH_MesherHelper* aHelper);

    virtual void CancelCompute();
    bool computeCanceled() { return _compute_canceled; }

    virtual bool Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                          MapShapeNbElems& aResMap);

    // List of ids
    typedef std::vector<int> TListOfIDs;

    // PreCad Edges periodicity
    struct TPreCadPeriodicityIDs {
      TListOfIDs shape1IDs;
      TListOfIDs shape2IDs;
      std::vector<double> theSourceVerticesCoords;
      std::vector<double> theTargetVerticesCoords;
    };

    // Edge periodicity
    struct TEdgePeriodicityIDs {
      int theFace1ID;
      int theEdge1ID;
      int theFace2ID;
      int theEdge2ID;
      int edge_orientation;
    };

    // Vertex periodicity
    struct TVertexPeriodicityIDs {
      int theEdge1ID;
      int theVertex1ID;
      int theEdge2ID;
      int theVertex2ID;
    };

    // Vector of pairs of ids
    typedef std::vector< TPreCadPeriodicityIDs > TPreCadIDsPeriodicityVector;
    typedef std::vector< std::pair<int, int> > TShapesIDsPeriodicityVector;
    typedef std::vector< TEdgePeriodicityIDs > TEdgesIDsPeriodicityVector;
    typedef std::vector< TVertexPeriodicityIDs > TVerticesIDsPeriodicityVector;



  protected:
    const BLSURFPlugin_Hypothesis* _hypothesis;
    bool                           _haveViscousLayers;

    TPreCadIDsPeriodicityVector _preCadFacesIDsPeriodicityVector;
    TPreCadIDsPeriodicityVector _preCadEdgesIDsPeriodicityVector;

  private:
    bool compute(SMESH_Mesh&          aMesh,
                 const TopoDS_Shape&  aShape,
                 bool                 allowSubMeshClearing);

    void set_param(cadsurf_session_t *css,
                   const char *       option_name,
                   const char *       option_value);

    TopoDS_Shape entryToShape(std::string entry);
    void addCoordsFromVertices(const std::vector<std::string> &theVerticesEntries, std::vector<double> &theVerticesCoords);
    void addCoordsFromVertex(BLSURFPlugin_Hypothesis::TEntry theVertexEntry, std::vector<double> &theVerticesCoords);
    void createEnforcedVertexOnFace(TopoDS_Shape FaceShape, BLSURFPlugin_Hypothesis::TEnfVertexList enfVertexList);
    void createPreCadFacesPeriodicity(TopoDS_Shape theGeomShape, const BLSURFPlugin_Hypothesis::TPreCadPeriodicity &preCadPeriodicity);
    void createPreCadEdgesPeriodicity(TopoDS_Shape theGeomShape, const BLSURFPlugin_Hypothesis::TPreCadPeriodicity &preCadPeriodicity);
    void Set_NodeOnEdge(SMESHDS_Mesh* meshDS, const SMDS_MeshNode* node, const TopoDS_Shape& ed);
    void BRepClass_FaceClassifierPerform(BRepClass_FaceClassifier* fc, const TopoDS_Face& face, const gp_Pnt& P, const Standard_Real Tol);

  private:
      // PyObject *          main_mod;
      // PyObject *          main_dict;
      SMESH_MesherHelper* myHelper;
      // SALOMEDS::Study_var myStudy;
      // SMESH_Gen_i*        smeshGen_i;

      volatile bool _compute_canceled;
};

#endif
