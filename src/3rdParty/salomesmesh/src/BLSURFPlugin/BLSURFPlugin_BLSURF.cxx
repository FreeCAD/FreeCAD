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
// File    : BLSURFPlugin_BLSURF.cxx
// Authors : Francis KLOSS (OCC) & Patrick LAUG (INRIA) & Lioka RAZAFINDRAZAKA (CEA)
//           & Aurelien ALLEAUME (DISTENE)
//           Size maps developement: Nicolas GEIMER (OCC) & Gilles DAVID (EURIWARE)
// ---

#include "BLSURFPlugin_BLSURF.hxx"
#include "BLSURFPlugin_Hypothesis.hxx"
#include "BLSURFPlugin_Attractor.hxx"

extern "C"{
#include <meshgems/meshgems.h>
#include <meshgems/cadsurf.h>
}

// #include <structmember.h>


#include <Basics_Utils.hxx>
#include <Basics_OCCTVersion.hxx>

#include <SMDS_EdgePosition.hxx>
#include <SMESHDS_Group.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MeshEditor.hxx>
#include <SMESH_MesherHelper.hxx>
#include <StdMeshers_FaceSide.hxx>
#include <StdMeshers_ViscousLayers2D.hxx>
#include <SMESH_File.hxx>
#include <SMESH_subMesh.hxx>

#include <utilities.h>

#include <limits>
#include <list>
#include <vector>
#include <set>
#include <cstdlib>

// OPENCASCADE includes
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_Line.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <NCollection_DataMap.hxx>
#include <NCollection_Map.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_DataMapOfShapeInteger.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>

#ifndef WIN32
#include <fenv.h>
#endif

using namespace std;

/* ==================================
 * ===========  PYTHON ==============
 * ==================================*/

//namespace
//{
//  typedef struct {
//    PyObject_HEAD
//    int softspace;
//    std::string *out;
//    } PyStdOut;
//
//  static void
//  PyStdOut_dealloc(PyStdOut *self)
//  {
//    PyObject_Del(self);
//  }
//
//  static PyObject *
//  PyStdOut_write(PyStdOut *self, PyObject *args)
//  {
//    char *c;
//    int l;
//    if (!PyArg_ParseTuple(args, "t#:write",&c, &l))
//      return NULL;
//
//    *(self->out)=*(self->out)+c;
//
//    Py_INCREF(Py_None);
//    return Py_None;
//  }
//
//  static PyMethodDef PyStdOut_methods[] = {
//    {"write",  (PyCFunction)PyStdOut_write,  METH_VARARGS,
//      PyDoc_STR("write(string) -> None")},
//    {NULL,    NULL}   /* sentinel */
//  };
//
//  static PyMemberDef PyStdOut_memberlist[] = {
//    {(char*)"softspace", T_INT,  offsetof(PyStdOut, softspace), 0,
//     (char*)"flag indicating that a space needs to be printed; used by print"},
//    {NULL} /* Sentinel */
//  };
//
//  static PyTypeObject PyStdOut_Type = {
//    /* The ob_type field must be initialized in the module init function
//     * to be portable to Windows without using C++. */
//    PyObject_HEAD_INIT(NULL)
//    // TODO Removed from Python2. Could add #indef for Python 2/3 support.
//	// 0,                            /*ob_size*/
//    "PyOut",                      /*tp_name*/
//    sizeof(PyStdOut),             /*tp_basicsize*/
//    0,                            /*tp_itemsize*/
//    /* methods */
//    (destructor)PyStdOut_dealloc, /*tp_dealloc*/
//    0,                            /*tp_print*/
//    0,                            /*tp_getattr*/
//    0,                            /*tp_setattr*/
//    0,                            /*tp_compare*/
//    0,                            /*tp_repr*/
//    0,                            /*tp_as_number*/
//    0,                            /*tp_as_sequence*/
//    0,                            /*tp_as_mapping*/
//    0,                            /*tp_hash*/
//    0,                            /*tp_call*/
//    0,                            /*tp_str*/
//    PyObject_GenericGetAttr,      /*tp_getattro*/
//    /* softspace is writable:  we must supply tp_setattro */
//    PyObject_GenericSetAttr,      /* tp_setattro */
//    0,                            /*tp_as_buffer*/
//    Py_TPFLAGS_DEFAULT,           /*tp_flags*/
//    0,                            /*tp_doc*/
//    0,                            /*tp_traverse*/
//    0,                            /*tp_clear*/
//    0,                            /*tp_richcompare*/
//    0,                            /*tp_weaklistoffset*/
//    0,                            /*tp_iter*/
//    0,                            /*tp_iternext*/
//    PyStdOut_methods,             /*tp_methods*/
//    PyStdOut_memberlist,          /*tp_members*/
//    0,                            /*tp_getset*/
//    0,                            /*tp_base*/
//    0,                            /*tp_dict*/
//    0,                            /*tp_descr_get*/
//    0,                            /*tp_descr_set*/
//    0,                            /*tp_dictoffset*/
//    0,                            /*tp_init*/
//    0,                            /*tp_alloc*/
//    0,                            /*tp_new*/
//    0,                            /*tp_free*/
//    0,                            /*tp_is_gc*/
//  };
//
//  PyObject * newPyStdOut( std::string& out )
//  {
//    PyStdOut* self = PyObject_New(PyStdOut, &PyStdOut_Type);
//    if (self) {
//      self->softspace = 0;
//      self->out=&out;
//    }
//    return (PyObject*)self;
//  }
// }


////////////////////////END PYTHON///////////////////////////

//////////////////MY MAPS////////////////////////////////////////
namespace {
TopTools_IndexedMapOfShape FacesWithSizeMap;
std::map<int,string> FaceId2SizeMap;
TopTools_IndexedMapOfShape EdgesWithSizeMap;
std::map<int,string> EdgeId2SizeMap;
TopTools_IndexedMapOfShape VerticesWithSizeMap;
std::map<int,string> VertexId2SizeMap;

// std::map<int,PyObject*> FaceId2PythonSmp;
// std::map<int,PyObject*> EdgeId2PythonSmp;
// std::map<int,PyObject*> VertexId2PythonSmp;

typedef std::map<int, std::vector< BLSURFPlugin_Attractor* > > TId2ClsAttractorVec;
TId2ClsAttractorVec FaceId2ClassAttractor;
TId2ClsAttractorVec FaceIndex2ClassAttractor;
std::map<int,std::vector<double> > FaceId2AttractorCoords;
int theNbAttractors;

TopTools_IndexedMapOfShape FacesWithEnforcedVertices;
std::map< int, BLSURFPlugin_Hypothesis::TEnfVertexCoordsList > FaceId2EnforcedVertexCoords;
std::map< BLSURFPlugin_Hypothesis::TEnfVertexCoords, BLSURFPlugin_Hypothesis::TEnfVertexCoords > EnfVertexCoords2ProjVertex;
std::map< BLSURFPlugin_Hypothesis::TEnfVertexCoords, BLSURFPlugin_Hypothesis::TEnfVertexList > EnfVertexCoords2EnfVertexList;
SMESH_MesherHelper* theHelper;

bool HasSizeMapOnFace=false;
bool HasSizeMapOnEdge=false;
bool HasSizeMapOnVertex=false;
//bool HasAttractorOnFace=false;
}
//=============================================================================
/*!
 *
 */
//=============================================================================

BLSURFPlugin_BLSURF::BLSURFPlugin_BLSURF(int        hypId,
                                         int        studyId,
                                         SMESH_Gen* gen,
                                         bool       theHasGEOM)
  : SMESH_2D_Algo(hypId, studyId, gen)
{
  _name = theHasGEOM ? "MG-CADSurf" : "MG-CADSurf_NOGEOM";//"BLSURF";
  _shapeType = (1 << TopAbs_FACE); // 1 bit /shape type
  _compatibleHypothesis.push_back(BLSURFPlugin_Hypothesis::GetHypType(theHasGEOM));
  if ( theHasGEOM )
    _compatibleHypothesis.push_back(StdMeshers_ViscousLayers2D::GetHypType());
  _requireDiscreteBoundary = false;
  _onlyUnaryInput = false;
  _hypothesis = NULL;
  _supportSubmeshes = true;
  _requireShape = theHasGEOM;

  /*
  smeshGen_i = SMESH_Gen_i::GetSMESHGen();
  CORBA::Object_var anObject = smeshGen_i->GetNS()->Resolve("/myStudyManager");
  SALOMEDS::StudyManager_var aStudyMgr = SALOMEDS::StudyManager::_narrow(anObject);

  myStudy = NULL;
  myStudy = aStudyMgr->GetStudyByID(_studyId);
  */

  /* Initialize the Python interpreter */
  /*
  assert(Py_IsInitialized());
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  main_mod = NULL;
  main_mod = PyImport_AddModule("__main__");

  main_dict = NULL;
  main_dict = PyModule_GetDict(main_mod);

  PyRun_SimpleString("from math import *");
  PyGILState_Release(gstate);
  */

  FacesWithSizeMap.Clear();
  FaceId2SizeMap.clear();
  EdgesWithSizeMap.Clear();
  EdgeId2SizeMap.clear();
  VerticesWithSizeMap.Clear();
  VertexId2SizeMap.clear();
  // FaceId2PythonSmp.clear();
  // EdgeId2PythonSmp.clear();
  // VertexId2PythonSmp.clear();
  FaceId2AttractorCoords.clear();
  FaceId2ClassAttractor.clear();
  FaceIndex2ClassAttractor.clear();
  FacesWithEnforcedVertices.Clear();
  FaceId2EnforcedVertexCoords.clear();
  EnfVertexCoords2ProjVertex.clear();
  EnfVertexCoords2EnfVertexList.clear();

  _compute_canceled = false;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

BLSURFPlugin_BLSURF::~BLSURFPlugin_BLSURF()
{
}


//=============================================================================
/*!
 *
 */
//=============================================================================

bool BLSURFPlugin_BLSURF::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  _hypothesis        = NULL;
  _haveViscousLayers = false;

  list<const SMESHDS_Hypothesis*>::const_iterator itl;
  const SMESHDS_Hypothesis* theHyp;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape,
                                                                  /*ignoreAuxiliary=*/false);
  aStatus = SMESH_Hypothesis::HYP_OK;
  if ( hyps.empty() )
  {
    return true;  // can work with no hypothesis
  }

  for ( itl = hyps.begin(); itl != hyps.end() && ( aStatus == HYP_OK ); ++itl )
  {
    theHyp = *itl;
    string hypName = theHyp->GetName();
    if ( hypName == BLSURFPlugin_Hypothesis::GetHypType(true) ||
         hypName == BLSURFPlugin_Hypothesis::GetHypType(false) )
    {
      _hypothesis = static_cast<const BLSURFPlugin_Hypothesis*> (theHyp);
      ASSERT(_hypothesis);
      if ( _hypothesis->GetPhysicalMesh() == BLSURFPlugin_Hypothesis::DefaultSize &&
           _hypothesis->GetGeometricMesh() == BLSURFPlugin_Hypothesis::DefaultGeom )
        //  hphy_flag = 0 and hgeo_flag = 0 is not allowed (spec)
        aStatus = SMESH_Hypothesis::HYP_BAD_PARAMETER;
    }
    else if ( hypName == StdMeshers_ViscousLayers2D::GetHypType() )
    {
      if ( !_haveViscousLayers )
      {
        if ( error( StdMeshers_ViscousLayers2D::CheckHypothesis( aMesh, aShape, aStatus )))
          _haveViscousLayers = true;
      }
    }
    else
    {
      aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
    }
  }
  return aStatus == SMESH_Hypothesis::HYP_OK;
}

//=============================================================================
/*!
 * Pass parameters to MG-CADSurf
 */
//=============================================================================

inline std::string val_to_string(double d)
{
   std::ostringstream o;
   o << d;
   return o.str();
}

inline std::string val_to_string_rel(double d)
{
   std::ostringstream o;
   o << d;
   o << 'r';
   return o.str();
}

inline std::string val_to_string(int i)
{
   std::ostringstream o;
   o << i;
   return o.str();
}

inline std::string val_to_string_rel(int i)
{
   std::ostringstream o;
   o << i;
   o << 'r';
   return o.str();
}

double _smp_phy_size;
status_t size_on_surface(integer face_id, real *uv, real *size, void *user_data);
status_t size_on_edge(integer edge_id, real t, real *size, void *user_data);
status_t size_on_vertex(integer vertex_id, real *size, void *user_data);

typedef struct {
        gp_XY uv;
        gp_XYZ xyz;
} projectionPoint;

/////////////////////////////////////////////////////////

projectionPoint getProjectionPoint(TopoDS_Face& theFace, const gp_Pnt& thePoint)
{
  projectionPoint myPoint;

  if ( theFace.IsNull() )
  {
    TopoDS_Shape foundFace, myShape = theHelper->GetSubShape();
    TopTools_MapOfShape checkedFaces;
    std::map< double, std::pair< TopoDS_Face, gp_Pnt2d > > dist2face;

    for ( TopExp_Explorer exp ( myShape, TopAbs_FACE ); exp.More(); exp.Next())
    {
      const TopoDS_Face& face = TopoDS::Face( exp.Current() );
      if ( !checkedFaces.Add( face )) continue;

      // check distance to face
      Handle(ShapeAnalysis_Surface) surface = theHelper->GetSurface( face );
      gp_Pnt2d uv = surface->ValueOfUV( thePoint, Precision::Confusion());
      double distance = surface->Gap();
      if ( distance > Precision::Confusion() )
      {
        // the face is far, store for future analysis
        dist2face.insert( std::make_pair( distance, std::make_pair( face, uv )));
      }
      else
      {
        // check location on the face
        BRepClass_FaceClassifier FC( face, uv, BRep_Tool::Tolerance( face ));
        if ( FC.State() == TopAbs_IN )
        {
          if ( !foundFace.IsNull() )
            return myPoint; // thePoint seems to be TopAbs_ON
          foundFace   = face;
          myPoint.uv  = uv.XY();
          myPoint.xyz = surface->Value( uv ).XYZ();
          // break;
        }
        if ( FC.State() == TopAbs_ON )
          return myPoint;
      }
    }
    if ( foundFace.IsNull() )
    {
      // find the closest face
      std::map< double, std::pair< TopoDS_Face, gp_Pnt2d > >::iterator d2f = dist2face.begin();
      for ( ; d2f != dist2face.end(); ++d2f )
      {
        const TopoDS_Face& face = d2f->second.first;
        const gp_Pnt2d &     uv = d2f->second.second;
        BRepClass_FaceClassifier FC( face, uv, Precision::Confusion());
        if ( FC.State() == TopAbs_IN )
        {
          foundFace   = face;
          myPoint.uv  = uv.XY();
          myPoint.xyz = theHelper->GetSurface( face )->Value( uv ).XYZ();
          break;
        }
      }
    }
    // set the resultShape
    // if ( foundFace.IsNull() )
    //   throw SMESH_ComputeError(COMPERR_BAD_PARMETERS,
    //                            "getProjectionPoint: can't find a face by a vertex");
    theFace = TopoDS::Face( foundFace );
  }
  else
  {
    Handle(Geom_Surface) surface = BRep_Tool::Surface( theFace );
    GeomAPI_ProjectPointOnSurf projector( thePoint, surface );
    if ( !projector.IsDone() || projector.NbPoints()==0 )
      throw SMESH_ComputeError(COMPERR_BAD_PARMETERS,
                               "getProjectionPoint: can't project a vertex to a face");

    Standard_Real u,v;
    projector.LowerDistanceParameters(u,v);
    myPoint.uv = gp_XY(u,v);
    gp_Pnt aPnt = projector.NearestPoint();
    myPoint.xyz = gp_XYZ(aPnt.X(),aPnt.Y(),aPnt.Z());

    BRepClass_FaceClassifier FC( theFace, myPoint.uv, Precision::Confusion());
    if ( FC.State() != TopAbs_IN )
      theFace.Nullify();
  }

  return myPoint;
}

/////////////////////////////////////////////////////////
TopoDS_Shape BLSURFPlugin_BLSURF::entryToShape(std::string entry)
{
  TopoDS_Shape S;
  // TODO entryToShape
  /*
  if ( !entry.empty() )
  {
    GEOM::GEOM_Object_var aGeomObj;
    SALOMEDS::SObject_var aSObj = myStudy->FindObjectID( entry.c_str() );
    if (!aSObj->_is_nil()) {
      CORBA::Object_var obj = aSObj->GetObject();
      aGeomObj = GEOM::GEOM_Object::_narrow(obj);
      aSObj->UnRegister();
    }
    if ( !aGeomObj->_is_nil() )
      S = smeshGen_i->GeomObjectToShape( aGeomObj.in() );
  }
  */
  return S;
}

void _createEnforcedVertexOnFace(TopoDS_Face faceShape, gp_Pnt aPnt, BLSURFPlugin_Hypothesis::TEnfVertex *enfVertex)
{
  BLSURFPlugin_Hypothesis::TEnfVertexCoords enf_coords, coords, s_coords;

  // Find the face and get the (u,v) values of the enforced vertex on the face
  projectionPoint myPoint = getProjectionPoint(faceShape,aPnt);
  if ( faceShape.IsNull() )
    return;

  enf_coords.push_back(aPnt.X());
  enf_coords.push_back(aPnt.Y());
  enf_coords.push_back(aPnt.Z());

  coords.push_back(myPoint.uv.X());
  coords.push_back(myPoint.uv.Y());
  coords.push_back(myPoint.xyz.X());
  coords.push_back(myPoint.xyz.Y());
  coords.push_back(myPoint.xyz.Z());

  s_coords.push_back(myPoint.xyz.X());
  s_coords.push_back(myPoint.xyz.Y());
  s_coords.push_back(myPoint.xyz.Z());

  // Save pair projected vertex / enf vertex
  EnfVertexCoords2ProjVertex[s_coords] = enf_coords;
  pair<BLSURFPlugin_Hypothesis::TEnfVertexList::iterator,bool> ret;
  BLSURFPlugin_Hypothesis::TEnfVertexList::iterator it;
  ret = EnfVertexCoords2EnfVertexList[s_coords].insert(enfVertex);
  if (ret.second == false) {
    it = ret.first;
    (*it)->grpName = enfVertex->grpName;
  }

  int key = 0;
  if (! FacesWithEnforcedVertices.Contains(faceShape)) {
    key = FacesWithEnforcedVertices.Add(faceShape);
  }
  else {
    key = FacesWithEnforcedVertices.FindIndex(faceShape);
  }

  // If a node is already created by an attractor, do not create enforced vertex
  int attractorKey = FacesWithSizeMap.FindIndex(faceShape);
  bool sameAttractor = false;
  if (attractorKey >= 0)
    if (FaceId2AttractorCoords.count(attractorKey) > 0)
      if (FaceId2AttractorCoords[attractorKey] == coords)
        sameAttractor = true;

  if (FaceId2EnforcedVertexCoords.find(key) != FaceId2EnforcedVertexCoords.end()) {
    if (! sameAttractor)
      FaceId2EnforcedVertexCoords[key].insert(coords); // there should be no redondant coords here (see std::set management)
  }
  else {
    if (! sameAttractor) {
      BLSURFPlugin_Hypothesis::TEnfVertexCoordsList ens;
      ens.insert(coords);
      FaceId2EnforcedVertexCoords[key] = ens;
    }
  }
}
  
/////////////////////////////////////////////////////////
void BLSURFPlugin_BLSURF::createEnforcedVertexOnFace(TopoDS_Shape faceShape, BLSURFPlugin_Hypothesis::TEnfVertexList enfVertexList)
{
  BLSURFPlugin_Hypothesis::TEnfVertex* enfVertex;
  gp_Pnt aPnt;

  BLSURFPlugin_Hypothesis::TEnfVertexList::const_iterator enfVertexListIt = enfVertexList.begin();

  for( ; enfVertexListIt != enfVertexList.end() ; ++enfVertexListIt ) {
    enfVertex = *enfVertexListIt;
    // Case of manual coords
    if (enfVertex->coords.size() != 0) {
      aPnt.SetCoord(enfVertex->coords[0],enfVertex->coords[1],enfVertex->coords[2]);
      _createEnforcedVertexOnFace( TopoDS::Face(faceShape),  aPnt, enfVertex);
    }

    // Case of geom vertex coords
    if (enfVertex->geomEntry != "") {
      TopoDS_Shape GeomShape = entryToShape(enfVertex->geomEntry);
      TopAbs_ShapeEnum GeomType  = GeomShape.ShapeType();
       if (GeomType == TopAbs_VERTEX)
       {
         enfVertex->vertex = TopoDS::Vertex( GeomShape );
         aPnt = BRep_Tool::Pnt( enfVertex->vertex );
         _createEnforcedVertexOnFace( TopoDS::Face(faceShape),  aPnt, enfVertex);
       }
       // Group Management
       if (GeomType == TopAbs_COMPOUND)
       {
         for (TopoDS_Iterator it (GeomShape); it.More(); it.Next())
           if (it.Value().ShapeType() == TopAbs_VERTEX)
           {
             enfVertex->vertex = TopoDS::Vertex( it.Value() );
             aPnt = BRep_Tool::Pnt( enfVertex->vertex );
             _createEnforcedVertexOnFace( TopoDS::Face(faceShape),  aPnt, enfVertex);
           }
       }
    }
  }
}

/////////////////////////////////////////////////////////
void createAttractorOnFace(TopoDS_Shape GeomShape, std::string AttractorFunction, double defaultSize)
{
  double xa, ya, za; // Coordinates of attractor point
  double a, b;       // Attractor parameter
  double d = 0.;
  bool createNode=false; // To create a node on attractor projection
  size_t pos1, pos2;
  const char *sep = ";";
  // atIt->second has the following pattern:
  // ATTRACTOR(xa;ya;za;a;b;True|False;d)
  // where:
  // xa;ya;za : coordinates of  attractor
  // a        : desired size on attractor
  // b        : distance of influence of attractor
  // d        : distance until which the size remains constant
  //
  // We search the parameters in the string
  // xa
  pos1 = AttractorFunction.find(sep);
  if (pos1!=string::npos)
  xa = atof(AttractorFunction.substr(10, pos1-10).c_str());
  // ya
  pos2 = AttractorFunction.find(sep, pos1+1);
  if (pos2!=string::npos) {
  ya = atof(AttractorFunction.substr(pos1+1, pos2-pos1-1).c_str());
  pos1 = pos2;
  }
  // za
  pos2 = AttractorFunction.find(sep, pos1+1);
  if (pos2!=string::npos) {
  za = atof(AttractorFunction.substr(pos1+1, pos2-pos1-1).c_str());
  pos1 = pos2;
  }
  // a
  pos2 = AttractorFunction.find(sep, pos1+1);
  if (pos2!=string::npos) {
  a = atof(AttractorFunction.substr(pos1+1, pos2-pos1-1).c_str());
  pos1 = pos2;
  }
  // b
  pos2 = AttractorFunction.find(sep, pos1+1);
  if (pos2!=string::npos) {
  b = atof(AttractorFunction.substr(pos1+1, pos2-pos1-1).c_str());
  pos1 = pos2;
  }
  // createNode
  pos2 = AttractorFunction.find(sep, pos1+1);
  if (pos2!=string::npos) {
    string createNodeStr = AttractorFunction.substr(pos1+1, pos2-pos1-1);
    createNode = (AttractorFunction.substr(pos1+1, pos2-pos1-1) == "True");
    pos1=pos2;
  }
  // d
  pos2 = AttractorFunction.find(")");
  if (pos2!=string::npos) {
  d = atof(AttractorFunction.substr(pos1+1, pos2-pos1-1).c_str());
  }

  // Get the (u,v) values of the attractor on the face
  projectionPoint myPoint = getProjectionPoint(TopoDS::Face(GeomShape),gp_Pnt(xa,ya,za));
  gp_XY uvPoint = myPoint.uv;
  gp_XYZ xyzPoint = myPoint.xyz;
  Standard_Real u0 = uvPoint.X();
  Standard_Real v0 = uvPoint.Y();
  Standard_Real x0 = xyzPoint.X();
  Standard_Real y0 = xyzPoint.Y();
  Standard_Real z0 = xyzPoint.Z();
  std::vector<double> coords;
  coords.push_back(u0);
  coords.push_back(v0);
  coords.push_back(x0);
  coords.push_back(y0);
  coords.push_back(z0);
  // We construct the python function
  ostringstream attractorFunctionStream;
  attractorFunctionStream << "def f(u,v): return ";
  attractorFunctionStream << defaultSize << "-(" << defaultSize <<"-" << a << ")";
  //attractorFunctionStream << "*exp(-((u-("<<u0<<"))*(u-("<<u0<<"))+(v-("<<v0<<"))*(v-("<<v0<<")))/(" << b << "*" << b <<"))";
  // rnc: make possible to keep the size constant until
  // a defined distance. Distance is expressed as the positiv part
  // of r-d where r is the distance to (u0,v0)
  attractorFunctionStream << "*exp(-(0.5*(sqrt((u-"<<u0<<")**2+(v-"<<v0<<")**2)-"<<d<<"+abs(sqrt((u-"<<u0<<")**2+(v-"<<v0<<")**2)-"<<d<<"))/(" << b << "))**2)";

  int key;
  if (! FacesWithSizeMap.Contains(TopoDS::Face(GeomShape))) {
    key = FacesWithSizeMap.Add(TopoDS::Face(GeomShape));
  }
  else {
    key = FacesWithSizeMap.FindIndex(TopoDS::Face(GeomShape));
  }
  FaceId2SizeMap[key] =attractorFunctionStream.str();
  if (createNode) {
    FaceId2AttractorCoords[key] = coords;
  }
//   // Test for new attractors
//   gp_Pnt myP(xyzPoint);
//   TopoDS_Vertex myV = BRepBuilderAPI_MakeVertex(myP);
//   BLSURFPlugin_Attractor myAttractor(TopoDS::Face(GeomShape),myV,200);
//   myAttractor.SetParameters(a, defaultSize, b, d);
//   myAttractor.SetType(1);
//   FaceId2ClassAttractor[key] = myAttractor;
//   if(FaceId2ClassAttractor[key].GetFace().IsNull()){
//   }
}

// One sub-shape to get ids from
BLSURFPlugin_BLSURF::TListOfIDs _getSubShapeIDsInMainShape(const TopoDS_Shape& theMainShape,
                                                           const TopoDS_Shape& theSubShape,
                                                           TopAbs_ShapeEnum    theShapeType)
{
  BLSURFPlugin_BLSURF::TListOfIDs face_ids;

  TopTools_MapOfShape subShapes;
  TopTools_IndexedMapOfShape anIndices;
  TopExp::MapShapes(theMainShape, theShapeType, anIndices);

  for (TopExp_Explorer face_iter(theSubShape,theShapeType);face_iter.More();face_iter.Next())
  {
    if ( subShapes.Add( face_iter.Current() )) // issue 23416
    {
      int face_id = anIndices.FindIndex( face_iter.Current() );
      if ( face_id == 0 )
        throw SALOME_Exception( "Periodicity: sub_shape not found in main_shape");
      face_ids.push_back( face_id );
    }
  }
  return face_ids;
}

BLSURFPlugin_BLSURF::TListOfIDs _getSubShapeIDsInMainShape(SMESH_Mesh*      theMesh,
                                                           TopoDS_Shape     theSubShape,
                                                           TopAbs_ShapeEnum theShapeType)
{
  BLSURFPlugin_BLSURF::TListOfIDs face_ids;

  for (TopExp_Explorer face_iter(theSubShape,theShapeType);face_iter.More();face_iter.Next())
  {
    int face_id = theMesh->GetMeshDS()->ShapeToIndex(face_iter.Current());
    if (face_id == 0)
      throw SALOME_Exception ( "Periodicity: sub_shape not found in main_shape");
    face_ids.push_back(face_id);
  }

  return face_ids;
}

void BLSURFPlugin_BLSURF::addCoordsFromVertices(const std::vector<std::string> &theVerticesEntries, std::vector<double> &theVerticesCoords)
{
  for (std::vector<std::string>::const_iterator it = theVerticesEntries.begin(); it != theVerticesEntries.end(); it++)
  {
    BLSURFPlugin_Hypothesis::TEntry theVertexEntry = *it;
    addCoordsFromVertex(theVertexEntry, theVerticesCoords);
  }
}


void BLSURFPlugin_BLSURF::addCoordsFromVertex(BLSURFPlugin_Hypothesis::TEntry theVertexEntry, std::vector<double> &theVerticesCoords)
{
  if (theVertexEntry!="")
  {
    TopoDS_Shape aShape = entryToShape(theVertexEntry);

    gp_Pnt aPnt = BRep_Tool::Pnt( TopoDS::Vertex( aShape ) );
    double theX, theY, theZ;
    theX = aPnt.X();
    theY = aPnt.Y();
    theZ = aPnt.Z();

    theVerticesCoords.push_back(theX);
    theVerticesCoords.push_back(theY);
    theVerticesCoords.push_back(theZ);
  }
}

/////////////////////////////////////////////////////////
void BLSURFPlugin_BLSURF::createPreCadFacesPeriodicity(TopoDS_Shape theGeomShape, const BLSURFPlugin_Hypothesis::TPreCadPeriodicity &preCadPeriodicity)
{
  TopoDS_Shape geomShape1 = entryToShape(preCadPeriodicity.shape1Entry);
  TopoDS_Shape geomShape2 = entryToShape(preCadPeriodicity.shape2Entry);

  TListOfIDs theFace1_ids = _getSubShapeIDsInMainShape(theGeomShape, geomShape1, TopAbs_FACE);
  TListOfIDs theFace2_ids = _getSubShapeIDsInMainShape(theGeomShape, geomShape2, TopAbs_FACE);

  TPreCadPeriodicityIDs preCadFacesPeriodicityIDs;
  preCadFacesPeriodicityIDs.shape1IDs = theFace1_ids;
  preCadFacesPeriodicityIDs.shape2IDs = theFace2_ids;

  addCoordsFromVertices(preCadPeriodicity.theSourceVerticesEntries, preCadFacesPeriodicityIDs.theSourceVerticesCoords);
  addCoordsFromVertices(preCadPeriodicity.theTargetVerticesEntries, preCadFacesPeriodicityIDs.theTargetVerticesCoords);

  _preCadFacesIDsPeriodicityVector.push_back(preCadFacesPeriodicityIDs);
}

/////////////////////////////////////////////////////////
void BLSURFPlugin_BLSURF::createPreCadEdgesPeriodicity(TopoDS_Shape theGeomShape, const BLSURFPlugin_Hypothesis::TPreCadPeriodicity &preCadPeriodicity)
{
  TopoDS_Shape geomShape1 = entryToShape(preCadPeriodicity.shape1Entry);
  TopoDS_Shape geomShape2 = entryToShape(preCadPeriodicity.shape2Entry);

  TListOfIDs theEdge1_ids = _getSubShapeIDsInMainShape(theGeomShape, geomShape1, TopAbs_EDGE);
  TListOfIDs theEdge2_ids = _getSubShapeIDsInMainShape(theGeomShape, geomShape2, TopAbs_EDGE);

  TPreCadPeriodicityIDs preCadEdgesPeriodicityIDs;
  preCadEdgesPeriodicityIDs.shape1IDs = theEdge1_ids;
  preCadEdgesPeriodicityIDs.shape2IDs = theEdge2_ids;

  addCoordsFromVertices(preCadPeriodicity.theSourceVerticesEntries, preCadEdgesPeriodicityIDs.theSourceVerticesCoords);
  addCoordsFromVertices(preCadPeriodicity.theTargetVerticesEntries, preCadEdgesPeriodicityIDs.theTargetVerticesCoords);

  _preCadEdgesIDsPeriodicityVector.push_back(preCadEdgesPeriodicityIDs);
}


/////////////////////////////////////////////////////////

void BLSURFPlugin_BLSURF::SetParameters(const BLSURFPlugin_Hypothesis* hyp,
                                        cadsurf_session_t *            css,
                                        const TopoDS_Shape&            theGeomShape)
{
  // rnc : Bug 1457
  // Clear map so that it is not stored in the algorithm with old enforced vertices in it
  FacesWithSizeMap.Clear();
  FaceId2SizeMap.clear();
  EdgesWithSizeMap.Clear();
  EdgeId2SizeMap.clear();
  VerticesWithSizeMap.Clear();
  VertexId2SizeMap.clear();
  // FaceId2PythonSmp.clear();
  // EdgeId2PythonSmp.clear();
  // VertexId2PythonSmp.clear();
  FaceId2AttractorCoords.clear();
  FaceId2ClassAttractor.clear();
  FaceIndex2ClassAttractor.clear();
  FacesWithEnforcedVertices.Clear();
  FaceId2EnforcedVertexCoords.clear();
  EnfVertexCoords2ProjVertex.clear();
  EnfVertexCoords2EnfVertexList.clear();

  double diagonal               = SMESH_Mesh::GetShapeDiagonalSize( theGeomShape );
  double bbSegmentation         = _gen->GetBoundaryBoxSegmentation();
  int    _physicalMesh          = BLSURFPlugin_Hypothesis::GetDefaultPhysicalMesh();
  int    _geometricMesh         = BLSURFPlugin_Hypothesis::GetDefaultGeometricMesh();
  double _phySize               = BLSURFPlugin_Hypothesis::GetDefaultPhySize(diagonal, bbSegmentation);
  bool   _phySizeRel            = BLSURFPlugin_Hypothesis::GetDefaultPhySizeRel();
  double _minSize               = BLSURFPlugin_Hypothesis::GetDefaultMinSize(diagonal);
  bool   _minSizeRel            = BLSURFPlugin_Hypothesis::GetDefaultMinSizeRel();
  double _maxSize               = BLSURFPlugin_Hypothesis::GetDefaultMaxSize(diagonal);
  bool   _maxSizeRel            = BLSURFPlugin_Hypothesis::GetDefaultMaxSizeRel();
  double _use_gradation         = BLSURFPlugin_Hypothesis::GetDefaultUseGradation();
  double _gradation             = BLSURFPlugin_Hypothesis::GetDefaultGradation();
  double _use_volume_gradation  = BLSURFPlugin_Hypothesis::GetDefaultUseVolumeGradation();
  double _volume_gradation      = BLSURFPlugin_Hypothesis::GetDefaultVolumeGradation();
  bool   _quadAllowed           = BLSURFPlugin_Hypothesis::GetDefaultQuadAllowed();
  double _angleMesh             = BLSURFPlugin_Hypothesis::GetDefaultAngleMesh();
  double _chordalError          = BLSURFPlugin_Hypothesis::GetDefaultChordalError(diagonal);
  bool   _anisotropic           = BLSURFPlugin_Hypothesis::GetDefaultAnisotropic();
  double _anisotropicRatio      = BLSURFPlugin_Hypothesis::GetDefaultAnisotropicRatio();
  bool   _removeTinyEdges       = BLSURFPlugin_Hypothesis::GetDefaultRemoveTinyEdges();
  double _tinyEdgeLength        = BLSURFPlugin_Hypothesis::GetDefaultTinyEdgeLength(diagonal);
  bool   _optimiseTinyEdges     = BLSURFPlugin_Hypothesis::GetDefaultOptimiseTinyEdges();
  double _tinyEdgeOptimisLength = BLSURFPlugin_Hypothesis::GetDefaultTinyEdgeOptimisationLength(diagonal);
  bool   _correctSurfaceIntersec= BLSURFPlugin_Hypothesis::GetDefaultCorrectSurfaceIntersection();
  double _corrSurfaceIntersCost = BLSURFPlugin_Hypothesis::GetDefaultCorrectSurfaceIntersectionMaxCost();
  bool   _badElementRemoval     = BLSURFPlugin_Hypothesis::GetDefaultBadElementRemoval();
  double _badElementAspectRatio = BLSURFPlugin_Hypothesis::GetDefaultBadElementAspectRatio();
  bool   _optimizeMesh          = BLSURFPlugin_Hypothesis::GetDefaultOptimizeMesh();
  bool   _quadraticMesh         = BLSURFPlugin_Hypothesis::GetDefaultQuadraticMesh();
  int    _verb                  = BLSURFPlugin_Hypothesis::GetDefaultVerbosity();
  //int    _topology              = BLSURFPlugin_Hypothesis::GetDefaultTopology();

  // PreCAD
  //int _precadMergeEdges         = BLSURFPlugin_Hypothesis::GetDefaultPreCADMergeEdges();
  //int _precadRemoveDuplicateCADFaces = BLSURFPlugin_Hypothesis::GetDefaultPreCADRemoveDuplicateCADFaces();
  //int _precadProcess3DTopology  = BLSURFPlugin_Hypothesis::GetDefaultPreCADProcess3DTopology();
  //int _precadDiscardInput       = BLSURFPlugin_Hypothesis::GetDefaultPreCADDiscardInput();

  const BLSURFPlugin_Hypothesis::TPreCadPeriodicityVector preCadFacesPeriodicityVector = BLSURFPlugin_Hypothesis::GetPreCadFacesPeriodicityVector(hyp);

  if (hyp) {
    _physicalMesh  = (int) hyp->GetPhysicalMesh();
    _geometricMesh = (int) hyp->GetGeometricMesh();
    if (hyp->GetPhySize() > 0) {
      _phySize       = hyp->GetPhySize();
      // if user size is not explicitly specified, "relative" flag is ignored
      _phySizeRel    = hyp->IsPhySizeRel();
    }
    if (hyp->GetMinSize() > 0) {
      _minSize       = hyp->GetMinSize();
      // if min size is not explicitly specified, "relative" flag is ignored
      _minSizeRel    = hyp->IsMinSizeRel();
    }
    if (hyp->GetMaxSize() > 0) {
      _maxSize       = hyp->GetMaxSize();
      // if max size is not explicitly specified, "relative" flag is ignored
      _maxSizeRel    = hyp->IsMaxSizeRel();
    }
    _use_gradation = hyp->GetUseGradation();
    if (hyp->GetGradation() > 0 && _use_gradation)
      _gradation     = hyp->GetGradation();
    _use_volume_gradation    = hyp->GetUseVolumeGradation();
    if (hyp->GetVolumeGradation() > 0 && _use_volume_gradation )
      _volume_gradation      = hyp->GetVolumeGradation();
    _quadAllowed     = hyp->GetQuadAllowed();
    if (hyp->GetAngleMesh() > 0)
      _angleMesh     = hyp->GetAngleMesh();
    if (hyp->GetChordalError() > 0)
      _chordalError          = hyp->GetChordalError();
    _anisotropic             = hyp->GetAnisotropic();
    if (hyp->GetAnisotropicRatio() >= 0)
      _anisotropicRatio      = hyp->GetAnisotropicRatio();
    _removeTinyEdges         = hyp->GetRemoveTinyEdges();
    if (hyp->GetTinyEdgeLength() > 0)
      _tinyEdgeLength        = hyp->GetTinyEdgeLength();
    _optimiseTinyEdges       = hyp->GetOptimiseTinyEdges();
    if (hyp->GetTinyEdgeOptimisationLength() > 0)
      _tinyEdgeOptimisLength = hyp->GetTinyEdgeOptimisationLength();
    _correctSurfaceIntersec  = hyp->GetCorrectSurfaceIntersection();
    if (hyp->GetCorrectSurfaceIntersectionMaxCost() > 0)
      _corrSurfaceIntersCost = hyp->GetCorrectSurfaceIntersectionMaxCost();
    _badElementRemoval       = hyp->GetBadElementRemoval();
    if (hyp->GetBadElementAspectRatio() >= 0)
      _badElementAspectRatio = hyp->GetBadElementAspectRatio();
    _optimizeMesh  = hyp->GetOptimizeMesh();
    _quadraticMesh = hyp->GetQuadraticMesh();
    _verb          = hyp->GetVerbosity();
    //_topology      = (int) hyp->GetTopology();
    // PreCAD
    //_precadMergeEdges        = hyp->GetPreCADMergeEdges();
    //_precadRemoveDuplicateCADFaces = hyp->GetPreCADRemoveDuplicateCADFaces();
    //_precadProcess3DTopology = hyp->GetPreCADProcess3DTopology();
    //_precadDiscardInput      = hyp->GetPreCADDiscardInput();

    const BLSURFPlugin_Hypothesis::TOptionValues& opts = hyp->GetOptionValues();
    BLSURFPlugin_Hypothesis::TOptionValues::const_iterator opIt;
    for ( opIt = opts.begin(); opIt != opts.end(); ++opIt ){
      MESSAGE("OptionValue: " << opIt->first.c_str() << ", value: " << opIt->second.c_str());
      if ( !opIt->second.empty() ) {
		// With MeshGems 2.4-5, there are issues with periodicity and multithread
		// => As a temporary workaround, we enforce to use only one thread if periodicity is used.
        if (opIt->first == "max_number_of_threads" && opIt->second != "1" && ! preCadFacesPeriodicityVector.empty()){
          std::cout << "INFO: Disabling multithread to avoid periodicity issues" << std::endl;
          set_param(css, opIt->first.c_str(), "1");
        }
        else
          set_param(css, opIt->first.c_str(), opIt->second.c_str());
      }
    }

    const BLSURFPlugin_Hypothesis::TOptionValues& custom_opts = hyp->GetCustomOptionValues();
    for ( opIt = custom_opts.begin(); opIt != custom_opts.end(); ++opIt )
      if ( !opIt->second.empty() ) {
        set_param(css, opIt->first.c_str(), opIt->second.c_str());
     }

    const BLSURFPlugin_Hypothesis::TOptionValues& preCADopts = hyp->GetPreCADOptionValues();
    for ( opIt = preCADopts.begin(); opIt != preCADopts.end(); ++opIt )
      if ( !opIt->second.empty() ) {
        set_param(css, opIt->first.c_str(), opIt->second.c_str());
      }
  }

  if ( BLSURFPlugin_Hypothesis::HasPreCADOptions( hyp ))
  {
    cadsurf_set_param(css, "use_precad", "yes" ); // for old versions
  }
  // PreProcessor (formerly PreCAD) -- commented params are preCADoptions (since 0023307)
  //set_param(css, "merge_edges",            _precadMergeEdges ? "yes" : "no");
  //set_param(css, "remove_duplicate_cad_faces", _precadRemoveDuplicateCADFaces ? "yes" : "no");
  //set_param(css, "process_3d_topology",    _precadProcess3DTopology ? "1" : "0");
  //set_param(css, "discard_input_topology", _precadDiscardInput ? "1" : "0");
  //set_param(css, "max_number_of_points_per_patch", "1000000");
  
   bool useGradation = false;
   switch (_physicalMesh)
   {
     case BLSURFPlugin_Hypothesis::PhysicalGlobalSize:
       set_param(css, "physical_size_mode", "global");
       set_param(css, "global_physical_size", _phySizeRel ? val_to_string_rel(_phySize).c_str() : val_to_string(_phySize).c_str());
       break;
     case BLSURFPlugin_Hypothesis::PhysicalLocalSize:
       set_param(css, "physical_size_mode", "local");
       set_param(css, "global_physical_size", _phySizeRel ? val_to_string_rel(_phySize).c_str() : val_to_string(_phySize).c_str());
       useGradation = true;
       break;
     default:
       set_param(css, "physical_size_mode", "none");
   }

   switch (_geometricMesh)
   {
     case BLSURFPlugin_Hypothesis::GeometricalGlobalSize:
       set_param(css, "geometric_size_mode", "global");
       set_param(css, "geometric_approximation", val_to_string(_angleMesh).c_str());
       set_param(css, "chordal_error", val_to_string(_chordalError).c_str());
       useGradation = true;
       break;
     case BLSURFPlugin_Hypothesis::GeometricalLocalSize:
       set_param(css, "geometric_size_mode", "local");
       set_param(css, "geometric_approximation", val_to_string(_angleMesh).c_str());
       set_param(css, "chordal_error", val_to_string(_chordalError).c_str());
       useGradation = true;
       break;
     default:
       set_param(css, "geometric_size_mode", "none");
   }

   if ( hyp && hyp->GetPhySize() > 0 ) {
     // user size is explicitly specified via hypothesis parameters
     // min and max sizes should be compared with explicitly specified user size
     // - compute absolute min size
     double mins = _minSizeRel ? _minSize * diagonal : _minSize;
     // - min size should not be greater than user size
     if ( _phySize < mins )
       set_param(css, "min_size", _phySizeRel ? val_to_string_rel(_phySize).c_str() : val_to_string(_phySize).c_str());
     else
       set_param(css, "min_size", _minSizeRel ? val_to_string_rel(_minSize).c_str() : val_to_string(_minSize).c_str());
     // - compute absolute max size
     double maxs = _maxSizeRel ? _maxSize * diagonal : _maxSize;
     // - max size should not be less than user size
     if ( _phySize > maxs )
       set_param(css, "max_size", _phySizeRel ? val_to_string_rel(_phySize).c_str() : val_to_string(_phySize).c_str());
     else
       set_param(css, "max_size", _maxSizeRel ? val_to_string_rel(_maxSize).c_str() : val_to_string(_maxSize).c_str());
   }
   else {
     // user size is not explicitly specified
     // - if minsize is not explicitly specified, we pass default value computed automatically, in this case "relative" flag is ignored
     set_param(css, "min_size", _minSizeRel ? val_to_string_rel(_minSize).c_str() : val_to_string(_minSize).c_str());
     // - if maxsize is not explicitly specified, we pass default value computed automatically, in this case "relative" flag is ignored
     set_param(css, "max_size", _maxSizeRel ? val_to_string_rel(_maxSize).c_str() : val_to_string(_maxSize).c_str());
   }
   // anisotropic and quadrangle mesh requires disabling gradation
   if ( _anisotropic && _quadAllowed )
     useGradation = false; // limitation of V1.3
   if ( useGradation && _use_gradation )
     set_param(css, "gradation",                       val_to_string(_gradation).c_str());
   if ( useGradation && _use_volume_gradation )
     set_param(css, "volume_gradation",                val_to_string(_volume_gradation).c_str());
   set_param(css, "element_generation",                _quadAllowed ? "quad_dominant" : "triangle");


   set_param(css, "metric",                            _anisotropic ? "anisotropic" : "isotropic");
   if ( _anisotropic )
     set_param(css, "anisotropic_ratio",                 val_to_string(_anisotropicRatio).c_str());
   set_param(css, "remove_tiny_edges",                 _removeTinyEdges ? "1" : "0");
   if ( _removeTinyEdges )
     set_param(css, "tiny_edge_length",                  val_to_string(_tinyEdgeLength).c_str());
   set_param(css, "optimise_tiny_edges",               _optimiseTinyEdges ? "1" : "0");
   if ( _optimiseTinyEdges )
     set_param(css, "tiny_edge_optimisation_length",   val_to_string(_tinyEdgeOptimisLength).c_str());
   set_param(css, "correct_surface_intersections",     _correctSurfaceIntersec ? "1" : "0");
   if ( _correctSurfaceIntersec )
     set_param(css, "surface_intersections_processing_max_cost", val_to_string(_corrSurfaceIntersCost ).c_str());
   set_param(css, "force_bad_surface_element_removal", _badElementRemoval ? "1" : "0");
   if ( _badElementRemoval )
     set_param(css, "bad_surface_element_aspect_ratio",  val_to_string(_badElementAspectRatio).c_str());
   set_param(css, "optimisation",                      _optimizeMesh ? "yes" : "no");
   set_param(css, "element_order",                     _quadraticMesh ? "quadratic" : "linear");
   set_param(css, "verbose",                           val_to_string(_verb).c_str());

   _smp_phy_size = _phySizeRel ? _phySize*diagonal : _phySize;
   if ( _verb > 0 )
     std::cout << "_smp_phy_size = " << _smp_phy_size << std::endl;

   if (_physicalMesh == BLSURFPlugin_Hypothesis::PhysicalLocalSize)
   {
    TopoDS_Shape GeomShape;
    TopoDS_Shape AttShape;
    TopAbs_ShapeEnum GeomType;
    //
    // Standard Size Maps
    //
    const BLSURFPlugin_Hypothesis::TSizeMap sizeMaps = BLSURFPlugin_Hypothesis::GetSizeMapEntries(hyp);
    BLSURFPlugin_Hypothesis::TSizeMap::const_iterator smIt = sizeMaps.begin();
    for ( ; smIt != sizeMaps.end(); ++smIt ) {
      if ( !smIt->second.empty() ) {
        GeomShape = entryToShape(smIt->first);
        GeomType  = GeomShape.ShapeType();
        int key = -1;
        // Group Management
        if (GeomType == TopAbs_COMPOUND) {
          for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()){
            // Group of faces
            if (it.Value().ShapeType() == TopAbs_FACE){
              HasSizeMapOnFace = true;
              if (! FacesWithSizeMap.Contains(TopoDS::Face(it.Value()))) {
                key = FacesWithSizeMap.Add(TopoDS::Face(it.Value()));
              }
              else {
                key = FacesWithSizeMap.FindIndex(TopoDS::Face(it.Value()));
              }
              FaceId2SizeMap[key] = smIt->second;
            }
            // Group of edges
            if (it.Value().ShapeType() == TopAbs_EDGE){
              HasSizeMapOnEdge = true;
              HasSizeMapOnFace = true;
              if (! EdgesWithSizeMap.Contains(TopoDS::Edge(it.Value()))) {
                key = EdgesWithSizeMap.Add(TopoDS::Edge(it.Value()));
              }
              else {
                key = EdgesWithSizeMap.FindIndex(TopoDS::Edge(it.Value()));
              }
              EdgeId2SizeMap[key] = smIt->second;
            }
            // Group of vertices
            if (it.Value().ShapeType() == TopAbs_VERTEX){
              HasSizeMapOnVertex = true;
              HasSizeMapOnEdge = true;
              HasSizeMapOnFace = true;
              if (! VerticesWithSizeMap.Contains(TopoDS::Vertex(it.Value()))) {
                key = VerticesWithSizeMap.Add(TopoDS::Vertex(it.Value()));
              }
              else {
                key = VerticesWithSizeMap.FindIndex(TopoDS::Vertex(it.Value()));
              }
              VertexId2SizeMap[key] = smIt->second;
            }
          }
        }
        // Single face
        if (GeomType == TopAbs_FACE){
          HasSizeMapOnFace = true;
          if (! FacesWithSizeMap.Contains(TopoDS::Face(GeomShape))) {
            key = FacesWithSizeMap.Add(TopoDS::Face(GeomShape));
          }
          else {
            key = FacesWithSizeMap.FindIndex(TopoDS::Face(GeomShape));
          }
          FaceId2SizeMap[key] = smIt->second;
        }
        // Single edge
        if (GeomType == TopAbs_EDGE){
          HasSizeMapOnEdge = true;
          HasSizeMapOnFace = true;
          if (! EdgesWithSizeMap.Contains(TopoDS::Edge(GeomShape))) {
            key = EdgesWithSizeMap.Add(TopoDS::Edge(GeomShape));
          }
          else {
            key = EdgesWithSizeMap.FindIndex(TopoDS::Edge(GeomShape));
          }
          EdgeId2SizeMap[key] = smIt->second;
        }
        // Single vertex
        if (GeomType == TopAbs_VERTEX){
          HasSizeMapOnVertex = true;
          HasSizeMapOnEdge   = true;
          HasSizeMapOnFace   = true;
          if (! VerticesWithSizeMap.Contains(TopoDS::Vertex(GeomShape))) {
            key = VerticesWithSizeMap.Add(TopoDS::Vertex(GeomShape));
          }
          else {
            key = VerticesWithSizeMap.FindIndex(TopoDS::Vertex(GeomShape));
          }
          VertexId2SizeMap[key] = smIt->second;
        }
      }
    }

    //
    // Attractors
    //
    // TODO appeler le constructeur des attracteurs directement ici
//     if ( !_phySizeRel ) {
      const BLSURFPlugin_Hypothesis::TSizeMap attractors = BLSURFPlugin_Hypothesis::GetAttractorEntries(hyp);
      BLSURFPlugin_Hypothesis::TSizeMap::const_iterator atIt = attractors.begin();
      for ( ; atIt != attractors.end(); ++atIt ) {
        if ( !atIt->second.empty() ) {
          GeomShape = entryToShape(atIt->first);
          GeomType  = GeomShape.ShapeType();
          // Group Management
          if (GeomType == TopAbs_COMPOUND){
            for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()){
              if (it.Value().ShapeType() == TopAbs_FACE){
                HasSizeMapOnFace = true;
                createAttractorOnFace(it.Value(), atIt->second, _phySizeRel ? _phySize*diagonal : _phySize);
              }
            }
          }

          if (GeomType == TopAbs_FACE){
            HasSizeMapOnFace = true;
            createAttractorOnFace(GeomShape, atIt->second, _phySizeRel ? _phySize*diagonal : _phySize);
          }
  /*
          if (GeomType == TopAbs_EDGE){
            HasSizeMapOnEdge = true;
            HasSizeMapOnFace = true;
          EdgeId2SizeMap[TopoDS::Edge(GeomShape).HashCode(IntegerLast())] = atIt->second;
          }
          if (GeomType == TopAbs_VERTEX){
            HasSizeMapOnVertex = true;
            HasSizeMapOnEdge   = true;
            HasSizeMapOnFace   = true;
          VertexId2SizeMap[TopoDS::Vertex(GeomShape).HashCode(IntegerLast())] = atIt->second;
          }
  */
        }
      }
//     }

    // Class Attractors
    // temporary commented out for testing
    // TODO
    //  - Fill in the BLSURFPlugin_Hypothesis::TAttractorMap map in the hypothesis
    //  - Uncomment and complete this part to construct the attractors from the attractor shape and the passed parameters on each face of the map
    //  - To do this use the public methodss: SetParameters(several double parameters) and SetType(int type)
    //  OR, even better:
    //  - Construct the attractors with an empty dist. map in the hypothesis
    //  - build the map here for each face with an attractor set and only if the attractor shape as changed since the last call to _buildmap()
    //  -> define a bool _mapbuilt in the class that is set to false by default and set to true when calling _buildmap()  OK

      theNbAttractors = 0;
    const BLSURFPlugin_Hypothesis::TAttractorMap class_attractors = BLSURFPlugin_Hypothesis::GetClassAttractorEntries(hyp);
    int key=-1;
    BLSURFPlugin_Hypothesis::TAttractorMap::const_iterator AtIt = class_attractors.begin();
    for ( ; AtIt != class_attractors.end(); ++AtIt ) {
      if ( !AtIt->second->Empty() ) {
        GeomShape = entryToShape(AtIt->first);
        if ( !SMESH_MesherHelper::IsSubShape( GeomShape, theGeomShape ))
          continue;
        AttShape = AtIt->second->GetAttractorShape();
        GeomType  = GeomShape.ShapeType();
        // Group Management
//         if (GeomType == TopAbs_COMPOUND){
//           for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()){
//             if (it.Value().ShapeType() == TopAbs_FACE){
//               HasAttractorOnFace = true;
//               myAttractor = BLSURFPluginAttractor(GeomShape, AttShape);
//             }
//           }
//         }

        if (GeomType == TopAbs_FACE
          && (AttShape.ShapeType() == TopAbs_VERTEX
           || AttShape.ShapeType() == TopAbs_EDGE
           || AttShape.ShapeType() == TopAbs_WIRE
           || AttShape.ShapeType() == TopAbs_COMPOUND) ){
            HasSizeMapOnFace = true;

            key = FacesWithSizeMap.Add(TopoDS::Face(GeomShape) );

            FaceId2ClassAttractor[key].push_back( AtIt->second );
            ++theNbAttractors;
        }
        else{
          MESSAGE("Wrong shape type !!")
        }

      }
    }


    //
    // Enforced Vertices
    //
    const BLSURFPlugin_Hypothesis::TFaceEntryEnfVertexListMap entryEnfVertexListMap = BLSURFPlugin_Hypothesis::GetAllEnforcedVerticesByFace(hyp);
    BLSURFPlugin_Hypothesis::TFaceEntryEnfVertexListMap::const_iterator enfIt = entryEnfVertexListMap.begin();
    for ( ; enfIt != entryEnfVertexListMap.end(); ++enfIt ) {
      if ( !enfIt->second.empty() ) {
        GeomShape = entryToShape(enfIt->first);
        if ( GeomShape.IsNull() )
        {
          createEnforcedVertexOnFace( GeomShape, enfIt->second );
        }
        // Group Management
        else if ( GeomShape.ShapeType() == TopAbs_COMPOUND)
        {
          for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()){
            if (it.Value().ShapeType() == TopAbs_FACE){
              HasSizeMapOnFace = true;
              createEnforcedVertexOnFace(it.Value(), enfIt->second);
            }
          }
        }
        else if ( GeomShape.ShapeType() == TopAbs_FACE)
        {
          HasSizeMapOnFace = true;
          createEnforcedVertexOnFace(GeomShape, enfIt->second);
        }
      }
    }

    // Internal vertices
    bool useInternalVertexAllFaces = BLSURFPlugin_Hypothesis::GetInternalEnforcedVertexAllFaces(hyp);
    if (useInternalVertexAllFaces) {
      std::string grpName = BLSURFPlugin_Hypothesis::GetInternalEnforcedVertexAllFacesGroup(hyp);
      gp_Pnt aPnt;
      TopExp_Explorer exp (theGeomShape, TopAbs_FACE);
      for (; exp.More(); exp.Next()){
        TopExp_Explorer exp_face (exp.Current(), TopAbs_VERTEX, TopAbs_EDGE);
        for (; exp_face.More(); exp_face.Next())
        {
          // Get coords of vertex
          // Check if current coords is already in enfVertexList
          // If coords not in enfVertexList, add new enfVertex
          aPnt = BRep_Tool::Pnt(TopoDS::Vertex(exp_face.Current()));
          BLSURFPlugin_Hypothesis::TEnfVertex* enfVertex = new BLSURFPlugin_Hypothesis::TEnfVertex();
          enfVertex->coords.push_back(aPnt.X());
          enfVertex->coords.push_back(aPnt.Y());
          enfVertex->coords.push_back(aPnt.Z());
          enfVertex->name = "";
          enfVertex->faceEntries.clear();
          enfVertex->geomEntry = "";
          enfVertex->grpName = grpName;
          enfVertex->vertex = TopoDS::Vertex( exp_face.Current() );
          _createEnforcedVertexOnFace( TopoDS::Face(exp.Current()),  aPnt, enfVertex);
          HasSizeMapOnFace = true;
        }
      }
    }

    cadsurf_set_sizemap_iso_cad_face(css, size_on_surface, &_smp_phy_size);

    if (HasSizeMapOnEdge){
      cadsurf_set_sizemap_iso_cad_edge(css, size_on_edge, &_smp_phy_size);
    }
    if (HasSizeMapOnVertex){
      cadsurf_set_sizemap_iso_cad_point(css, size_on_vertex, &_smp_phy_size);
    }
  }

  // PERIODICITY

   // reset vectors
   _preCadFacesIDsPeriodicityVector.clear();
   _preCadEdgesIDsPeriodicityVector.clear();

  for (std::size_t i = 0; i<preCadFacesPeriodicityVector.size(); i++){
    createPreCadFacesPeriodicity(theGeomShape, preCadFacesPeriodicityVector[i]);
  }

  const BLSURFPlugin_Hypothesis::TPreCadPeriodicityVector preCadEdgesPeriodicityVector = BLSURFPlugin_Hypothesis::GetPreCadEdgesPeriodicityVector(hyp);

  for (std::size_t i = 0; i<preCadEdgesPeriodicityVector.size(); i++){
    createPreCadEdgesPeriodicity(theGeomShape, preCadEdgesPeriodicityVector[i]);
  }
}

//================================================================================
/*!
 * \brief Throws an exception if a parameter name is wrong
 */
//================================================================================

void BLSURFPlugin_BLSURF::set_param(cadsurf_session_t *css,
                                    const char *       option_name,
                                    const char *       option_value)
{
  status_t status = cadsurf_set_param(css, option_name, option_value );

  if ( _hypothesis && _hypothesis->GetVerbosity() > _hypothesis->GetDefaultVerbosity() )
    cout << option_name << " = " << option_value << endl;

  if ( status != MESHGEMS_STATUS_OK )
  {
    if ( status == MESHGEMS_STATUS_UNKNOWN_PARAMETER ) {
      throw SALOME_Exception
        ( SMESH_Comment("Invalid name of CADSURF parameter: ") << option_name );
    }
    else if ( status == MESHGEMS_STATUS_NOLICENSE )
      throw SALOME_Exception
        ( "No valid license available" );
    else
      throw SALOME_Exception
        ( SMESH_Comment("Either wrong name or unacceptable value of CADSURF parameter '")
          << option_name << "': " << option_value);
  }
}

namespace
{
  // --------------------------------------------------------------------------
  /*!
   * \brief Class correctly terminating usage of MG-CADSurf library at destruction
   */
  struct BLSURF_Cleaner
  {
    context_t *        _ctx;
    cadsurf_session_t* _css;
    cad_t *            _cad;
    dcad_t *           _dcad;

    BLSURF_Cleaner(context_t *        ctx,
                   cadsurf_session_t* css=0,
                   cad_t *            cad=0,
                   dcad_t *           dcad=0)
      : _ctx ( ctx  ),
        _css ( css  ),
        _cad ( cad  ),
        _dcad( dcad )
    {
    }
    ~BLSURF_Cleaner()
    {
      Clean( /*exceptContext=*/false );
    }
    void Clean(const bool exceptContext)
    {
      if ( _css )
      {
        cadsurf_session_delete(_css); _css = 0;

        // #if BLSURF_VERSION_LONG >= "3.1.1"
        // //     if(geo_sizemap_e)
        // //       distene_sizemap_delete(geo_sizemap_e);
        // //     if(geo_sizemap_f)
        // //       distene_sizemap_delete(geo_sizemap_f);
        //     if(iso_sizemap_p)
        //       distene_sizemap_delete(iso_sizemap_p);
        //     if(iso_sizemap_e)
        //       distene_sizemap_delete(iso_sizemap_e);
        //     if(iso_sizemap_f)
        //       distene_sizemap_delete(iso_sizemap_f);
        // 
        // //     if(clean_geo_sizemap_e)
        // //       distene_sizemap_delete(clean_geo_sizemap_e);
        // //     if(clean_geo_sizemap_f)
        // //       distene_sizemap_delete(clean_geo_sizemap_f);
        //     if(clean_iso_sizemap_p)
        //       distene_sizemap_delete(clean_iso_sizemap_p);
        //     if(clean_iso_sizemap_e)
        //       distene_sizemap_delete(clean_iso_sizemap_e);
        //     if(clean_iso_sizemap_f)
        //       distene_sizemap_delete(clean_iso_sizemap_f);
        // #endif

        cad_delete(_cad); _cad = 0;
        dcad_delete(_dcad); _dcad = 0;
        if ( !exceptContext )
        {
          context_delete(_ctx); _ctx = 0;
        }
      }
    }
  };

  // --------------------------------------------------------------------------
  // comparator to sort nodes and sub-meshes
  struct ShapeTypeCompare
  {
    // sort nodes by position in the following order:
    // SMDS_TOP_FACE=2, SMDS_TOP_EDGE=1, SMDS_TOP_VERTEX=0, SMDS_TOP_3DSPACE=3
    bool operator()( const SMDS_MeshNode* n1, const SMDS_MeshNode* n2 ) const
    {
      // NEW ORDER: nodes earlier added to sub-mesh are considered "less"
      return n1->getIdInShape() < n2->getIdInShape();
      // SMDS_TypeOfPosition pos1 = n1->GetPosition()->GetTypeOfPosition();
      // SMDS_TypeOfPosition pos2 = n2->GetPosition()->GetTypeOfPosition();
      // if ( pos1 == pos2 ) return 0;
      // if ( pos1 < pos2 || pos1 == SMDS_TOP_3DSPACE ) return 1;
      // return -1;
    }
    // sort sub-meshes in order: EDGE, VERTEX
    bool operator()( const SMESHDS_SubMesh* s1, const SMESHDS_SubMesh* s2 ) const
    {
      int isVertex1 = ( s1 && s1->NbElements() == 0 );
      int isVertex2 = ( s2 && s2->NbElements() == 0 );
      if ( isVertex1 == isVertex2 )
        return s1 < s2;
      return isVertex1 < isVertex2;
    }
  };

  //================================================================================
  /*!
   * \brief Fills groups of nodes to be merged
   */
  //================================================================================

  void getNodeGroupsToMerge( const SMESHDS_SubMesh*                smDS,
                             const TopoDS_Shape&                   shape,
                             SMESH_MeshEditor::TListOfListOfNodes& nodeGroupsToMerge)
  {
    SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
    switch ( shape.ShapeType() )
    {
    case TopAbs_VERTEX: {
      std::list< const SMDS_MeshNode* > nodes;
      while ( nIt->more() )
        nodes.push_back( nIt->next() );
      if ( nodes.size() > 1 )
        nodeGroupsToMerge.push_back( nodes );
      break;
    }
    case TopAbs_EDGE: {
      std::multimap< double, const SMDS_MeshNode* > u2node;
      const SMDS_EdgePosition* ePos;
      while ( nIt->more() )
      {
        const SMDS_MeshNode* n = nIt->next();
        if (( ePos = dynamic_cast< const SMDS_EdgePosition* >( n->GetPosition() )))
          u2node.insert( make_pair( ePos->GetUParameter(), n ));
      }
      if ( u2node.size() < 2 ) return;

      //double tol = (( u2node.rbegin()->first - u2node.begin()->first ) / 20.) / u2node.size();
      Standard_Real f,l;
      BRep_Tool::Range( TopoDS::Edge( shape ), f,l );
      double tol = (( l - f ) / 20.) / u2node.size();

      std::multimap< double, const SMDS_MeshNode* >::iterator un2, un1;
      for ( un2 = u2node.begin(), un1 = un2++; un2 != u2node.end(); un1 = un2++ )
      {
        if (( un2->first - un1->first ) <= tol )
        {
          std::list< const SMDS_MeshNode* > nodes;
          nodes.push_back( un1->second );
          while (( un2->first - un1->first ) <= tol )
          {
            nodes.push_back( un2->second );
            if ( ++un2 == u2node.end()) {
              --un2;
              break;
            }
          }
          // make nodes created on the boundary of viscous layer replace nodes created
          // by MG-CADSurf as their SMDS_Position is more correct
          nodes.sort( ShapeTypeCompare() );
          nodeGroupsToMerge.push_back( nodes );
        }
      }
      break;
    }
    default: ;
    }
    // SMESH_MeshEditor::TListOfListOfNodes::const_iterator nll = nodeGroupsToMerge.begin();
    // for ( ; nll != nodeGroupsToMerge.end(); ++nll )
    // {
    //   cout << "Merge ";
    //   const std::list< const SMDS_MeshNode* >& nl = *nll;
    //   std::list< const SMDS_MeshNode* >::const_iterator nIt = nl.begin();
    //   for ( ; nIt != nl.end(); ++nIt )
    //     cout << (*nIt) << " ";
    //   cout << endl;
    // }
    // cout << endl;
  }

  //================================================================================
  /*!
   * \brief A temporary mesh used to compute mesh on a proxy FACE
   */
  //================================================================================

  struct TmpMesh: public SMESH_Mesh
  {
    typedef std::map<const SMDS_MeshNode*, const SMDS_MeshNode*, TIDCompare > TN2NMap;
    TN2NMap     _tmp2origNN;
    TopoDS_Face _proxyFace;

    TmpMesh()
    {
      _myMeshDS = new SMESHDS_Mesh( _id, true );
    }
    //--------------------------------------------------------------------------------
    /*!
     * \brief Creates a FACE bound by viscous layers and mesh each its EDGE with 1 segment
     */
    //--------------------------------------------------------------------------------

    const TopoDS_Face& makeProxyFace( SMESH_ProxyMesh::Ptr& viscousMesh,
                                      const TopoDS_Face&    origFace)
    {
      SMESH_Mesh* origMesh = viscousMesh->GetMesh();

      SMESH_MesherHelper helper( *origMesh );
      helper.SetSubShape( origFace );
      const bool hasSeam = helper.HasRealSeam();

      // get data of nodes on inner boundary of viscous layers
      TError err;
      TSideVector wireVec = StdMeshers_FaceSide::GetFaceWires(origFace, *origMesh,
                                                              /*skipMediumNodes = */true,
                                                              err, &helper, viscousMesh );
      if ( err && err->IsKO() )
        throw *err.get(); // it should be caught at SMESH_subMesh

      // proxy nodes and corresponding tmp VERTEXes
      std::vector<const SMDS_MeshNode*> origNodes;
      std::vector<TopoDS_Vertex>        tmpVertex;

      // create a proxy FACE
      TopoDS_Face origFaceCopy = TopoDS::Face( origFace.EmptyCopied() );
      BRepBuilderAPI_MakeFace newFace( origFaceCopy );
      bool hasPCurves = false;
      for ( size_t iW = 0; iW != wireVec.size(); ++iW )
      {
        StdMeshers_FaceSidePtr& wireData = wireVec[iW];
        const UVPtStructVec&  wirePoints = wireData->GetUVPtStruct();
        if ( wirePoints.size() < 3 )
          continue;

        BRepBuilderAPI_MakePolygon polygon;
        const size_t i0 = tmpVertex.size();
        for ( size_t iN = 0; iN < wirePoints.size(); ++iN )
        {
          polygon.Add( SMESH_TNodeXYZ( wirePoints[ iN ].node ));
          origNodes.push_back( wirePoints[ iN ].node );
          tmpVertex.push_back( polygon.LastVertex() );

          // check presence of a pcurve
          checkPCurve( polygon, origFaceCopy, hasPCurves, &wirePoints[ iN-1 ] );
        }
        tmpVertex[ i0 ] = polygon.FirstVertex(); // polygon.LastVertex()==NULL for 1 vertex in wire
        polygon.Close();
        if ( !polygon.IsDone() )
          throw SALOME_Exception("BLSURFPlugin_BLSURF: BRepBuilderAPI_MakePolygon failed");
        TopoDS_Wire wire = polygon;
        if ( hasSeam )
          wire = updateSeam( wire, origNodes );
        newFace.Add( wire );
      }
      _proxyFace = newFace;

      // set a new shape to mesh
      TopoDS_Compound auxCompoundToMesh;
      BRep_Builder shapeBuilder;
      shapeBuilder.MakeCompound( auxCompoundToMesh );
      shapeBuilder.Add( auxCompoundToMesh, _proxyFace );
      shapeBuilder.Add( auxCompoundToMesh, origMesh->GetShapeToMesh() );

      ShapeToMesh( auxCompoundToMesh );


      // Make input mesh for MG-CADSurf: segments on EDGE's of newFace

      // make nodes and fill in _tmp2origNN
      //
      SMESHDS_Mesh* tmpMeshDS = GetMeshDS();
      for ( size_t i = 0; i < origNodes.size(); ++i )
      {
        GetSubMesh( tmpVertex[i] )->ComputeStateEngine( SMESH_subMesh::COMPUTE );
        if ( const SMDS_MeshNode* tmpN = SMESH_Algo::VertexNode( tmpVertex[i], tmpMeshDS ))
          _tmp2origNN.insert( _tmp2origNN.end(), make_pair( tmpN, origNodes[i] ));
        // else -- it can be a seam vertex replaced by updateSeam()
        //   throw SALOME_Exception("BLSURFPlugin_BLSURF: a proxy vertex not meshed");
      }

      // make segments
      TopoDS_Vertex v1, v2;
      for ( TopExp_Explorer edge( _proxyFace, TopAbs_EDGE ); edge.More(); edge.Next() )
      {
        const TopoDS_Edge& E = TopoDS::Edge( edge.Current() );
        TopExp::Vertices( E, v1, v2 );
        const SMDS_MeshNode* n1 = SMESH_Algo::VertexNode( v1, tmpMeshDS );
        const SMDS_MeshNode* n2 = SMESH_Algo::VertexNode( v2, tmpMeshDS );

        if ( SMDS_MeshElement* seg = tmpMeshDS->AddEdge( n1, n2 ))
          tmpMeshDS->SetMeshElementOnShape( seg, E );
      }

      return _proxyFace;
    }

    //--------------------------------------------------------------------------------
    /*!
     * \brief Add pcurve to the last edge of a wire
     */
    //--------------------------------------------------------------------------------

    void checkPCurve( BRepBuilderAPI_MakePolygon& wire,
                      const TopoDS_Face&          face,
                      bool &                      hasPCurves,
                      const uvPtStruct *          wirePoints )
    {
      if ( hasPCurves )
        return;
      TopoDS_Edge edge = wire.Edge();
      if ( edge.IsNull() ) return;
      double f,l;
      if ( BRep_Tool::CurveOnSurface(edge, face, f, l))
      {
        hasPCurves = true;
        return;
      }
      gp_XY p1 = wirePoints[ 0 ].UV(), p2 = wirePoints[ 1 ].UV();
      Handle(Geom2d_Line) pcurve = new Geom2d_Line( p1, gp_Dir2d( p2 - p1 ));
      BRep_Builder().UpdateEdge( edge, Handle(Geom_Curve)(), Precision::Confusion() );
      BRep_Builder().UpdateEdge( edge, pcurve, face, Precision::Confusion() );
      BRep_Builder().Range( edge, 0, ( p2 - p1 ).Modulus() );
      // cout << "n1 = mesh.AddNode( " << p1.X()*10 << ", " << p1.Y() << ", 0 )" << endl
      //      << "n2 = mesh.AddNode( " << p2.X()*10 << ", " << p2.Y() << ", 0 )" << endl
      //      << "mesh.AddEdge( [ n1, n2 ] )" << endl;
    }

    //--------------------------------------------------------------------------------
    /*!
     * \brief Replace coincident EDGEs with reversed copies.
     */
    //--------------------------------------------------------------------------------

    TopoDS_Wire updateSeam( const TopoDS_Wire&                       wire,
                            const std::vector<const SMDS_MeshNode*>& nodesOfVertices )
    {
      BRepBuilderAPI_MakeWire newWire;

      typedef NCollection_DataMap<SMESH_TLink, TopoDS_Edge, SMESH_TLink > TSeg2EdgeMap;
      TSeg2EdgeMap seg2EdgeMap;

      TopoDS_Iterator edgeIt( wire );
      for ( int iSeg = 1; edgeIt.More(); edgeIt.Next(), ++iSeg )
      {
        SMESH_TLink link( nodesOfVertices[ iSeg-1 ], nodesOfVertices[ iSeg ]);
        TopoDS_Edge edge( TopoDS::Edge( edgeIt.Value() ));

        TopoDS_Edge* edgeInMap = seg2EdgeMap.Bound( link, edge );
        bool            isSeam = ( *edgeInMap != edge );
        if ( isSeam )
        {
          edgeInMap->Reverse();
          edge = *edgeInMap;
        }
        newWire.Add( edge );
      }
      return newWire;
    }

    //--------------------------------------------------------------------------------
    /*!
     * \brief Fill in the origMesh with faces computed by MG-CADSurf in this tmp mesh
     */
    //--------------------------------------------------------------------------------

    void FillInOrigMesh( SMESH_Mesh&        origMesh,
                         const TopoDS_Face& origFace )
    {
      SMESH_MesherHelper helper( origMesh );
      helper.SetSubShape( origFace );
      helper.SetElementsOnShape( true );

      SMESH_MesherHelper tmpHelper( *this );
      tmpHelper.SetSubShape( _proxyFace );

      // iterate over tmp faces and copy them in origMesh
      const SMDS_MeshNode* nodes[27];
      const SMDS_MeshNode* nullNode = 0;
      double xyz[3];
      SMDS_FaceIteratorPtr fIt = GetMeshDS()->facesIterator(/*idInceasingOrder=*/true);
      while ( fIt->more() )
      {
        const SMDS_MeshElement* f = fIt->next();
        SMDS_ElemIteratorPtr nIt = f->nodesIterator();
        int nbN = 0;
        for ( ; nIt->more(); ++nbN )
        {
          const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nIt->next() );
          TN2NMap::iterator n2nIt =
            _tmp2origNN.insert( _tmp2origNN.end(), make_pair( n, nullNode ));
          if ( !n2nIt->second ) {
            n->GetXYZ( xyz );
            gp_XY uv = tmpHelper.GetNodeUV( _proxyFace, n );
            n2nIt->second = helper.AddNode( xyz[0], xyz[1], xyz[2], uv.X(), uv.Y() );
          }
          nodes[ nbN ] = n2nIt->second;
        }
        switch( nbN ) {
        case 3: helper.AddFace( nodes[0], nodes[1], nodes[2] ); break;
          // case 6: helper.AddFace( nodes[0], nodes[1], nodes[2],
          //                         nodes[3], nodes[4], nodes[5]); break;
        case 4: helper.AddFace( nodes[0], nodes[1], nodes[2], nodes[3] ); break;
        // case 9: helper.AddFace( nodes[0], nodes[1], nodes[2], nodes[3],
        //                         nodes[4], nodes[5], nodes[6], nodes[7], nodes[8]); break;
        // case 8: helper.AddFace( nodes[0], nodes[1], nodes[2], nodes[3],
        //                         nodes[4], nodes[5], nodes[6], nodes[7]); break;
        }
      }
    }
  };

  /*!
   * \brief A structure holding an error description and a verbisity level
   */
  struct message_cb_user_data
  {
    std::string * _error;
    int           _verbosity;
    double *      _progress;
  };

} // namespace

status_t curv_fun(real t, real *uv, real *dt, real *dtt, void *user_data);
status_t surf_fun(real *uv, real *xyz, real*du, real *dv,
                  real *duu, real *duv, real *dvv, void *user_data);
status_t message_cb(message_t *msg, void *user_data);
status_t interrupt_cb(integer *interrupt_status, void *user_data);

//=============================================================================
/*!
 *
 */
//=============================================================================

bool BLSURFPlugin_BLSURF::Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape)
{
  // Fix problem with locales
  Kernel_Utils::Localizer aLocalizer;

  this->SMESH_Algo::_progress = 1e-3; // prevent progress advancment while computing attractors

  bool viscousLayersMade =
    ( aShape.ShapeType() == TopAbs_FACE &&
      StdMeshers_ViscousLayers2D::HasProxyMesh( TopoDS::Face( aShape ), aMesh ));

  if ( !viscousLayersMade )
    if ( !compute( aMesh, aShape, /*allowSubMeshClearing=*/true ))
      return false;

  if ( _haveViscousLayers || viscousLayersMade )
  {
    // Compute viscous layers

    TopTools_MapOfShape map;
    for (TopExp_Explorer face_iter(aShape,TopAbs_FACE);face_iter.More();face_iter.Next())
    {
      const TopoDS_Face& F = TopoDS::Face(face_iter.Current());
      if ( !map.Add( F )) continue;
      SMESH_ProxyMesh::Ptr viscousMesh = StdMeshers_ViscousLayers2D::Compute( aMesh, F );
      if ( !viscousMesh )
        return false; // error in StdMeshers_ViscousLayers2D::Compute()

      // Compute MG-CADSurf mesh on viscous layers

      if ( viscousMesh->NbProxySubMeshes() > 0 )
      {
        TmpMesh tmpMesh;
        const TopoDS_Face& proxyFace = tmpMesh.makeProxyFace( viscousMesh, F );
        if ( !compute( tmpMesh, proxyFace, /*allowSubMeshClearing=*/false ))
          return false;
        tmpMesh.FillInOrigMesh( aMesh, F );
      }
    }

    // Re-compute MG-CADSurf mesh on the rest faces if the mesh was cleared

    for (TopExp_Explorer face_iter(aShape,TopAbs_FACE);face_iter.More();face_iter.Next())
    {
      const TopoDS_Face& F = TopoDS::Face(face_iter.Current());
      SMESH_subMesh* fSM = aMesh.GetSubMesh( F );
      if ( fSM->IsMeshComputed() ) continue;

      if ( !compute( aMesh, aShape, /*allowSubMeshClearing=*/true ))
        return false;
      break;
    }
  }
  return true;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool BLSURFPlugin_BLSURF::compute(SMESH_Mesh&         aMesh,
                                  const TopoDS_Shape& aShape,
                                  bool                allowSubMeshClearing)
{
  /* create a distene context (generic object) */
  status_t status = STATUS_ERROR;

  SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();
  SMESH_MesherHelper helper( aMesh ), helperWithShape( aMesh );
  myHelper = theHelper = & helperWithShape;
  // do not call helper.IsQuadraticSubMesh() because sub-meshes
  // may be cleaned and helper.myTLinkNodeMap gets invalid in such a case
  bool haveQuadraticSubMesh = helperWithShape.IsQuadraticSubMesh( aShape );
  bool quadraticSubMeshAndViscousLayer = false;
  bool needMerge = false;
  typedef set< SMESHDS_SubMesh*, ShapeTypeCompare > TSubMeshSet;
  TSubMeshSet edgeSubmeshes;
  TSubMeshSet& mergeSubmeshes = edgeSubmeshes;

  TopTools_IndexedMapOfShape pmap, emap, fmap;

  TopTools_IndexedDataMapOfShapeListOfShape e2ffmap;
  TopExp::MapShapesAndAncestors( aShape, TopAbs_EDGE, TopAbs_FACE, e2ffmap );

  // Issue 0019864. On DebianSarge, FE signals do not obey to OSD::SetSignal(false)
#ifndef WIN32
  feclearexcept( FE_ALL_EXCEPT );
  int oldFEFlags = fedisableexcept( FE_ALL_EXCEPT );
#endif

  context_t *ctx =  context_new();

  /* Set the message callback in the working context */
  message_cb_user_data mcud;
  mcud._error     = & this->SMESH_Algo::_comment;
  mcud._progress  = & this->SMESH_Algo::_progress;
  mcud._verbosity =
    _hypothesis ? _hypothesis->GetVerbosity() : BLSURFPlugin_Hypothesis::GetDefaultVerbosity();
  context_set_message_callback(ctx, message_cb, &mcud);

  /* set the interruption callback */
  _compute_canceled = false;
  context_set_interrupt_callback(ctx, interrupt_cb, this);

  /* create the CAD object we will work on. It is associated to the context ctx. */
  cad_t *c     = cad_new(ctx);
  dcad_t *dcad = dcad_new(c);

  // To enable multithreading
  cad_set_thread_safety(c, 1);

  /* Now fill the CAD object with data from your CAD
   * environement. This is the most complex part of a successfull
   * integration.
   */

  // PreCAD

  cadsurf_session_t *css = cadsurf_session_new(ctx);

  // an object that correctly deletes all cadsurf objects at destruction
  BLSURF_Cleaner cleaner( ctx,css,c,dcad );

  SetParameters(_hypothesis, css, aShape);

  haveQuadraticSubMesh = haveQuadraticSubMesh || (_hypothesis != NULL && _hypothesis->GetQuadraticMesh());
  helper.SetIsQuadratic( haveQuadraticSubMesh );

  // To remove as soon as quadratic mesh is allowed - BEGIN
  // GDD: Viscous layer is not allowed with quadratic mesh
  if (_haveViscousLayers && haveQuadraticSubMesh ) {
    quadraticSubMeshAndViscousLayer = true;
    _haveViscousLayers = !haveQuadraticSubMesh;
    _comment += "Warning: Viscous layer is not possible with a quadratic mesh, it is ignored.";
    error(COMPERR_WARNING, _comment);
  }
  // To remove as soon as quadratic mesh is allowed - END

  // needed to prevent the opencascade memory managmement from freeing things
  vector<Handle(Geom2d_Curve)> curves;
  vector<Handle(Geom_Surface)> surfaces;

  emap.Clear();
  pmap.Clear();
  // FaceId2PythonSmp.clear();
  // EdgeId2PythonSmp.clear();
  // VertexId2PythonSmp.clear();

  /****************************************************************************************
                                          FACES
  *****************************************************************************************/
  int iface = 0;
  string bad_end = "return";
  int faceKey = -1;
  TopTools_IndexedMapOfShape _map;
  TopExp::MapShapes(aShape,TopAbs_VERTEX,_map);
  int ienf = _map.Extent();

  // assert(Py_IsInitialized());
  // PyGILState_STATE gstate;

  string theSizeMapStr;

  for (TopExp_Explorer face_iter(aShape,TopAbs_FACE);face_iter.More();face_iter.Next())
  {
    TopoDS_Face f = TopoDS::Face(face_iter.Current());

    SMESH_subMesh* fSM = aMesh.GetSubMesh( f );
    if ( !fSM->IsEmpty() ) continue; // skip already meshed FACE with viscous layers

    // make INTERNAL face oriented FORWARD (issue 0020993)
    if (f.Orientation() != TopAbs_FORWARD && f.Orientation() != TopAbs_REVERSED )
      f.Orientation(TopAbs_FORWARD);

    iface = fmap.Add(f);

    surfaces.push_back(BRep_Tool::Surface(f));

    /* create an object representing the face for cadsurf */
    /* where face_id is an integer identifying the face.
     * surf_function is the function that defines the surface
     * (For this face, it will be called by cadsurf with your_face_object_ptr
     * as last parameter.
     */
#if OCC_VERSION_MAJOR < 7
    cad_face_t *fce = cad_face_new(c, iface, surf_fun, surfaces.back());
#else
    cad_face_t *fce = cad_face_new(c, iface, surf_fun, surfaces.back().get());
#endif

    /* by default a face has no tag (color).
       The following call sets it to the same value as the Geom module ID : */
    int faceTag = meshDS->ShapeToIndex(f);
    faceTag = BLSURFPlugin_Hypothesis::GetHyperPatchTag( faceTag, _hypothesis );
    cad_face_set_tag(fce, faceTag);

    /* Set face orientation (optional if you want a well oriented output mesh)*/
    if(f.Orientation() != TopAbs_FORWARD)
      cad_face_set_orientation(fce, CAD_ORIENTATION_REVERSED);
    else
      cad_face_set_orientation(fce, CAD_ORIENTATION_FORWARD);

    if (HasSizeMapOnFace /*&& !use_precad*/) //22903: use_precad seems not to interfere
    {
      // -----------------
      // Classic size map
      // -----------------
      faceKey = FacesWithSizeMap.FindIndex(f);


      if (FaceId2SizeMap.find(faceKey)!=FaceId2SizeMap.end())
      {
        theSizeMapStr = FaceId2SizeMap[faceKey];
        // check if function ends with "return"
        if (theSizeMapStr.find(bad_end) == (theSizeMapStr.size()-bad_end.size()-1))
          continue;
        // Expr To Python function, verification is performed at validation in GUI
		throw SALOME_Exception("Python expression not supported for size map.");
		/*
        gstate = PyGILState_Ensure();
        PyObject * obj = NULL;
        obj= PyRun_String(theSizeMapStr.c_str(), Py_file_input, main_dict, NULL);
        Py_DECREF(obj);
        PyObject * func = NULL;
        func = PyObject_GetAttrString(main_mod, "f");
        FaceId2PythonSmp[iface]=func;
        FaceId2SizeMap.erase(faceKey);
        PyGILState_Release(gstate);
		*/
      }

      // Specific size map = Attractor
      std::map<int,std::vector<double> >::iterator attractor_iter = FaceId2AttractorCoords.begin();

      for (; attractor_iter != FaceId2AttractorCoords.end(); ++attractor_iter) {
        if (attractor_iter->first == faceKey)
        {
          double xyzCoords[3]  = {attractor_iter->second[2],
                                  attractor_iter->second[3],
                                  attractor_iter->second[4]};

          gp_Pnt P(xyzCoords[0],xyzCoords[1],xyzCoords[2]);
          BRepClass_FaceClassifier scl(f,P,1e-7);
          scl.Perform(f, P, 1e-7);
          TopAbs_State result = scl.State();
          if ( result == TopAbs_OUT )
            MESSAGE("Point is out of face: node is not created");
          if ( result == TopAbs_UNKNOWN )
            MESSAGE("Point position on face is unknown: node is not created");
          if ( result == TopAbs_ON )
            MESSAGE("Point is on border of face: node is not created");
          if ( result == TopAbs_IN )
          {
            // Point is inside face and not on border
            double uvCoords[2] = {attractor_iter->second[0],attractor_iter->second[1]};
            ienf++;
            cad_point_t* point_p = cad_point_new(fce, ienf, uvCoords);
            cad_point_set_tag(point_p, ienf);
          }
          FaceId2AttractorCoords.erase(faceKey);
        }
      }

      // -----------------
      // Class Attractors
      // -----------------
      TId2ClsAttractorVec::iterator clAttractor_iter = FaceId2ClassAttractor.find(faceKey);
      if (clAttractor_iter != FaceId2ClassAttractor.end()){
        std::vector< BLSURFPlugin_Attractor* > & attVec = clAttractor_iter->second;
        for ( size_t i = 0; i < attVec.size(); ++i )
          if ( !attVec[i]->IsMapBuilt() ) {
            std::cout<<"Compute " << theNbAttractors-- << "-th attractor" <<std::endl;
            attVec[i]->BuildMap();
          }
        FaceIndex2ClassAttractor[iface].swap( attVec );
        FaceId2ClassAttractor.erase(clAttractor_iter);
      }
    } // if (HasSizeMapOnFace && !use_precad)

    // ------------------
    // Enforced Vertices
    // ------------------
    faceKey = FacesWithEnforcedVertices.FindIndex(f);
    std::map<int,BLSURFPlugin_Hypothesis::TEnfVertexCoordsList >::const_iterator evmIt = FaceId2EnforcedVertexCoords.find(faceKey);
    if (evmIt != FaceId2EnforcedVertexCoords.end())
    {
      BLSURFPlugin_Hypothesis::TEnfVertexCoordsList evl = evmIt->second;
      BLSURFPlugin_Hypothesis::TEnfVertexCoordsList::const_iterator evlIt = evl.begin();
      for (; evlIt != evl.end(); ++evlIt)
      {
        double uvCoords[2] = { evlIt->at(0), evlIt->at(1) };
        ienf++;
        cad_point_t* point_p = cad_point_new(fce, ienf, uvCoords);
        int tag = 0;
        BLSURFPlugin_Hypothesis::TEnfVertexCoords xyzCoords;
        xyzCoords.push_back(evlIt->at(2));
        xyzCoords.push_back(evlIt->at(3));
        xyzCoords.push_back(evlIt->at(4));
        std::map< BLSURFPlugin_Hypothesis::TEnfVertexCoords, BLSURFPlugin_Hypothesis::TEnfVertexList >::const_iterator enfCoordsIt = EnfVertexCoords2EnfVertexList.find(xyzCoords);
        if (enfCoordsIt != EnfVertexCoords2EnfVertexList.end() &&
            !enfCoordsIt->second.empty() )
        {
          // to merge nodes of an INTERNAL vertex belonging to several faces
          TopoDS_Vertex     v = (*enfCoordsIt->second.begin() )->vertex;
          if ( v.IsNull() ) v = (*enfCoordsIt->second.rbegin())->vertex;
          if ( !v.IsNull() && meshDS->ShapeToIndex( v ) > 0 )
          {
            tag = pmap.Add( v );
            SMESH_subMesh* vSM = aMesh.GetSubMesh( v );
            vSM->ComputeStateEngine( SMESH_subMesh::COMPUTE );
            mergeSubmeshes.insert( vSM->GetSubMeshDS() );
            // //if ( tag != pmap.Extent() )
            // needMerge = true;
          }
        }
        if ( tag == 0 ) tag = ienf;
        cad_point_set_tag(point_p, tag);
      }
      FaceId2EnforcedVertexCoords.erase(faceKey);

    }

    /****************************************************************************************
                                           EDGES
                        now create the edges associated to this face
    *****************************************************************************************/
    int edgeKey = -1;
    for (TopExp_Explorer edge_iter(f,TopAbs_EDGE);edge_iter.More();edge_iter.Next())
    {
      TopoDS_Edge e = TopoDS::Edge(edge_iter.Current());
      int ic = emap.FindIndex(e);
      if (ic <= 0)
        ic = emap.Add(e);

      double tmin,tmax;
      curves.push_back(BRep_Tool::CurveOnSurface(e, f, tmin, tmax));

      if (HasSizeMapOnEdge){
        edgeKey = EdgesWithSizeMap.FindIndex(e);
        if (EdgeId2SizeMap.find(edgeKey)!=EdgeId2SizeMap.end())
        {
          theSizeMapStr = EdgeId2SizeMap[edgeKey];
          if (theSizeMapStr.find(bad_end) == (theSizeMapStr.size()-bad_end.size()-1))
            continue;
          // Expr To Python function, verification is performed at validation in GUI
		  throw SALOME_Exception("Python expression not supported for size map.");
		  /*
          gstate = PyGILState_Ensure();
          PyObject * obj = NULL;
          obj= PyRun_String(theSizeMapStr.c_str(), Py_file_input, main_dict, NULL);
          Py_DECREF(obj);
          PyObject * func = NULL;
          func = PyObject_GetAttrString(main_mod, "f");
          EdgeId2PythonSmp[ic]=func;
          EdgeId2SizeMap.erase(edgeKey);
          PyGILState_Release(gstate);
		  */
        }
      }
      /* data of nodes existing on the edge */
      StdMeshers_FaceSidePtr nodeData;
      SMESH_subMesh* sm = aMesh.GetSubMesh( e );
      if ( !sm->IsEmpty() )
      {
        // SMESH_subMeshIteratorPtr subsmIt = sm->getDependsOnIterator( /*includeSelf=*/true,
        //                                                              /*complexFirst=*/false);
        // while ( subsmIt->more() )
        //   edgeSubmeshes.insert( subsmIt->next()->GetSubMeshDS() );
        edgeSubmeshes.insert( sm->GetSubMeshDS() );

        nodeData.reset( new StdMeshers_FaceSide( f, e, &aMesh, /*isForwrd = */true,
                                                 /*ignoreMedium=*/haveQuadraticSubMesh));
        if ( nodeData->MissVertexNode() )
          return error(COMPERR_BAD_INPUT_MESH,"No node on vertex");

        const std::vector<UVPtStruct>& nodeDataVec = nodeData->GetUVPtStruct();
        if ( !nodeDataVec.empty() )
        {
          if ( Abs( nodeDataVec[0].param - tmin ) > Abs( nodeDataVec.back().param - tmin ))
          {
            nodeData->Reverse();
            nodeData->GetUVPtStruct(); // nodeData recomputes nodeDataVec
          }
          // tmin and tmax can change in case of viscous layer on an adjacent edge
          tmin = nodeDataVec.front().param;
          tmax = nodeDataVec.back().param;
        }
        else
        {
          cout << "---------------- Invalid nodeData" << endl;
          nodeData.reset();
        }
      }

      /* attach the edge to the current cadsurf face */
#if OCC_VERSION_MAJOR < 7
      cad_edge_t *edg = cad_edge_new(fce, ic, tmin, tmax, curv_fun, curves.back());
#else
      cad_edge_t *edg = cad_edge_new(fce, ic, tmin, tmax, curv_fun, curves.back().get());
#endif

      /* by default an edge has no tag (color).
         The following call sets it to the same value as the edge_id : */
      // IMP23368. Do not set tag to an EDGE shared by FACEs of a hyper-patch
      bool isInHyperPatch = false;
      {
        std::set< int > faceTags, faceIDs;
        TopTools_ListIteratorOfListOfShape fIt( e2ffmap.FindFromKey( e ));
        for ( ; fIt.More(); fIt.Next() )
        {
          int faceTag = meshDS->ShapeToIndex( fIt.Value() );
          if ( !faceIDs.insert( faceTag ).second )
            continue; // a face encounters twice for a seam edge
          int hpTag   = BLSURFPlugin_Hypothesis::GetHyperPatchTag( faceTag, _hypothesis );
          if ( !faceTags.insert( hpTag ).second )
          {
            isInHyperPatch = true;
            break;
          }
        }
      }
      if ( !isInHyperPatch )
        cad_edge_set_tag(edg, ic);

      /* by default, an edge does not necessalry appear in the resulting mesh,
         unless the following property is set :
      */
      cad_edge_set_property(edg, EDGE_PROPERTY_SOFT_REQUIRED);

      /* by default an edge is a boundary edge */
      if (e.Orientation() == TopAbs_INTERNAL)
        cad_edge_set_property(edg, EDGE_PROPERTY_INTERNAL);

      // pass existing nodes of sub-meshes to MG-CADSurf
      if ( nodeData )
      {
        const std::vector<UVPtStruct>& nodeDataVec = nodeData->GetUVPtStruct();
        const int                      nbNodes     = nodeDataVec.size();

        dcad_edge_discretization_t *dedge;
        dcad_get_edge_discretization(dcad, edg, &dedge);
        dcad_edge_discretization_set_vertex_count( dedge, nbNodes );

        // cout << endl << " EDGE " << ic << endl;
        // cout << "tmin = "<<tmin << ", tmax = "<< tmax << endl;
        for ( int iN = 0; iN < nbNodes; ++iN )
        {
          const UVPtStruct& nData = nodeDataVec[ iN ];
          double t                = nData.param;
          real uv[2]              = { nData.u, nData.v };
          SMESH_TNodeXYZ nXYZ( nData.node );
          // cout << "\tt = " << t
          //      << "\t uv = ( " << uv[0] << ","<< uv[1] << " ) "
          //      << "\t u = " << nData.param
          //      << "\t ID = " << nData.node->GetID() << endl;
          dcad_edge_discretization_set_vertex_coordinates( dedge, iN+1, t, uv, nXYZ._xyz );
        }
        dcad_edge_discretization_set_property(dedge, DISTENE_DCAD_PROPERTY_REQUIRED);
      }

      /****************************************************************************************
                                      VERTICES
      *****************************************************************************************/

      int npts = 0;
      int ip1, ip2, *ip;
      gp_Pnt2d e0 = curves.back()->Value(tmin);
      gp_Pnt ee0 = surfaces.back()->Value(e0.X(), e0.Y());
      Standard_Real d1=0,d2=0;

      int vertexKey = -1;
      for (TopExp_Explorer ex_edge(e ,TopAbs_VERTEX); ex_edge.More(); ex_edge.Next()) {
        TopoDS_Vertex v = TopoDS::Vertex(ex_edge.Current());
        ++npts;
        if (npts == 1){
          ip = &ip1;
          d1 = ee0.SquareDistance(BRep_Tool::Pnt(v));
        } else {
          ip = &ip2;
          d2 = ee0.SquareDistance(BRep_Tool::Pnt(v));
        }
        *ip = pmap.FindIndex(v);
        if(*ip <= 0) {
          *ip = pmap.Add(v);
          // SMESH_subMesh* sm = aMesh.GetSubMesh(v);
          // if ( sm->IsMeshComputed() )
          //   edgeSubmeshes.insert( sm->GetSubMeshDS() );
        }

//        std::string aFileName = "fmap_vertex_";
//        aFileName.append(val_to_string(*ip));
//        aFileName.append(".brep");
//        BRepTools::Write(v,aFileName.c_str());

        if (HasSizeMapOnVertex){
          vertexKey = VerticesWithSizeMap.FindIndex(v);
          if (VertexId2SizeMap.find(vertexKey)!=VertexId2SizeMap.end()){
            theSizeMapStr = VertexId2SizeMap[vertexKey];
            if (theSizeMapStr.find(bad_end) == (theSizeMapStr.size()-bad_end.size()-1))
              continue;
            // Expr To Python function, verification is performed at validation in GUI
			throw SALOME_Exception("Python expression not supported for size map.");
			/*
            gstate = PyGILState_Ensure();
            PyObject * obj = NULL;
            obj= PyRun_String(theSizeMapStr.c_str(), Py_file_input, main_dict, NULL);
            Py_DECREF(obj);
            PyObject * func = NULL;
            func = PyObject_GetAttrString(main_mod, "f");
            VertexId2PythonSmp[*ip]=func;
            VertexId2SizeMap.erase(vertexKey);   // do not erase if using a vector
            PyGILState_Release(gstate);
			*/
          }
        }
      }
      if (npts != 2) {
        // should not happen
        MESSAGE("An edge does not have 2 extremities.");
      } else {
        if (d1 < d2) {
          // This defines the curves extremity connectivity
          cad_edge_set_extremities(edg, ip1, ip2);
          /* set the tag (color) to the same value as the extremity id : */
          cad_edge_set_extremities_tag(edg, ip1, ip2);
        }
        else {
          cad_edge_set_extremities(edg, ip2, ip1);
          cad_edge_set_extremities_tag(edg, ip2, ip1);
        }
      }
    } // for edge
  } //for face

  // Clear mesh from already meshed edges if possible else
  // remember that merge is needed
  TSubMeshSet::iterator smIt = edgeSubmeshes.begin();
  for ( ; smIt != edgeSubmeshes.end(); ++smIt ) // loop on already meshed EDGEs
  {
    SMESHDS_SubMesh* smDS = *smIt;
    if ( !smDS ) continue;
    SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
    if ( nIt->more() )
    {
      const SMDS_MeshNode* n = nIt->next();
      if ( n->NbInverseElements( SMDSAbs_Face ) > 0 )
      {
        needMerge = true; // to correctly sew with viscous mesh
        // add existing medium nodes to helper
        if ( aMesh.NbEdges( ORDER_QUADRATIC ) > 0 )
        {
          SMDS_ElemIteratorPtr edgeIt = smDS->GetElements();
          while ( edgeIt->more() )
            helper.AddTLinks( static_cast<const SMDS_MeshEdge*>(edgeIt->next()));
        }
        continue;
      }
    }
    if ( allowSubMeshClearing )
    {
      SMDS_ElemIteratorPtr eIt = smDS->GetElements();
      while ( eIt->more() ) meshDS->RemoveFreeElement( eIt->next(), 0 );
      SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
      while ( nIt->more() ) meshDS->RemoveFreeNode( nIt->next(), 0 );
      smDS->Clear();
    }
    else
    {
      needMerge = true;
    }
  }

  ///////////////////////
  // PERIODICITY       //
  ///////////////////////

  if (! _preCadFacesIDsPeriodicityVector.empty())
  {
    for (std::size_t i=0; i < _preCadFacesIDsPeriodicityVector.size(); i++){
      std::vector<int> theFace1_ids = _preCadFacesIDsPeriodicityVector[i].shape1IDs;
      std::vector<int> theFace2_ids = _preCadFacesIDsPeriodicityVector[i].shape2IDs;
      int* theFace1_ids_c = &theFace1_ids[0];
      int* theFace2_ids_c = &theFace2_ids[0];
      std::ostringstream o;
      o << "_preCadFacesIDsPeriodicityVector[" << i << "] = [";
      for (std::size_t j=0; j < theFace1_ids.size(); j++)
        o << theFace1_ids[j] << ", ";
      o << "], [";
      for (std::size_t j=0; j < theFace2_ids.size(); j++)
        o << theFace2_ids[j] << ", ";
      o << "]";
      // if ( _hypothesis->GetVerbosity() > _hypothesis->GetDefaultVerbosity() )
      //   cout << o.str() << endl;
      if (_preCadFacesIDsPeriodicityVector[i].theSourceVerticesCoords.empty())
      {
        // If no source points, call periodicity without transformation function
        meshgems_cad_periodicity_transformation_t periodicity_transformation = NULL;
        status = cad_add_face_multiple_periodicity_with_transformation_function(c, theFace1_ids_c, theFace1_ids.size(),
                                                                                theFace2_ids_c, theFace2_ids.size(), periodicity_transformation, NULL);
        if(status != STATUS_OK)
          cout << "cad_add_face_multiple_periodicity_with_transformation_function failed with error code " << status << "\n";
      }
      else
      {
        // get the transformation vertices
        double* theSourceVerticesCoords_c = &_preCadFacesIDsPeriodicityVector[i].theSourceVerticesCoords[0];
        double* theTargetVerticesCoords_c = &_preCadFacesIDsPeriodicityVector[i].theTargetVerticesCoords[0];
        int nbSourceVertices = _preCadFacesIDsPeriodicityVector[i].theSourceVerticesCoords.size()/3;
        int nbTargetVertices = _preCadFacesIDsPeriodicityVector[i].theTargetVerticesCoords.size()/3;

        status = cad_add_face_multiple_periodicity_with_transformation_function_by_points(c, theFace1_ids_c, theFace1_ids.size(),
                                                                                          theFace2_ids_c, theFace2_ids.size(), theSourceVerticesCoords_c, nbSourceVertices, theTargetVerticesCoords_c, nbTargetVertices);
        if(status != STATUS_OK)
          cout << "cad_add_face_multiple_periodicity_with_transformation_function_by_points failed with error code " << status << "\n";
      }
    }
  }

  if (! _preCadEdgesIDsPeriodicityVector.empty())
  {
    for (std::size_t i=0; i < _preCadEdgesIDsPeriodicityVector.size(); i++){
      std::vector<int> theEdge1_ids = _preCadEdgesIDsPeriodicityVector[i].shape1IDs;
      std::vector<int> theEdge2_ids = _preCadEdgesIDsPeriodicityVector[i].shape2IDs;
      // Use the address of the first element of the vector to initialize the array
      int* theEdge1_ids_c = &theEdge1_ids[0];
      int* theEdge2_ids_c = &theEdge2_ids[0];

      std::ostringstream o;
      o << "_preCadEdgesIDsPeriodicityVector[" << i << "] = [";
      for (std::size_t j=0; j < theEdge1_ids.size(); j++)
        o << theEdge1_ids[j] << ", ";
      o << "], [";
      for (std::size_t j=0; j < theEdge2_ids.size(); j++)
        o << theEdge2_ids[j] << ", ";
      o << "]";
      // if ( _hypothesis->GetVerbosity() > _hypothesis->GetDefaultVerbosity() )
      //   cout << o.str() << endl;

      if (_preCadEdgesIDsPeriodicityVector[i].theSourceVerticesCoords.empty())
      {
        // If no source points, call periodicity without transformation function
        meshgems_cad_periodicity_transformation_t periodicity_transformation = NULL;
        status = cad_add_edge_multiple_periodicity_with_transformation_function(c, theEdge1_ids_c, theEdge1_ids.size(),
                                                                                theEdge2_ids_c, theEdge2_ids.size(), periodicity_transformation, NULL);
        if(status != STATUS_OK)
          cout << "cad_add_edge_multiple_periodicity_with_transformation_function failed with error code " << status << "\n";
      }
      else
      {
        // get the transformation vertices
        double* theSourceVerticesCoords_c = &_preCadEdgesIDsPeriodicityVector[i].theSourceVerticesCoords[0];
        double* theTargetVerticesCoords_c = &_preCadEdgesIDsPeriodicityVector[i].theTargetVerticesCoords[0];
        int nbSourceVertices = _preCadEdgesIDsPeriodicityVector[i].theSourceVerticesCoords.size()/3;
        int nbTargetVertices = _preCadEdgesIDsPeriodicityVector[i].theTargetVerticesCoords.size()/3;

        status = cad_add_edge_multiple_periodicity_with_transformation_function_by_points(c, theEdge1_ids_c, theEdge1_ids.size(),
                                                                                          theEdge2_ids_c, theEdge2_ids.size(), theSourceVerticesCoords_c, nbSourceVertices, theTargetVerticesCoords_c, nbTargetVertices);
        if(status != STATUS_OK)
          cout << "cad_add_edge_multiple_periodicity_with_transformation_function_by_points failed with error code " << status << "\n";
      }
    }
  }

  
  // TODO: be able to use a mesh in input.
  // See imsh usage in Products/templates/mg-cadsurf_template_common.cpp
  // => cadsurf_set_mesh
    
  // Use the original dcad
  cadsurf_set_dcad(css, dcad);

  // Use the original cad
  cadsurf_set_cad(css, c);

  std::cout << std::endl;
  std::cout << "Beginning of Surface Mesh generation" << std::endl;
  std::cout << std::endl;

  try {
    OCC_CATCH_SIGNALS;

    status = cadsurf_compute_mesh(css);

  }
  catch ( std::exception& exc ) {
    _comment += exc.what();
  }
  catch (Standard_Failure& ex) {
    _comment += ex.DynamicType()->Name();
    if ( ex.GetMessageString() && strlen( ex.GetMessageString() )) {
      _comment += ": ";
      _comment += ex.GetMessageString();
    }
  }
  catch (...) {
    if ( _comment.empty() )
      _comment = "Exception in cadsurf_compute_mesh()";
  }

  std::cout << std::endl;
  std::cout << "End of Surface Mesh generation" << std::endl;
  std::cout << std::endl;

  mesh_t *msh = NULL;
  cadsurf_get_mesh(css, &msh);
  if(!msh){
    /* release the mesh object */
    cadsurf_regain_mesh(css, msh);
    return error(_comment);
  }

  std::string GMFFileName = BLSURFPlugin_Hypothesis::GetDefaultGMFFile();
  if (_hypothesis)
    GMFFileName = _hypothesis->GetGMFFile();
  if (GMFFileName != "") {
    bool asciiFound  = (GMFFileName.find(".mesh", GMFFileName.length()-5) != std::string::npos);
    bool binaryFound = (GMFFileName.find(".meshb",GMFFileName.length()-6) != std::string::npos);
    if (!asciiFound && !binaryFound)
      GMFFileName.append(".mesh");
    mesh_write_mesh(msh, GMFFileName.c_str());
  }

  /* retrieve mesh data (see meshgems/mesh.h) */
  integer nv, ne, nt, nq, vtx[4], tag, nb_tag;
  integer *evedg, *evtri, *evquad, *tags_buff, type;
  real xyz[3];

  mesh_get_vertex_count(msh, &nv);
  mesh_get_edge_count(msh, &ne);
  mesh_get_triangle_count(msh, &nt);
  mesh_get_quadrangle_count(msh, &nq);

  evedg  = (integer *)mesh_calloc_generic_buffer(msh);
  evtri  = (integer *)mesh_calloc_generic_buffer(msh);
  evquad = (integer *)mesh_calloc_generic_buffer(msh);
  tags_buff = (integer*)mesh_calloc_generic_buffer(msh);

  std::vector<const SMDS_MeshNode*> nodes(nv+1);
  std::vector<bool>                  tags(nv+1);

  /* enumerated vertices */
  for(int iv=1;iv<=nv;iv++) {
    mesh_get_vertex_coordinates(msh, iv, xyz);
    mesh_get_vertex_tag(msh, iv, &tag);
    // Issue 0020656. Use vertex coordinates
    nodes[iv] = NULL;
    if ( tag > 0 && tag <= pmap.Extent() ) {
      TopoDS_Vertex v = TopoDS::Vertex(pmap(tag));
      double tol = BRep_Tool::Tolerance( v );
      gp_Pnt p = BRep_Tool::Pnt( v );
      if ( p.IsEqual( gp_Pnt( xyz[0], xyz[1], xyz[2]), 2*tol))
        xyz[0] = p.X(), xyz[1] = p.Y(), xyz[2] = p.Z();
      else
        tag = 0; // enforced or attracted vertex
      nodes[iv] = SMESH_Algo::VertexNode( v, meshDS );
    }
    if ( !nodes[iv] )
      nodes[iv] = meshDS->AddNode(xyz[0], xyz[1], xyz[2]);

    // Create group of enforced vertices if requested
    BLSURFPlugin_Hypothesis::TEnfVertexCoords projVertex;
    projVertex.clear();
    projVertex.push_back((double)xyz[0]);
    projVertex.push_back((double)xyz[1]);
    projVertex.push_back((double)xyz[2]);
    std::map< BLSURFPlugin_Hypothesis::TEnfVertexCoords, BLSURFPlugin_Hypothesis::TEnfVertexList >::const_iterator enfCoordsIt = EnfVertexCoords2EnfVertexList.find(projVertex);
    if (enfCoordsIt != EnfVertexCoords2EnfVertexList.end())
    {
      BLSURFPlugin_Hypothesis::TEnfVertexList::const_iterator enfListIt = enfCoordsIt->second.begin();
      BLSURFPlugin_Hypothesis::TEnfVertex *currentEnfVertex;
      for (; enfListIt != enfCoordsIt->second.end(); ++enfListIt) {
        currentEnfVertex = (*enfListIt);
        if (currentEnfVertex->grpName != "") {
          bool groupDone = false;
          SMESH_Mesh::GroupIteratorPtr grIt = aMesh.GetGroups();
          while (grIt->more()) {
            SMESH_Group * group = grIt->next();
            if ( !group ) continue;
            SMESHDS_GroupBase* groupDS = group->GetGroupDS();
            if ( !groupDS ) continue;
            if ( groupDS->GetType()==SMDSAbs_Node && currentEnfVertex->grpName.compare(group->GetName())==0) {
              SMESHDS_Group* aGroupDS = static_cast<SMESHDS_Group*>( groupDS );
              aGroupDS->SMDSGroup().Add(nodes[iv]);
              // How can I inform the hypothesis ?
              //                 _hypothesis->AddEnfVertexNodeID(currentEnfVertex->grpName,nodes[iv]->GetID());
              groupDone = true;
              break;
            }
          }
          if (!groupDone)
          {
            int groupId;
            SMESH_Group* aGroup = aMesh.AddGroup(SMDSAbs_Node, currentEnfVertex->grpName.c_str(), groupId);
            aGroup->SetName( currentEnfVertex->grpName.c_str() );
            SMESHDS_Group* aGroupDS = static_cast<SMESHDS_Group*>( aGroup->GetGroupDS() );
            aGroupDS->SMDSGroup().Add(nodes[iv]);
            groupDone = true;
          }
          if (!groupDone)
            throw SALOME_Exception(LOCALIZED("An enforced vertex node was not added to a group"));
        }
        else
          MESSAGE("Group name is empty: '"<<currentEnfVertex->grpName<<"' => group is not created");
      }
    }

    // internal points are tagged to zero
    if(tag > 0 && tag <= pmap.Extent() ){
      meshDS->SetNodeOnVertex(nodes[iv], TopoDS::Vertex(pmap(tag)));
      tags[iv] = false;
    } else {
      tags[iv] = true;
    }
  }

  /* enumerate edges */
  for(int it=1;it<=ne;it++) {
    SMDS_MeshEdge* edg;
    mesh_get_edge_vertices(msh, it, vtx);
    mesh_get_edge_extra_vertices(msh, it, &type, evedg);
    mesh_get_edge_tag(msh, it, &tag);

    // If PreCAD performed some cleaning operations (remove tiny edges,
    // merge edges ...) an output tag can indeed represent several original tags.
    // Get the initial tags corresponding to the output tag and redefine the tag as 
    // the last of the two initial tags (else the output tag is out of emap and hasn't any meaning)
    mesh_get_composite_tag_definition(msh, tag, &nb_tag, tags_buff);
    if(nb_tag > 1)  
      tag=tags_buff[nb_tag-1];
    if ( tag < 1 || tag > emap.Extent() )
    {
      std::cerr << "MG-CADSurf BUG:::: Edge tag " << tag
                << " does not point to a CAD edge (nb edges " << emap.Extent() << ")" << std::endl;
      continue;
    }
    if (tags[vtx[0]]) {
      Set_NodeOnEdge(meshDS, nodes[vtx[0]], emap(tag));
      tags[vtx[0]] = false;
    };
    if (tags[vtx[1]]) {
      Set_NodeOnEdge(meshDS, nodes[vtx[1]], emap(tag));
      tags[vtx[1]] = false;
    };
    if (type == MESHGEMS_MESH_ELEMENT_TYPE_EDGE3) {
      // QUADRATIC EDGE
      if (tags[evedg[0]]) {
        Set_NodeOnEdge(meshDS, nodes[evedg[0]], emap(tag));
        tags[evedg[0]] = false;
      }
      edg = meshDS->AddEdge(nodes[vtx[0]], nodes[vtx[1]], nodes[evedg[0]]);
    }
    else {
      edg = helper.AddEdge(nodes[vtx[0]], nodes[vtx[1]]);
    }
    meshDS->SetMeshElementOnShape(edg, TopoDS::Edge(emap(tag)));
  }

  /* enumerate triangles */
  for(int it=1;it<=nt;it++) {
    SMDS_MeshFace* tri;
    mesh_get_triangle_vertices(msh, it, vtx);
    mesh_get_triangle_extra_vertices(msh, it, &type, evtri);
    mesh_get_triangle_tag(msh, it, &tag);
    if (tags[vtx[0]]) {
      meshDS->SetNodeOnFace(nodes[vtx[0]], tag);
      tags[vtx[0]] = false;
    };
    if (tags[vtx[1]]) {
      meshDS->SetNodeOnFace(nodes[vtx[1]], tag);
      tags[vtx[1]] = false;
    };
    if (tags[vtx[2]]) {
      meshDS->SetNodeOnFace(nodes[vtx[2]], tag);
      tags[vtx[2]] = false;
    };
    if (type == MESHGEMS_MESH_ELEMENT_TYPE_TRIA6) {
      // QUADRATIC TRIANGLE
      if (tags[evtri[0]]) {
        meshDS->SetNodeOnFace(nodes[evtri[0]], tag);
        tags[evtri[0]] = false;
      }
      if (tags[evtri[1]]) {
        meshDS->SetNodeOnFace(nodes[evtri[1]], tag);
        tags[evtri[1]] = false;
      }
      if (tags[evtri[2]]) {
        meshDS->SetNodeOnFace(nodes[evtri[2]], tag);
        tags[evtri[2]] = false;
      }
      tri = meshDS->AddFace(nodes[vtx[0]], nodes[vtx[1]], nodes[vtx[2]],
                            nodes[evtri[0]], nodes[evtri[1]], nodes[evtri[2]]);
    }
    else {
      tri = helper.AddFace(nodes[vtx[0]], nodes[vtx[1]], nodes[vtx[2]]);
    }
    meshDS->SetMeshElementOnShape(tri, tag);
  }

  /* enumerate quadrangles */
  for(int it=1;it<=nq;it++) {
    SMDS_MeshFace* quad;
    mesh_get_quadrangle_vertices(msh, it, vtx);
    mesh_get_quadrangle_extra_vertices(msh, it, &type, evquad);
    mesh_get_quadrangle_tag(msh, it, &tag);
    if (tags[vtx[0]]) {
      meshDS->SetNodeOnFace(nodes[vtx[0]], tag);
      tags[vtx[0]] = false;
    };
    if (tags[vtx[1]]) {
      meshDS->SetNodeOnFace(nodes[vtx[1]], tag);
      tags[vtx[1]] = false;
    };
    if (tags[vtx[2]]) {
      meshDS->SetNodeOnFace(nodes[vtx[2]], tag);
      tags[vtx[2]] = false;
    };
    if (tags[vtx[3]]) {
      meshDS->SetNodeOnFace(nodes[vtx[3]], tag);
      tags[vtx[3]] = false;
    };
    if (type == MESHGEMS_MESH_ELEMENT_TYPE_QUAD9) {
      // QUADRATIC QUADRANGLE
      std::cout << "This is a quadratic quadrangle" << std::endl;
      if (tags[evquad[0]]) {
        meshDS->SetNodeOnFace(nodes[evquad[0]], tag);
        tags[evquad[0]] = false;
      }
      if (tags[evquad[1]]) {
        meshDS->SetNodeOnFace(nodes[evquad[1]], tag);
        tags[evquad[1]] = false;
      }
      if (tags[evquad[2]]) {
        meshDS->SetNodeOnFace(nodes[evquad[2]], tag);
        tags[evquad[2]] = false;
      }
      if (tags[evquad[3]]) {
        meshDS->SetNodeOnFace(nodes[evquad[3]], tag);
        tags[evquad[3]] = false;
      }
      if (tags[evquad[4]]) {
        meshDS->SetNodeOnFace(nodes[evquad[4]], tag);
        tags[evquad[4]] = false;
      }
      quad = meshDS->AddFace(nodes[vtx[0]], nodes[vtx[1]], nodes[vtx[2]], nodes[vtx[3]],
                             nodes[evquad[0]], nodes[evquad[1]], nodes[evquad[2]], nodes[evquad[3]],
                             nodes[evquad[4]]);
    }
    else {
      quad = helper.AddFace(nodes[vtx[0]], nodes[vtx[1]], nodes[vtx[2]], nodes[vtx[3]]);
    }
    meshDS->SetMeshElementOnShape(quad, tag);
  }

  /* release the mesh object, the rest is released by cleaner */
  cadsurf_regain_mesh(css, msh);


  // Remove free nodes that can appear e.g. if "remove tiny edges"(IPAL53235)
  for(int iv=1;iv<=nv;iv++)
    if ( nodes[iv] && nodes[iv]->NbInverseElements() == 0 )
      meshDS->RemoveFreeNode( nodes[iv], 0, /*fromGroups=*/false );


  if ( needMerge ) // sew mesh computed by MG-CADSurf with pre-existing mesh
  {
    SMESH_MeshEditor editor( &aMesh );
    SMESH_MeshEditor::TListOfListOfNodes nodeGroupsToMerge;
    TIDSortedElemSet segementsOnEdge;
    TSubMeshSet::iterator smIt;
    SMESHDS_SubMesh* smDS;

    // merge nodes on EDGE's with ones computed by MG-CADSurf
    for ( smIt = mergeSubmeshes.begin(); smIt != mergeSubmeshes.end(); ++smIt )
    {
      if (! (smDS = *smIt) ) continue;
      getNodeGroupsToMerge( smDS, meshDS->IndexToShape((*smIt)->GetID()), nodeGroupsToMerge );

      SMDS_ElemIteratorPtr segIt = smDS->GetElements();
      while ( segIt->more() )
        segementsOnEdge.insert( segIt->next() );
    }
    // merge nodes
    editor.MergeNodes( nodeGroupsToMerge );

    // merge segments
    SMESH_MeshEditor::TListOfListOfElementsID equalSegments;
    editor.FindEqualElements( segementsOnEdge, equalSegments );
    editor.MergeElements( equalSegments );

    // remove excess segments created on the boundary of viscous layers
    const SMDS_TypeOfPosition onFace = SMDS_TOP_FACE;
    for ( int i = 1; i <= emap.Extent(); ++i )
    {
      if ( SMESHDS_SubMesh* smDS = meshDS->MeshElements( emap( i )))
      {
        SMDS_ElemIteratorPtr segIt = smDS->GetElements();
        while ( segIt->more() )
        {
          const SMDS_MeshElement* seg = segIt->next();
          if ( seg->GetNode(0)->GetPosition()->GetTypeOfPosition() == onFace ||
               seg->GetNode(1)->GetPosition()->GetTypeOfPosition() == onFace )
            meshDS->RemoveFreeElement( seg, smDS );
        }
      }
    }
  }


  // SetIsAlwaysComputed( true ) to sub-meshes of EDGEs w/o mesh
  for (int i = 1; i <= emap.Extent(); i++)
    if ( SMESH_subMesh* sm = aMesh.GetSubMeshContaining( emap( i )))
      sm->SetIsAlwaysComputed( true );
  for (int i = 1; i <= pmap.Extent(); i++)
    if ( SMESH_subMesh* sm = aMesh.GetSubMeshContaining( pmap( i )))
      if ( !sm->IsMeshComputed() )
        sm->SetIsAlwaysComputed( true );

  // Set error to FACE's w/o elements
  SMESH_ComputeErrorName err = COMPERR_ALGO_FAILED;
  if ( _comment.empty() && status == STATUS_OK )
  {
    err      = COMPERR_WARNING;
    _comment = "No mesh elements assigned to a face";
  }
  bool badFaceFound = false;
  for (TopExp_Explorer face_iter(aShape,TopAbs_FACE);face_iter.More();face_iter.Next())
  {
    TopoDS_Face f = TopoDS::Face(face_iter.Current());
    SMESH_subMesh* sm = aMesh.GetSubMesh( f );
    if ( !sm->GetSubMeshDS() || sm->GetSubMeshDS()->NbElements() == 0 )
    {
      int faceTag = sm->GetId();
      if ( faceTag != BLSURFPlugin_Hypothesis::GetHyperPatchTag( faceTag, _hypothesis ))
      {
        // triangles are assigned to the first face of hyper-patch
        sm->SetIsAlwaysComputed( true );
      }
      else
      {
        sm->GetComputeError().reset( new SMESH_ComputeError( err, _comment, this ));
        badFaceFound = true;
      }
    }
  }
  if ( err == COMPERR_WARNING )
  {
    _comment.clear();
  }
  if ( status != STATUS_OK && !badFaceFound ) {
    error(_comment);
  }

  // Issue 0019864. On DebianSarge, FE signals do not obey to OSD::SetSignal(false)
#ifndef WIN32
  if ( oldFEFlags > 0 )
    feenableexcept( oldFEFlags );
  feclearexcept( FE_ALL_EXCEPT );
#endif

  /*
    std::cout << "FacesWithSizeMap" << std::endl;
    FacesWithSizeMap.Statistics(std::cout);
    std::cout << "EdgesWithSizeMap" << std::endl;
    EdgesWithSizeMap.Statistics(std::cout);
    std::cout << "VerticesWithSizeMap" << std::endl;
    VerticesWithSizeMap.Statistics(std::cout);
    std::cout << "FacesWithEnforcedVertices" << std::endl;
    FacesWithEnforcedVertices.Statistics(std::cout);
  */

  return ( status == STATUS_OK && !quadraticSubMeshAndViscousLayer );
}

//================================================================================
/*!
 * \brief Compute a mesh basing on discrete CAD description
 */
//================================================================================

bool BLSURFPlugin_BLSURF::Compute(SMESH_Mesh & aMesh, SMESH_MesherHelper* aHelper)
{
  if ( aMesh.NbFaces() == 0 )
    return error( COMPERR_BAD_INPUT_MESH, "2D elements are missing" );

  context_t *ctx = context_new();
  if (!ctx) return error("Pb in context_new()");

  BLSURF_Cleaner cleaner( ctx );

  message_cb_user_data mcud;
  mcud._error     = & this->SMESH_Algo::_comment;
  mcud._progress  = & this->SMESH_Algo::_progress;
  mcud._verbosity =
    _hypothesis ? _hypothesis->GetVerbosity() : BLSURFPlugin_Hypothesis::GetDefaultVerbosity();
  meshgems_status_t ret = context_set_message_callback(ctx, message_cb, &mcud);
  if (ret != STATUS_OK) return error("Pb. in context_set_message_callback() ");

  cadsurf_session_t * css = cadsurf_session_new(ctx);
  if(!css) return error( "Pb. in cadsurf_session_new() " );
  cleaner._css = css;


  // Fill an input mesh

  mesh_t * msh = meshgems_mesh_new_in_memory( ctx );
  if ( !msh ) return error("Pb. in meshgems_mesh_new_in_memory()"); 

  // mark nodes used by 2D elements
  SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();
  SMDS_NodeIteratorPtr nodeIt = meshDS->nodesIterator();
  while ( nodeIt->more() )
  {
    const SMDS_MeshNode* n = nodeIt->next();
    n->setIsMarked( n->NbInverseElements( SMDSAbs_Face ));
  }
  meshgems_mesh_set_vertex_count( msh, meshDS->NbNodes() );

  // set node coordinates
  if ( meshDS->NbNodes() != meshDS->MaxNodeID() )
  {
    meshDS->compactMesh();
  }
  SMESH_TNodeXYZ nXYZ;
  nodeIt = meshDS->nodesIterator();
  meshgems_integer i;
  for ( i = 1; nodeIt->more(); ++i )
  {
    nXYZ.Set( nodeIt->next() );
    meshgems_mesh_set_vertex_coordinates( msh, i, nXYZ._xyz );
  }

  // set nodes of faces
  meshgems_mesh_set_triangle_count  ( msh, meshDS->GetMeshInfo().NbTriangles() );
  meshgems_mesh_set_quadrangle_count( msh, meshDS->GetMeshInfo().NbQuadrangles() );
  meshgems_integer nodeIDs[4];
  meshgems_integer iT = 1, iQ = 1;
  SMDS_FaceIteratorPtr faceIt = meshDS->facesIterator();
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    meshgems_integer nbNodes = face->NbCornerNodes();
    if ( nbNodes > 4 || face->IsPoly() ) continue;

    for ( i = 0; i < nbNodes; ++i )
      nodeIDs[i] = face->GetNode( i )->GetID();
    if ( nbNodes == 3 )
      meshgems_mesh_set_triangle_vertices  ( msh, iT++, nodeIDs );
    else
      meshgems_mesh_set_quadrangle_vertices( msh, iQ++, nodeIDs );
  }

  ret = cadsurf_set_mesh(css, msh);
  if ( ret != STATUS_OK ) return error("Pb in cadsurf_set_mesh()");


  // Compute the mesh

  SetParameters(_hypothesis, css, aMesh.GetShapeToMesh() );

  ret = cadsurf_compute_mesh(css);
  if ( ret != STATUS_OK ) return false;

  mesh_t *omsh = 0;
  cadsurf_get_mesh(css, &omsh);
  if ( !omsh ) return error( "Pb. in cadsurf_get_mesh()" );


  // Update SALOME mesh

  // remove quadrangles and triangles
  for ( faceIt = meshDS->facesIterator(); faceIt->more();  )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( !face->IsPoly() )
      meshDS->RemoveFreeElement( face, /*sm=*/0, /*fromGroups=*/true );
  }
  // remove edges that bound the just removed faces
  for ( SMDS_EdgeIteratorPtr edgeIt = meshDS->edgesIterator(); edgeIt->more(); )
  {
    const SMDS_MeshElement* edge = edgeIt->next();
    const SMDS_MeshNode* n0 = edge->GetNode(0);
    const SMDS_MeshNode* n1 = edge->GetNode(1);
    if ( n0->isMarked() &&
         n1->isMarked() &&
         n0->NbInverseElements( SMDSAbs_Volume ) == 0 &&
         n1->NbInverseElements( SMDSAbs_Volume ) == 0 )
      meshDS->RemoveFreeElement( edge, /*sm=*/0, /*fromGroups=*/true );
  }
  // remove nodes that just became free
  for ( nodeIt = meshDS->nodesIterator(); nodeIt->more(); )
  {
    const SMDS_MeshNode* n = nodeIt->next();
    if ( n->isMarked() && n->NbInverseElements() == 0 )
      meshDS->RemoveFreeNode( n, /*sm=*/0, /*fromGroups=*/true );
  }

  // add nodes
  meshgems_integer nbvtx = 0, nodeID;
  meshgems_mesh_get_vertex_count( omsh, &nbvtx );
  meshgems_real xyz[3];
  for ( i = 1; i <= nbvtx; ++i )
  {
    meshgems_mesh_get_vertex_coordinates( omsh, i, xyz );
    SMDS_MeshNode* n = meshDS->AddNode( xyz[0], xyz[1], xyz[2] );
    nodeID = n->GetID();
    meshgems_mesh_set_vertex_tag( omsh, i, &nodeID ); // save mapping of IDs in MG and SALOME meshes
  }

  // add triangles
  meshgems_integer nbtri = 0;
  meshgems_mesh_get_triangle_count( omsh, &nbtri );
  const SMDS_MeshNode* nodes[3];
  for ( i = 1; i <= nbtri; ++i )
  {
    meshgems_mesh_get_triangle_vertices( omsh, i, nodeIDs );
    for ( int j = 0; j < 3; ++j )
    {
      meshgems_mesh_get_vertex_tag( omsh, nodeIDs[j], &nodeID );
      nodes[j] = meshDS->FindNode( nodeID );
    }
    meshDS->AddFace( nodes[0], nodes[1], nodes[2] );
  }

  cadsurf_regain_mesh(css, omsh);

  // as we don't assign the new triangles to a shape (the pseudo-shape),
  // we mark the shape as always computed to avoid the error messages
  // that no elements assigned to the shape
  aMesh.GetSubMesh( aHelper->GetSubShape() )->SetIsAlwaysComputed( true );

  return true;
}

//================================================================================
/*!
 * \brief Terminates computation
 */
//================================================================================

void BLSURFPlugin_BLSURF::CancelCompute()
{
  _compute_canceled = true;
}

//=============================================================================
/*!
 *  SetNodeOnEdge
 */
//=============================================================================

void BLSURFPlugin_BLSURF::Set_NodeOnEdge(SMESHDS_Mesh*        meshDS,
                                         const SMDS_MeshNode* node,
                                         const TopoDS_Shape&  ed)
{
  const TopoDS_Edge edge = TopoDS::Edge(ed);

  gp_Pnt pnt(node->X(), node->Y(), node->Z());

  Standard_Real p0 = 0.0;
  Standard_Real p1 = 1.0;
  TopLoc_Location loc;
  Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, loc, p0, p1);
  if ( curve.IsNull() )
  {
    // issue 22499. Node at a sphere apex
    meshDS->SetNodeOnEdge(node, edge, p0);
    return;
  }

  if ( !loc.IsIdentity() ) pnt.Transform( loc.Transformation().Inverted() );
  GeomAPI_ProjectPointOnCurve proj(pnt, curve, p0, p1);

  double pa = 0.;
  if ( proj.NbPoints() > 0 )
  {
    pa = (double)proj.LowerDistanceParameter();
    // Issue 0020656. Move node if it is too far from edge
    gp_Pnt curve_pnt = curve->Value( pa );
    double dist2     = pnt.SquareDistance( curve_pnt );
    double tol       = BRep_Tool::Tolerance( edge );
    if ( 1e-14 < dist2 && dist2 <= 1000*tol ) // large enough and within tolerance
    {
      curve_pnt.Transform( loc );
      meshDS->MoveNode( node, curve_pnt.X(), curve_pnt.Y(), curve_pnt.Z() );
    }
  }

  meshDS->SetNodeOnEdge(node, edge, pa);
}

/* Curve definition function See cad_curv_t in file meshgems/cad.h for
 * more information.
 * NOTE : if when your CAD systems evaluates second
 * order derivatives it also computes first order derivatives and
 * function evaluation, you can optimize this example by making only
 * one CAD call and filling the necessary uv, dt, dtt arrays.
 */
status_t curv_fun(real t, real *uv, real *dt, real *dtt, void *user_data)
{
  /* t is given. It contains the t (time) 1D parametric coordintaes
     of the point PreCAD/MG-CADSurf is querying on the curve */

  /* user_data identifies the edge PreCAD/MG-CADSurf is querying
   * (see cad_edge_new later in this example) */
  const Geom2d_Curve*pargeo = (const Geom2d_Curve*) user_data;

  if (uv){
   /* MG-CADSurf is querying the function evaluation */
    gp_Pnt2d P;
    P=pargeo->Value(t);
    uv[0]=P.X(); uv[1]=P.Y();
  }

  if(dt) {
   /* query for the first order derivatives */
    gp_Vec2d V1;
    V1=pargeo->DN(t,1);
    dt[0]=V1.X(); dt[1]=V1.Y();
  }

  if(dtt){
    /* query for the second order derivatives */
    gp_Vec2d V2;
    V2=pargeo->DN(t,2);
    dtt[0]=V2.X(); dtt[1]=V2.Y();
  }

  return STATUS_OK;
}

/* Surface definition function.
 * See cad_surf_t in file meshgems/cad.h for more information.
 * NOTE : if when your CAD systems evaluates second order derivatives it also
 * computes first order derivatives and function evaluation, you can optimize
 * this example by making only one CAD call and filling the necessary xyz, du, dv, etc..
 * arrays.
 */
status_t surf_fun(real *uv, real *xyz, real*du, real *dv,
                  real *duu, real *duv, real *dvv, void *user_data)
{
  /* uv[2] is given. It contains the u,v coordinates of the point
   * PreCAD/MG-CADSurf is querying on the surface */

  /* user_data identifies the face PreCAD/MG-CADSurf is querying (see
   * cad_face_new later in this example)*/
  const Geom_Surface* geometry = (const Geom_Surface*) user_data;

  if(xyz){
   gp_Pnt P;
   P=geometry->Value(uv[0],uv[1]);   // S.D0(U,V,P);
   xyz[0]=P.X(); xyz[1]=P.Y(); xyz[2]=P.Z();
  }

  if(du && dv){
    gp_Pnt P;
    gp_Vec D1U,D1V;

    geometry->D1(uv[0],uv[1],P,D1U,D1V);
    du[0]=D1U.X(); du[1]=D1U.Y(); du[2]=D1U.Z();
    dv[0]=D1V.X(); dv[1]=D1V.Y(); dv[2]=D1V.Z();
  }

  if(duu && duv && dvv){

    gp_Pnt P;
    gp_Vec D1U,D1V;
    gp_Vec D2U,D2V,D2UV;

    geometry->D2(uv[0],uv[1],P,D1U,D1V,D2U,D2V,D2UV);
    duu[0]=D2U.X(); duu[1]=D2U.Y(); duu[2]=D2U.Z();
    duv[0]=D2UV.X(); duv[1]=D2UV.Y(); duv[2]=D2UV.Z();
    dvv[0]=D2V.X(); dvv[1]=D2V.Y(); dvv[2]=D2V.Z();
  }

  return STATUS_OK;
}


status_t size_on_surface(integer face_id, real *uv, real *size, void *user_data)
{
  TId2ClsAttractorVec::iterator f2attVec;
  /*
  if (FaceId2PythonSmp.count(face_id) != 0){
    assert(Py_IsInitialized());
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyObject* pyresult = PyObject_CallFunction(FaceId2PythonSmp[face_id],(char*)"(f,f)",uv[0],uv[1]);
    real result;
    if ( pyresult != NULL) {
      result = PyFloat_AsDouble(pyresult);
      Py_DECREF(pyresult);
      //       *size = result;
    }
    else{
      fflush(stderr);
      string err_description="";
      PyObject* new_stderr = newPyStdOut(err_description);
      PyObject* old_stderr = PySys_GetObject((char*)"stderr");
      Py_INCREF(old_stderr);
      PySys_SetObject((char*)"stderr", new_stderr);
      PyErr_Print();
      PySys_SetObject((char*)"stderr", old_stderr);
      Py_DECREF(new_stderr);
      MESSAGE("Can't evaluate f(" << uv[0] << "," << uv[1] << ")" << " error is " << err_description);
      result = *((real*)user_data);
    }
    *size = result;
    PyGILState_Release(gstate);
  }
  */
  if (( f2attVec = FaceIndex2ClassAttractor.find(face_id)) != FaceIndex2ClassAttractor.end() && !f2attVec->second.empty())
  {
    real result = 0;
    result = 1e100;
    std::vector< BLSURFPlugin_Attractor* > & attVec = f2attVec->second;
    for ( size_t i = 0; i < attVec.size(); ++i )
    {
      //result += attVec[i]->GetSize(uv[0],uv[1]);
      result = Min( result, attVec[i]->GetSize(uv[0],uv[1]));
    }
    //*size = result / attVec.size(); // mean of sizes defined by all attractors
    *size = result;
  }
  else {
    *size = *((real*)user_data);
  }
  //   std::cout << "Size_on_surface sur la face " << face_id << " donne une size de: " << *size << std::endl;
  return STATUS_OK;
}

status_t size_on_edge(integer edge_id, real t, real *size, void *user_data)
{
  /*
	if (EdgeId2PythonSmp.count(edge_id) != 0){
    assert(Py_IsInitialized());
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyObject* pyresult = PyObject_CallFunction(EdgeId2PythonSmp[edge_id],(char*)"(f)",t);
    real result;
    if ( pyresult != NULL) {
      result = PyFloat_AsDouble(pyresult);
      Py_DECREF(pyresult);
//       *size = result;
    }
    else{
      fflush(stderr);
      string err_description="";
      PyObject* new_stderr = newPyStdOut(err_description);
      PyObject* old_stderr = PySys_GetObject((char*)"stderr");
      Py_INCREF(old_stderr);
      PySys_SetObject((char*)"stderr", new_stderr);
      PyErr_Print();
      PySys_SetObject((char*)"stderr", old_stderr);
      Py_DECREF(new_stderr);
      MESSAGE("Can't evaluate f(" << t << ")" << " error is " << err_description);
      result = *((real*)user_data);
    }
    *size = result;
    PyGILState_Release(gstate);
  }
  else {
    *size = *((real*)user_data);
  }
  */
  *size = *((real*)user_data);
  return STATUS_OK;
}

status_t size_on_vertex(integer point_id, real *size, void *user_data)
{
  /*
	if (VertexId2PythonSmp.count(point_id) != 0){
    assert(Py_IsInitialized());
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyObject* pyresult = PyObject_CallFunction(VertexId2PythonSmp[point_id],(char*)"");
    real result;
    if ( pyresult != NULL) {
      result = PyFloat_AsDouble(pyresult);
      Py_DECREF(pyresult);
//       *size = result;
    }
    else {
      fflush(stderr);
      string err_description="";
      PyObject* new_stderr = newPyStdOut(err_description);
      PyObject* old_stderr = PySys_GetObject((char*)"stderr");
      Py_INCREF(old_stderr);
      PySys_SetObject((char*)"stderr", new_stderr);
      PyErr_Print();
      PySys_SetObject((char*)"stderr", old_stderr);
      Py_DECREF(new_stderr);
      MESSAGE("Can't evaluate f()" << " error is " << err_description);
      result = *((real*)user_data);
    }
    *size = result;
    PyGILState_Release(gstate);
  }
  else {
    *size = *((real*)user_data);
  }
  */
 *size = *((real*)user_data);
 return STATUS_OK;
}

/*
 * The following function will be called for PreCAD/MG-CADSurf message
 * printing.  See context_set_message_callback (later in this
 * template) for how to set user_data.
 */
status_t message_cb(message_t *msg, void *user_data)
{
  integer errnumber = 0;
  char *desc;
  message_get_number(msg, &errnumber);
  message_get_description(msg, &desc);
  string err( desc );
  message_cb_user_data * mcud = (message_cb_user_data*)user_data;
  // Get all the error message and some warning messages related to license and periodicity
  if ( errnumber < 0 ||
       err.find("license"    ) != string::npos ||
       err.find("periodicity") != string::npos )
  {
    // remove ^A from the tail
    int len = strlen( desc );
    while (len > 0 && desc[len-1] != '\n')
      len--;
    mcud->_error->append( desc, len );
  }
  else {
    if ( errnumber == 3009001 )
      * mcud->_progress = atof( desc + 11 ) / 100.;
    if ( mcud->_verbosity > 0 )
      std::cout << desc << std::endl;
  }
  return STATUS_OK;
}

/* This is the interrupt callback. PreCAD/MG-CADSurf will call this
 * function regularily. See the file meshgems/interrupt.h
 */
status_t interrupt_cb(integer *interrupt_status, void *user_data)
{
  integer you_want_to_continue = 1;
  BLSURFPlugin_BLSURF* tmp = (BLSURFPlugin_BLSURF*)user_data;
  you_want_to_continue = !tmp->computeCanceled();

  if(you_want_to_continue)
  {
    *interrupt_status = INTERRUPT_CONTINUE;
    return STATUS_OK;
  }
  else /* you want to stop MG-CADSurf */
  {
    *interrupt_status = INTERRUPT_STOP;
    return STATUS_ERROR;
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================
bool BLSURFPlugin_BLSURF::Evaluate(SMESH_Mesh&         aMesh,
                                   const TopoDS_Shape& aShape,
                                   MapShapeNbElems&    aResMap)
{
  double diagonal       = aMesh.GetShapeDiagonalSize();
  double bbSegmentation = _gen->GetBoundaryBoxSegmentation();
  int    _physicalMesh  = BLSURFPlugin_Hypothesis::GetDefaultPhysicalMesh();
  double _phySize       = BLSURFPlugin_Hypothesis::GetDefaultPhySize(diagonal, bbSegmentation);
  bool   _phySizeRel    = BLSURFPlugin_Hypothesis::GetDefaultPhySizeRel();
  //int    _geometricMesh = BLSURFPlugin_Hypothesis::GetDefaultGeometricMesh();
  double _angleMesh     = BLSURFPlugin_Hypothesis::GetDefaultAngleMesh();
  bool   _quadAllowed   = BLSURFPlugin_Hypothesis::GetDefaultQuadAllowed();
  if(_hypothesis) {
    _physicalMesh  = (int) _hypothesis->GetPhysicalMesh();
    _phySizeRel         = _hypothesis->IsPhySizeRel();
    if ( _hypothesis->GetPhySize() > 0)
      _phySize          = _phySizeRel ? diagonal*_hypothesis->GetPhySize() : _hypothesis->GetPhySize();
    //_geometricMesh = (int) hyp->GetGeometricMesh();
    if (_hypothesis->GetAngleMesh() > 0)
      _angleMesh        = _hypothesis->GetAngleMesh();
    _quadAllowed        = _hypothesis->GetQuadAllowed();
  } else {
    //0020968: EDF1545 SMESH: Problem in the creation of a mesh group on geometry
    // GetDefaultPhySize() sometimes leads to computation failure
    _phySize = aMesh.GetShapeDiagonalSize() / _gen->GetBoundaryBoxSegmentation();
  }

  bool IsQuadratic = _quadraticMesh;

  // ----------------
  // evaluate 1D
  // ----------------
  TopTools_DataMapOfShapeInteger EdgesMap;
  double fullLen = 0.0;
  double fullNbSeg = 0;
  for (TopExp_Explorer exp(aShape, TopAbs_EDGE); exp.More(); exp.Next()) {
    TopoDS_Edge E = TopoDS::Edge( exp.Current() );
    if( EdgesMap.IsBound(E) )
      continue;
    SMESH_subMesh *sm = aMesh.GetSubMesh(E);
    double aLen = SMESH_Algo::EdgeLength(E);
    fullLen += aLen;
    int nb1d = 0;
    if(_physicalMesh==1) {
       nb1d = (int)( aLen/_phySize + 1 );
    }
    else {
      // use geometry
      double f,l;
      Handle(Geom_Curve) C = BRep_Tool::Curve(E,f,l);
      double fullAng = 0.0;
      double dp = (l-f)/200;
      gp_Pnt P1,P2,P3;
      C->D0(f,P1);
      C->D0(f+dp,P2);
      gp_Vec V1(P1,P2);
      for(int j=2; j<=200; j++) {
        C->D0(f+dp*j,P3);
        gp_Vec V2(P2,P3);
        fullAng += fabs(V1.Angle(V2));
        V1 = V2;
        P2 = P3;
      }
      nb1d = (int)( fullAng/_angleMesh + 1 );
    }
    fullNbSeg += nb1d;
    std::vector<int> aVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i]=0;
    if( IsQuadratic > 0 ) {
      aVec[SMDSEntity_Node] = 2*nb1d - 1;
      aVec[SMDSEntity_Quad_Edge] = nb1d;
    }
    else {
      aVec[SMDSEntity_Node] = nb1d - 1;
      aVec[SMDSEntity_Edge] = nb1d;
    }
    aResMap.insert(std::make_pair(sm,aVec));
    EdgesMap.Bind(E,nb1d);
  }
  double ELen = fullLen/fullNbSeg;
  // ----------------
  // evaluate 2D
  // ----------------
  // try to evaluate as in MEFISTO
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next()) {
    TopoDS_Face F = TopoDS::Face( exp.Current() );
    SMESH_subMesh *sm = aMesh.GetSubMesh(F);
    GProp_GProps G;
    BRepGProp::SurfaceProperties(F,G);
    double anArea = G.Mass();
    int nb1d = 0;
    std::vector<int> nb1dVec;
    for (TopExp_Explorer exp1(F,TopAbs_EDGE); exp1.More(); exp1.Next()) {
      int nbSeg = EdgesMap.Find(exp1.Current());
      nb1d += nbSeg;
      nb1dVec.push_back( nbSeg );
    }
    int nbQuad = 0;
    int nbTria = (int) ( anArea/( ELen*ELen*sqrt(3.) / 4 ) );
    int nbNodes = (int) ( ( nbTria*3 - (nb1d-1)*2 ) / 6 + 1 );
    if ( _quadAllowed )
    {
      if ( nb1dVec.size() == 4 ) // quadrangle geom face
      {
        int n1 = nb1dVec[0], n2 = nb1dVec[ nb1dVec[1] == nb1dVec[0] ? 2 : 1 ];
        nbQuad = n1 * n2;
        nbNodes = (n1 + 1) * (n2 + 1);
        nbTria = 0;
      }
      else
      {
        nbTria = nbQuad = nbTria / 3 + 1;
      }
    }
    std::vector<int> aVec(SMDSEntity_Last,0);
    if( IsQuadratic ) {
      int nb1d_in = (nbTria*3 - nb1d) / 2;
      aVec[SMDSEntity_Node] = nbNodes + nb1d_in;
      aVec[SMDSEntity_Quad_Triangle] = nbTria;
      aVec[SMDSEntity_Quad_Quadrangle] = nbQuad;
    }
    else {
      aVec[SMDSEntity_Node] = nbNodes;
      aVec[SMDSEntity_Triangle] = nbTria;
      aVec[SMDSEntity_Quadrangle] = nbQuad;
    }
    aResMap.insert(std::make_pair(sm,aVec));
  }

  // ----------------
  // evaluate 3D
  // ----------------
  GProp_GProps G;
  BRepGProp::VolumeProperties(aShape,G);
  double aVolume = G.Mass();
  double tetrVol = 0.1179*ELen*ELen*ELen;
  int nbVols  = int(aVolume/tetrVol);
  int nb1d_in = int(( nbVols*6 - fullNbSeg ) / 6 );
  std::vector<int> aVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i]=0;
  if( IsQuadratic ) {
    aVec[SMDSEntity_Node] = nb1d_in/3 + 1 + nb1d_in;
    aVec[SMDSEntity_Quad_Tetra] = nbVols;
  }
  else {
    aVec[SMDSEntity_Node] = nb1d_in/3 + 1;
    aVec[SMDSEntity_Tetra] = nbVols;
  }
  SMESH_subMesh *sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}
