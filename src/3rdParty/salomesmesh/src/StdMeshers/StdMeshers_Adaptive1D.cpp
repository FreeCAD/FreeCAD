// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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
//  File   : StdMeshers_Adaptive1D.cxx
//  Module : SMESH
//
#include "StdMeshers_Adaptive1D.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_Octree.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_HypoFilter.hxx"

#include <Utils_SALOME_Exception.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B3d.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Version.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>

#include <limits>
#include <vector>
#include <set>

using namespace std;

namespace // internal utils
{
  //================================================================================
  /*!
   * \brief Bnd_B3d with access to its center and half-size
   */
  struct BBox : public Bnd_B3d
  {
    gp_XYZ Center() const { return gp_XYZ( myCenter[0], myCenter[1], myCenter[2] ); }
    gp_XYZ HSize()  const { return gp_XYZ( myHSize[0],  myHSize[1],  myHSize[2]  ); }
    double Size()   const { return 2 * myHSize[0]; }
  };
  //================================================================================
  /*!
   * \brief Working data of an EDGE
   */
  struct EdgeData
  {
    struct ProbePnt
    {
      gp_Pnt myP;
      double myU;
      double mySegSize;
      ProbePnt( gp_Pnt p, double u, double sz=1e100 ): myP( p ), myU( u ), mySegSize( sz ) {}
    };
    BRepAdaptor_Curve myC3d;
    double            myLength;
    list< ProbePnt >  myPoints;
    BBox              myBBox;

    typedef list< ProbePnt >::iterator TPntIter;
    void AddPoint( TPntIter where, double u )
    {
      TPntIter it = myPoints.insert( where, ProbePnt( myC3d.Value( u ), u ));
      myBBox.Add( it->myP.XYZ() );
    }
    const ProbePnt& First() const { return myPoints.front(); }
    const ProbePnt& Last()  const { return myPoints.back(); }
    const TopoDS_Edge& Edge() const { return myC3d.Edge(); }
    bool IsTooDistant( const BBox& faceBox, double maxSegSize ) const
    {
      gp_XYZ hsize = myBBox.HSize() + gp_XYZ( maxSegSize, maxSegSize, maxSegSize );
      return faceBox.IsOut ( Bnd_B3d( myBBox.Center(), hsize ));
    }
  };
  //================================================================================
  /*!
   * \brief Octree of local segment size
   */
  class SegSizeTree : public SMESH_Octree
  {
    double mySegSize; // segment size

    // structure holding some common parameters of SegSizeTree
    struct _CommonData : public SMESH_TreeLimit
    {
      double myGrading, myMinSize, myMaxSize;
    };
    _CommonData* getData() const { return (_CommonData*) myLimit; }

    SegSizeTree(double size): SMESH_Octree(), mySegSize(size)
    {
      allocateChildren();
    }
    void allocateChildren()
    {
      myChildren = new SMESH_Octree::TBaseTree*[nbChildren()];
      for ( int i = 0; i < nbChildren(); ++i )
        myChildren[i] = NULL;
    }
    virtual box_type* buildRootBox() { return 0; }
    virtual SegSizeTree* newChild() const { return 0; }
    virtual void buildChildrenData() {}

  public:

    SegSizeTree( Bnd_B3d & bb, double grading, double mixSize, double maxSize);
    void   SetSize( const gp_Pnt& p, double size );
    double SetSize( const gp_Pnt& p1, const gp_Pnt& p2 );
    double GetSize( const gp_Pnt& p ) const;
    const BBox* GetBox() const { return (BBox*) getBox(); }
    double GetMinSize() { return getData()->myMinSize; }
  };
  //================================================================================
  /*!
   * \brief Adaptive wire discertizator.
   */
  class AdaptiveAlgo : public StdMeshers_Regular_1D
  {
  public:
    AdaptiveAlgo(int hypId, int studyId, SMESH_Gen* gen);
    virtual bool Compute(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape );
    virtual bool Evaluate(SMESH_Mesh &         theMesh,
                          const TopoDS_Shape & theShape,
                          MapShapeNbElems&     theResMap);
    void SetHypothesis( const StdMeshers_Adaptive1D* hyp );
  private:

    bool makeSegments();

    const StdMeshers_Adaptive1D* myHyp;
    SMESH_Mesh*                  myMesh;
    vector< EdgeData >           myEdges;
    SegSizeTree*                 mySizeTree;
  };

  //================================================================================
  /*!
   * \brief Segment of Poly_PolygonOnTriangulation
   */
  struct Segment
  {
    gp_XYZ myPos, myDir;
    double myLength;

    void Init( const gp_Pnt& p1, const gp_Pnt& p2 )
    {
      myPos    = p1.XYZ();
      myDir    = p2.XYZ() - p1.XYZ();
      myLength = myDir.Modulus();
      if ( myLength > std::numeric_limits<double>::min() )
        myDir /= myLength;
    }
    bool Distance( const gp_Pnt& P, double& dist ) const // returns length of normal projection
    {
      gp_XYZ p = P.XYZ();
      p.Subtract( myPos );
      double proj = p.Dot( myDir );
      if ( 0 < proj && proj < myLength )
      {
        p.Cross( myDir );
        dist = p.Modulus();
        return true;
      }
      return false;
    }
  };
  //================================================================================
  /*!
   * \brief Data of triangle used to locate it in an octree and to find distance
   *        to a point
   */
  struct Triangle
  {
    Bnd_B3d  myBox;
    bool     myIsChecked; // to mark treated trias instead of using std::set
    bool     myHasNodeOnVertex;
    Segment* mySegments[3];
    // data for DistToProjection()
    gp_XYZ   myN0, myEdge1, myEdge2, myNorm, myPVec;
    double   myInvDet, myMaxSize2;

    void Init( const gp_Pnt& n1, const gp_Pnt& n2, const gp_Pnt& n3 ); 
    bool DistToProjection( const gp_Pnt& p, double& dist ) const;
    bool DistToSegment   ( const gp_Pnt& p, double& dist ) const;
  };
  //================================================================================
  /*!
   * \brief Element data held by ElementBndBoxTree + algorithm computing a distance
   *        from a point to element
   */
  class ElementBndBoxTree;
  struct ElemTreeData : public SMESH_TreeLimit
  {
    vector< int >                myWorkIDs[8];// to speed up filling ElementBndBoxTree::_elementIDs
    virtual const Bnd_B3d* GetBox(int elemID) const = 0;
  };
  struct TriaTreeData : public ElemTreeData
  {
    vector< Triangle >           myTrias;
    vector< Segment >            mySegments;
    double                       myFaceTol;
    double                       myTriasDeflection;
    BBox                         myBBox;
    BRepAdaptor_Surface          mySurface;
    ElementBndBoxTree*           myTree;
    const Poly_Array1OfTriangle* myPolyTrias;
    const TColgp_Array1OfPnt*    myNodes;
    bool                         myOwnNodes;

    typedef vector<int> IntVec;
    IntVec                       myFoundTriaIDs;

    TriaTreeData( const TopoDS_Face& face, ElementBndBoxTree* triaTree );
    ~TriaTreeData() { if ( myOwnNodes ) delete myNodes; myNodes = NULL; }
    virtual const Bnd_B3d* GetBox(int elemID) const { return &myTrias[elemID].myBox; }
    void PrepareToTriaSearch();
    void SetSizeByTrias( SegSizeTree& sizeTree, double deflection ) const;
    double GetMinDistInSphere(const gp_Pnt& p,
                              const double  radius,
                              const bool    projectedOnly,
                              const gp_Pnt* avoidP=0) const;
  };
  //================================================================================
  /*!
   * \brief Octree of triangles or segments
   */
  class ElementBndBoxTree : public SMESH_Octree
  {
  public:
    ElementBndBoxTree(const TopoDS_Face& face);
    void GetElementsInSphere( const gp_XYZ& center,
                              const double  radius, vector<int> & foundElemIDs) const;
    void FillIn();
    ElemTreeData* GetElemData() const { return (ElemTreeData*) myLimit; }
    TriaTreeData* GetTriaData() const { return (TriaTreeData*) myLimit; }

  protected:
    ElementBndBoxTree() {}
    SMESH_Octree* newChild() const { return new ElementBndBoxTree; }
    void          buildChildrenData();
    Bnd_B3d*      buildRootBox();
  private:
    vector< int > _elementIDs;
  };
  //================================================================================
  /*!
   * \brief Link of two nodes
   */
  struct NLink : public std::pair< int, int >
  {
    NLink( int n1, int n2 )
    {
      if ( n1 < n2 )
      {
        first  = n1;
        second = n2;
      }
      else
      {
        first  = n2;
        second = n1;
      }
    }
    int N1() const { return first; }
    int N2() const { return second; }
  };

  //================================================================================
  /*!
   * \brief Initialize TriaTreeData
   */
  //================================================================================

  TriaTreeData::TriaTreeData( const TopoDS_Face& face, ElementBndBoxTree* triaTree )
    : myTriasDeflection(0), mySurface( face ),
      myTree(NULL), myPolyTrias(NULL), myNodes(NULL), myOwnNodes(false)
  {
    TopLoc_Location loc;
    Handle(Poly_Triangulation) tr = BRep_Tool::Triangulation( face, loc );
    if ( !tr.IsNull() )
    {
      myFaceTol         = SMESH_MesherHelper::MaxTolerance( face );
      myTree            = triaTree;
#if OCC_VERSION_HEX < 0x070600
      myNodes           = & tr->Nodes();
#else
      TColgp_Array1OfPnt* trNodes = new TColgp_Array1OfPnt( 1, tr->NbNodes() );
      for (Standard_Integer i = myNodes->Lower(); i <= myNodes->Upper(); i++)
      {
        trNodes->SetValue(i, tr->Node(i));
      }
      myNodes = trNodes;
      myOwnNodes = true;
#endif
      myPolyTrias       = & tr->Triangles();
      myTriasDeflection = tr->Deflection();
      if ( !loc.IsIdentity() ) // transform nodes if necessary
      {
        TColgp_Array1OfPnt* trsfNodes = new TColgp_Array1OfPnt( myNodes->Lower(), myNodes->Upper() );
        trsfNodes->Assign( *myNodes );
#if OCC_VERSION_HEX >= 0x070600
        delete myNodes; // it's already a copy
#endif
        myNodes    = trsfNodes;
        myOwnNodes = true;
        const gp_Trsf& trsf = loc;
        for ( int i = trsfNodes->Lower(); i <= trsfNodes->Upper(); ++i )
          trsfNodes->ChangeValue(i).Transform( trsf );
      }
      for ( int i = myNodes->Lower(); i <= myNodes->Upper(); ++i )
        myBBox.Add( myNodes->Value(i).XYZ() );
    }
  }
  //================================================================================
  /*!
   * \brief Prepare data for search of trinagles in GetMinDistInSphere()
   */
  //================================================================================

  void TriaTreeData::PrepareToTriaSearch()
  {
    if ( !myTrias.empty() ) return; // already done
    if ( !myPolyTrias ) return;

    // get all boundary links and nodes on VERTEXes
    map< NLink, Segment* > linkToSegMap;
    map< NLink, Segment* >::iterator l2s;
    set< int > vertexNodes;
    TopLoc_Location loc;
    Handle(Poly_Triangulation) tr = BRep_Tool::Triangulation( mySurface.Face(), loc );
    if ( !tr.IsNull() )
    {
      TopTools_IndexedMapOfShape edgeMap;
      TopExp::MapShapes( mySurface.Face(), TopAbs_EDGE, edgeMap );
      for ( int iE = 1; iE <= edgeMap.Extent(); ++iE )
      {
        const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( iE ));
        Handle(Poly_PolygonOnTriangulation) polygon =
          BRep_Tool::PolygonOnTriangulation( edge, tr, loc );
        if ( polygon.IsNull()  )
          continue;
        const TColStd_Array1OfInteger& nodes = polygon->Nodes();
        for ( int i = nodes.Lower(); i < nodes.Upper(); ++i )
          linkToSegMap.insert( make_pair( NLink( nodes(i), nodes(i+1)), (Segment*)0 ));
        vertexNodes.insert( nodes( nodes.Lower()));
        vertexNodes.insert( nodes( nodes.Upper()));
      }
      // fill mySegments by boundary links
      mySegments.resize( linkToSegMap.size() );
      int iS = 0;
      for ( l2s = linkToSegMap.begin(); l2s != linkToSegMap.end(); ++l2s, ++iS )
      {
        const NLink& link = (*l2s).first;
        (*l2s).second = & mySegments[ iS ];
        mySegments[ iS ].Init( myNodes->Value( link.N1() ),
                               myNodes->Value( link.N2() ));
      }
    }

    // initialize myTrias
    myTrias.resize( myPolyTrias->Length() );
    Standard_Integer n1,n2,n3;
    for ( int i = 1; i <= myPolyTrias->Upper(); ++i )
    {
      Triangle & t = myTrias[ i-1 ];
      myPolyTrias->Value( i ).Get( n1,n2,n3 );
      t.Init( myNodes->Value( n1 ),
              myNodes->Value( n2 ),
              myNodes->Value( n3 ));
      int nbSeg = 0;
      if (( l2s = linkToSegMap.find( NLink( n1, n2 ))) != linkToSegMap.end())
        t.mySegments[ nbSeg++ ] = l2s->second;
      if (( l2s = linkToSegMap.find( NLink( n2, n3 ))) != linkToSegMap.end())
        t.mySegments[ nbSeg++ ] = l2s->second;
      if (( l2s = linkToSegMap.find( NLink( n3, n1 ))) != linkToSegMap.end())
        t.mySegments[ nbSeg++ ] = l2s->second;
      while ( nbSeg < 3 )
        t.mySegments[ nbSeg++ ] = NULL;

      t.myIsChecked = false;
      t.myHasNodeOnVertex = ( vertexNodes.count( n1 ) ||
                              vertexNodes.count( n2 ) ||
                              vertexNodes.count( n3 ));
    }

    // fill the tree of triangles
    myTree->FillIn();
  }

  //================================================================================
  /*!
   * \brief Set size of segments by size of triangles
   */
  //================================================================================

  void TriaTreeData::SetSizeByTrias( SegSizeTree& sizeTree, double hypDeflection ) const
  {
    if ( mySurface.GetType() == GeomAbs_Plane ||
         myTriasDeflection   <= 1e-100 )
      return;
    const double factor = hypDeflection / myTriasDeflection;

    bool isConstSize;
    switch( mySurface.GetType() ) {
    case GeomAbs_Cylinder:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
      isConstSize = true; break;
    default:
      isConstSize = false;
    }

    map< NLink, double >           lenOfDoneLink;
    map< NLink, double >::iterator link2len;

    Standard_Integer n[4];
    gp_Pnt p[4];
    double a[3];
    bool   isDone[3];
    double size = -1., maxLinkLen;
    int    jLongest;

    //int nbLinks = 0;
    for ( int i = 1; i <= myPolyTrias->Upper(); ++i )
    {
      // get corners of a triangle
      myPolyTrias->Value( i ).Get( n[0],n[1],n[2] );
      n[3] = n[0];
      p[0] = myNodes->Value( n[0] );
      p[1] = myNodes->Value( n[1] );
      p[2] = myNodes->Value( n[2] );
      p[3] = p[0];
      // get length of links and find the longest one
      maxLinkLen = 0;
      for ( int j = 0; j < 3; ++j )
      {
        link2len  = lenOfDoneLink.insert( make_pair( NLink( n[j], n[j+1] ), -1. )).first;
        isDone[j] = !((*link2len).second < 0 );
        a[j]      = isDone[j] ? (*link2len).second : (*link2len).second = p[j].Distance( p[j+1] );
        if ( isDone[j] )
          lenOfDoneLink.erase( link2len );
        if ( a[j] > maxLinkLen )
        {
          maxLinkLen = a[j];
          jLongest   = j;
        }
      }
      // compute minimal altitude of a triangle
      if ( !isConstSize || size < 0. )
      {
        double s    = 0.5 * ( a[0] + a[1] + a[2] );
        double area = sqrt( s * (s - a[0]) * (s - a[1]) * (s - a[2]));
        size        = 2 * area / maxLinkLen; // minimal altitude
      }
      // set size to the size tree
      if ( !isDone[ jLongest ] || !isConstSize )
      {
        //++nbLinks;
        if ( size < numeric_limits<double>::min() )
          continue;
        int nb = Max( 1, int( maxLinkLen / size / 2 ));
        for ( int k = 0; k <= nb; ++k )
        {
          double r = double( k ) / nb;
          sizeTree.SetSize( r * p[ jLongest ].XYZ() + ( 1-r ) * p[ jLongest+1 ].XYZ(),
                            size * factor );
        }
      }
      //cout << "SetSizeByTrias, i="<< i << " " << sz * factor << endl;
    }
    // cout << "SetSizeByTrias, nn tria="<< myPolyTrias->Upper()
    //      << " nb links" << nbLinks << " isConstSize="<<isConstSize
    //      << " " << size * factor << endl;
  }
  //================================================================================
  /*!
   * \brief Return minimal distance from a given point to a trinangle but not more
   *        distant than a given radius. Triangles with a node at avoidPnt are ignored.
   *        If projectedOnly, 
   */
  //================================================================================

  double TriaTreeData::GetMinDistInSphere(const gp_Pnt& p,
                                          const double  radius,
                                          const bool    projectedOnly,
                                          const gp_Pnt* avoidPnt) const
  {
    double minDist2 = 1e100;
    const double tol2 = myFaceTol * myFaceTol;
    const double dMin2 = myTriasDeflection * myTriasDeflection;

    TriaTreeData* me = const_cast<TriaTreeData*>( this );
    me->myFoundTriaIDs.clear();
    myTree->GetElementsInSphere( p.XYZ(), radius, me->myFoundTriaIDs );
    if ( myFoundTriaIDs.empty() )
      return minDist2;

    Standard_Integer n[ 3 ];
    for ( size_t i = 0; i < myFoundTriaIDs.size(); ++i )
    {
      Triangle& t = me->myTrias[ myFoundTriaIDs[i] ];
      if ( t.myIsChecked )
        continue;
      t.myIsChecked = true;

      double d, minD2 = minDist2;
      myPolyTrias->Value( myFoundTriaIDs[i]+1 ).Get( n[0],n[1],n[2] );
      if ( avoidPnt && t.myHasNodeOnVertex )
      {
        bool avoidTria = false;
        for ( int i = 0; i < 3; ++i )
        {
          const gp_Pnt& pn = myNodes->Value(n[i]);
          if ( ( avoidTria = ( pn.SquareDistance( *avoidPnt ) <= tol2 )))
            break;
          if ( !projectedOnly )
            minD2 = Min( minD2, pn.SquareDistance( p ));
        }
        if ( avoidTria )
          continue;
        if (( projectedOnly || minD2 < t.myMaxSize2 ) &&
            ( t.DistToProjection( p, d ) || t.DistToSegment( p, d )))
          minD2 = Min( minD2, d*d );
        minDist2 = Min( minDist2, minD2 );
      }
      else if ( projectedOnly )
      {
        if ( t.DistToProjection( p, d ) && d*d > dMin2 )
          minDist2 = Min( minDist2, d*d );
      }
      else
      {
        for ( int i = 0; i < 3; ++i )
          minD2 = Min( minD2, p.SquareDistance( myNodes->Value(n[i]) ));
        if ( minD2 < t.myMaxSize2  && ( t.DistToProjection( p, d ) || t.DistToSegment( p, d )))
          minD2 = Min( minD2, d*d );
        minDist2 = Min( minDist2, minD2 );
      }
    }

    for ( size_t i = 0; i < myFoundTriaIDs.size(); ++i )
      me->myTrias[ myFoundTriaIDs[i] ].myIsChecked = false;

    return sqrt( minDist2 );
  }
  //================================================================================
  /*!
   * \brief Prepare Triangle data
   */
  //================================================================================

  void Triangle::Init( const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3 )
  {
    myBox.Add( p1 );
    myBox.Add( p2 );
    myBox.Add( p3 );
    myN0 = p1.XYZ();
    myEdge1 = p2.XYZ() - myN0;
    myEdge2 = p3.XYZ() - myN0;
    myNorm = myEdge1 ^ myEdge2;
    double normSize = myNorm.Modulus();
    if ( normSize > std::numeric_limits<double>::min() )
    {
      myNorm /= normSize;
      myPVec = myNorm ^ myEdge2;
      myInvDet = 1. / ( myEdge1 * myPVec );
    }
    else
    {
      myInvDet = 0.;
    }
    myMaxSize2 = Max( p2.SquareDistance( p3 ),
                      Max( myEdge2.SquareModulus(), myEdge1.SquareModulus() ));
  }
  //================================================================================
  /*!
   * \brief Compute distance from a point to the triangle. Return false if the point
   *        is not projected inside the triangle
   */
  //================================================================================

  bool Triangle::DistToProjection( const gp_Pnt& p, double& dist ) const
  {
    if ( myInvDet == 0 )
      return false; // degenerated triangle

    /* distance from n0 to the point */
    gp_XYZ tvec = p.XYZ() - myN0;

    /* calculate U parameter and test bounds */
    double u = ( tvec * myPVec ) * myInvDet;
    if (u < 0.0 || u > 1.0)
      return false; // projected outside the triangle

    /* calculate V parameter and test bounds */
    gp_XYZ qvec = tvec ^ myEdge1;
    double v = ( myNorm * qvec) * myInvDet;
    if ( v < 0.0 || u + v > 1.0 )
      return false; // projected outside the triangle

    dist = ( myEdge2 * qvec ) * myInvDet;
    return true;
  }

  //================================================================================
  /*!
   * \brief Compute distance from a point to either of mySegments. Return false if the point
   *        is not projected on a segment
   */
  //================================================================================

  bool Triangle::DistToSegment( const gp_Pnt& p, double& dist ) const
  {
    dist = 1e100;
    bool res = false;
    double d;
    for ( int i = 0; i < 3; ++i )
    {
      if ( !mySegments[ i ])
        break;
      if ( mySegments[ i ]->Distance( p, d ))
      {
        res = true;
        dist = Min( dist, d );
      }
    }
    return res;
  }

  //================================================================================
  /*!
   * \brief Consturct ElementBndBoxTree of Poly_Triangulation of a FACE
   */
  //================================================================================

  ElementBndBoxTree::ElementBndBoxTree(const TopoDS_Face& face)
    :SMESH_Octree()
  {
    TriaTreeData* data = new TriaTreeData( face, this );
    data->myMaxLevel = 5;
    myLimit = data;
  }
  //================================================================================
  /*!
   * \brief Fill all levels of octree of Poly_Triangulation of a FACE
   */
  //================================================================================

  void ElementBndBoxTree::FillIn()
  {
    if ( myChildren ) return;
    TriaTreeData* data = GetTriaData();
    if ( !data->myTrias.empty() )
    {
      for ( size_t i = 0; i < data->myTrias.size(); ++i )
        _elementIDs.push_back( i );

      compute();
    }
  }
  //================================================================================
  /*!
   * \brief Return the maximal box
   */
  //================================================================================

  Bnd_B3d* ElementBndBoxTree::buildRootBox()
  {
    TriaTreeData* data = GetTriaData();
    Bnd_B3d*       box = new Bnd_B3d( data->myBBox );
    return box;
  }
  //================================================================================
  /*!
   * \brief Redistrubute element boxes among children
   */
  //================================================================================

  void ElementBndBoxTree::buildChildrenData()
  {
    ElemTreeData* data = GetElemData();
    for ( int i = 0; i < _elementIDs.size(); ++i )
    {
      const Bnd_B3d* elemBox = data->GetBox( _elementIDs[i] );
      for (int j = 0; j < 8; j++)
        if ( !elemBox->IsOut( *myChildren[ j ]->getBox() ))
          data->myWorkIDs[ j ].push_back( _elementIDs[i] );
    }
    SMESHUtils::FreeVector( _elementIDs ); // = _elements.clear() + free memory

    const int theMaxNbElemsInLeaf = 7;

    for (int j = 0; j < 8; j++)
    {
      ElementBndBoxTree* child = static_cast<ElementBndBoxTree*>( myChildren[j] );
      child->_elementIDs = data->myWorkIDs[ j ];
      if ( child->_elementIDs.size() <= theMaxNbElemsInLeaf )
        child->myIsLeaf = true;
      data->myWorkIDs[ j ].clear();
    }
  }
  //================================================================================
  /*!
   * \brief Return elements from leaves intersecting the sphere
   */
  //================================================================================

  void ElementBndBoxTree::GetElementsInSphere( const gp_XYZ& center,
                                               const double  radius,
                                               vector<int> & foundElemIDs) const
  {
    if ( const box_type* box = getBox() )
    {
      if ( box->IsOut( center, radius ))
        return;

      if ( isLeaf() )
      {
        ElemTreeData* data = GetElemData();
        for ( int i = 0; i < _elementIDs.size(); ++i )
          if ( !data->GetBox( _elementIDs[i] )->IsOut( center, radius ))
            foundElemIDs.push_back( _elementIDs[i] );
      }
      else
      {
        for (int i = 0; i < 8; i++)
          ((ElementBndBoxTree*) myChildren[i])->GetElementsInSphere( center, radius, foundElemIDs );
      }
    }
  }

  //================================================================================
  /*!
   * \brief Constructor of SegSizeTree
   *  \param [in,out] bb - bounding box enclosing all EDGEs to discretize 
   *  \param [in] grading - factor to get max size of the neighbour segment by 
   *         size of a current one.
   */
  //================================================================================

  SegSizeTree::SegSizeTree( Bnd_B3d & bb, double grading, double minSize, double maxSize )
    : SMESH_Octree( new _CommonData() )
  {
    // make cube myBox from the box bb
    gp_XYZ pmin = bb.CornerMin(), pmax = bb.CornerMax();
    double maxBoxHSize = 0.5 * Max( pmax.X()-pmin.X(), Max( pmax.Y()-pmin.Y(), pmax.Z()-pmin.Z() ));
    maxBoxHSize *= 1.01;
    bb.SetHSize( gp_XYZ( maxBoxHSize, maxBoxHSize, maxBoxHSize ));
    myBox = new box_type( bb );

    mySegSize = Min( 2 * maxBoxHSize, maxSize );

    getData()->myGrading = grading;
    getData()->myMinSize = Max( minSize, 2*maxBoxHSize / 1.e6 );
    getData()->myMaxSize = maxSize;
    allocateChildren();
  }

  //================================================================================
  /*!
   * \brief Set segment size at a given point
   */
  //================================================================================

  void SegSizeTree::SetSize( const gp_Pnt& p, double size )
  {
    // check if the point is out of the largest cube
    SegSizeTree* root = this;
    while ( root->myFather )
      root = (SegSizeTree*) root->myFather;
    if ( root->getBox()->IsOut( p.XYZ() ))
      return;

    // keep size whthin the valid range
    size = Max( size, getData()->myMinSize );
    //size = Min( size, getData()->myMaxSize );

    // find an existing leaf at the point
    SegSizeTree* leaf = (SegSizeTree*) root;
    int iChild;
    while ( true )
    {
      iChild = SMESH_Octree::getChildIndex( p.X(), p.Y(), p.Z(), leaf->GetBox()->Center() );
      if ( leaf->myChildren[ iChild ] )
        leaf = (SegSizeTree*) leaf->myChildren[ iChild ];
      else
        break;
    }
    // don't increase the current size
    if ( leaf->mySegSize <= 1.1 * size )
      return;

    // split the found leaf until its box size is less than the given size
    const double rootSize = root->GetBox()->Size();
    while ( leaf->GetBox()->Size() > size )
    {
      const BBox* bb = leaf->GetBox();
      iChild   = SMESH_Octree::getChildIndex( p.X(), p.Y(), p.Z(), bb->Center() );
      SegSizeTree* newLeaf = new SegSizeTree( bb->Size() / 2 );
      leaf->myChildren[iChild] = newLeaf;
      newLeaf->myFather = leaf;
      newLeaf->myLimit  = leaf->myLimit;
      newLeaf->myLevel  = leaf->myLevel + 1;
      newLeaf->myBox    = leaf->newChildBox( iChild );
      newLeaf->myBox->Enlarge( rootSize * 1e-10 );
      //newLeaf->myIsLeaf = ( newLeaf->mySegSize <= size );
      leaf = newLeaf;
    }
    leaf->mySegSize = size;

    // propagate increased size out from the leaf
    double boxSize = leaf->GetBox()->Size();
    double sizeInc = size + boxSize * getData()->myGrading;
    for ( int iDir = 1; iDir <= 3; ++iDir )
    {
      gp_Pnt outPnt = p;
      outPnt.SetCoord( iDir, p.Coord( iDir ) + boxSize );
      SetSize( outPnt, sizeInc );
      outPnt.SetCoord( iDir, p.Coord( iDir ) - boxSize );
      SetSize( outPnt, sizeInc );
    }
  }
  //================================================================================
  /*!
   * \brief Set size of a segment given by two end points
   */
  //================================================================================

  double SegSizeTree::SetSize( const gp_Pnt& p1, const gp_Pnt& p2 )
  {
    const double size = p1.Distance( p2 );
    gp_XYZ p = 0.5 * ( p1.XYZ() + p2.XYZ() );
    SetSize( p, size );
    SetSize( p1, size );
    SetSize( p2, size );
    //cout << "SetSize " << p1.Distance( p2 ) << " at " << p.X() <<", "<< p.Y()<<", "<<p.Z()<< endl;
    return GetSize( p );
  }

  //================================================================================
  /*!
   * \brief Return segment size at a point
   */
  //================================================================================

  double SegSizeTree::GetSize( const gp_Pnt& p ) const
  {
    const SegSizeTree* leaf = this;
    while ( true )
    {
      int iChild = SMESH_Octree::getChildIndex( p.X(), p.Y(), p.Z(), leaf->GetBox()->Center() );
      if ( leaf->myChildren[ iChild ] )
        leaf = (SegSizeTree*) leaf->myChildren[ iChild ];
      else
        return leaf->mySegSize;
    }
    return mySegSize; // just to return anything
  }

  //================================================================================
  /*!
   * \brief Evaluate curve deflection between two points
   * \param theCurve - the curve
   * \param theU1 - the parameter of the first point
   * \param theU2 - the parameter of the second point
   * \retval double - square deflection value
   */
  //================================================================================

  double deflection2(const BRepAdaptor_Curve & theCurve,
                     double                    theU1,
                     double                    theU2)
  {
    // line between theU1 and theU2
    gp_Pnt p1 = theCurve.Value( theU1 ), p2 = theCurve.Value( theU2 );
    gp_Lin segment( p1, gp_Vec( p1, p2 ));

    // evaluate square distance of theCurve from the segment
    Standard_Real dist2 = 0;
    const int nbPnt = 5;
    const double step = ( theU2 - theU1 ) / nbPnt;
    while (( theU1 += step ) < theU2 )
      dist2 = Max( dist2, segment.SquareDistance( theCurve.Value( theU1 )));

    return dist2;
  }

} // namespace

//=======================================================================
//function : StdMeshers_Adaptive1D
//purpose  : Constructor
StdMeshers_Adaptive1D::StdMeshers_Adaptive1D(int         hypId,
                                             int         studyId,
                                             SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  myMinSize       = 1e-10;
  myMaxSize       = 1e+10;
  myDeflection    = 1e-2;
  myAlgo          = NULL;
  _name           = "Adaptive1D";
  _param_algo_dim = 1; // is used by SMESH_Regular_1D
}
//=======================================================================
//function : ~StdMeshers_Adaptive1D
//purpose  : Destructor
StdMeshers_Adaptive1D::~StdMeshers_Adaptive1D()
{
  delete myAlgo; myAlgo = NULL;
}
//=======================================================================
//function : SetDeflection
//purpose  : 
void StdMeshers_Adaptive1D::SetDeflection(double value)
{
  if (value <= std::numeric_limits<double>::min() )
    throw SALOME_Exception("Deflection must be greater that zero");
  if (myDeflection != value)
  {
    myDeflection = value;
    NotifySubMeshesHypothesisModification();
  }
}
//=======================================================================
//function : SetMinSize
//purpose  : Sets minimal allowed segment length
void StdMeshers_Adaptive1D::SetMinSize(double minSize)
{
  if (minSize <= std::numeric_limits<double>::min() )
    throw SALOME_Exception("Min size must be greater that zero");

  if (myMinSize != minSize )
  {
    myMinSize = minSize;
    NotifySubMeshesHypothesisModification();
  }
}
//=======================================================================
//function : SetMaxSize
//purpose  : Sets maximal allowed segment length
void StdMeshers_Adaptive1D::SetMaxSize(double maxSize)
{
  if (maxSize <= std::numeric_limits<double>::min() )
    throw SALOME_Exception("Max size must be greater that zero");

  if (myMaxSize != maxSize )
  {
    myMaxSize = maxSize;
    NotifySubMeshesHypothesisModification();
  }
}
//=======================================================================
//function : SaveTo
//purpose  : Persistence
ostream & StdMeshers_Adaptive1D::SaveTo(ostream & save)
{
  save << myMinSize << " " << myMaxSize << " " << myDeflection;
  save << " " << -1 << " " << -1; // preview addition of parameters
  return save;
}
//=======================================================================
//function : LoadFrom
//purpose  : Persistence
istream & StdMeshers_Adaptive1D::LoadFrom(istream & load)
{
  int dummyParam;
  bool isOK = (bool)(load >> myMinSize >> myMaxSize >> myDeflection >> dummyParam >> dummyParam);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  return load;
}
//=======================================================================
//function : SetParametersByMesh
//purpose  : Initialize parameters by the mesh built on the geometry
//param theMesh - the built mesh
//param theShape - the geometry of interest
//retval bool - true if parameter values have been successfully defined
bool StdMeshers_Adaptive1D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );

  SMESH_MesherHelper helper( (SMESH_Mesh&) *theMesh );
  double minSz2 = 1e100, maxSz2 = 0, sz2, maxDefl2 = 0;
  for ( int iE = 1; iE <= edgeMap.Extent(); ++iE )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( iE ));
    SMESHDS_SubMesh* smDS = theMesh->GetMeshDS()->MeshElements( edge );
    if ( !smDS ) continue;
    ++nbEdges;

    helper.SetSubShape( edge );
    BRepAdaptor_Curve curve( edge );

    SMDS_ElemIteratorPtr segIt = smDS->GetElements();
    while ( segIt->more() )
    {
      const SMDS_MeshElement* seg = segIt->next();
      const SMDS_MeshNode* n1 = seg->GetNode(0);
      const SMDS_MeshNode* n2 = seg->GetNode(1);
      sz2 = SMESH_TNodeXYZ( n1 ).SquareDistance( n2 );
      minSz2 = Min( minSz2, sz2 );
      maxSz2 = Max( maxSz2, sz2 );
      if ( curve.GetType() != GeomAbs_Line )
      {
        double u1 = helper.GetNodeU( edge, n1, n2 );
        double u2 = helper.GetNodeU( edge, n2, n1 );
        maxDefl2 = Max( maxDefl2, deflection2( curve, u1, u2 ));
      }
    }
  }
  if ( nbEdges )
  {
    myMinSize = sqrt( minSz2 );
    myMaxSize = sqrt( maxSz2 );
    if ( maxDefl2 > 0 )
      myDeflection = maxDefl2;
  }
  return nbEdges;
}

//=======================================================================
//function : SetParametersByDefaults
//purpose  : Initialize my parameter values by default parameters.
//retval   : bool - true if parameter values have been successfully defined
bool StdMeshers_Adaptive1D::SetParametersByDefaults(const TDefaults&  dflts,
                                                    const SMESH_Mesh* /*theMesh*/)
{
  myMinSize = dflts._elemLength / 10;
  myMaxSize = dflts._elemLength * 2;
  myDeflection = myMinSize / 7;
  return true;
}

//=======================================================================
//function : GetAlgo
//purpose  : Returns an algorithm that works using this hypothesis
//=======================================================================

SMESH_Algo* StdMeshers_Adaptive1D::GetAlgo() const
{
  if ( !myAlgo )
  {
    AdaptiveAlgo* newAlgo = 
      new AdaptiveAlgo( _gen->GetANewId(), _studyId, _gen );
    newAlgo->SetHypothesis( this );

    ((StdMeshers_Adaptive1D*) this)->myAlgo = newAlgo;
  }
  return myAlgo;
}

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

AdaptiveAlgo::AdaptiveAlgo(int        hypId,
                           int        studyId,
                           SMESH_Gen* gen)
  : StdMeshers_Regular_1D( hypId, studyId, gen ),
    myHyp(NULL)
{
  _name = "AdaptiveAlgo_1D";
}

//================================================================================
/*!
 * \brief Sets the hypothesis
 */
//================================================================================

void AdaptiveAlgo::SetHypothesis( const StdMeshers_Adaptive1D* hyp )
{
  myHyp = hyp;
}

//================================================================================
/*!
 * \brief Creates segments on all given EDGEs
 */
//================================================================================

bool AdaptiveAlgo::Compute(SMESH_Mesh &         theMesh,
                           const TopoDS_Shape & theShape)
{
  // *theProgress = 0.01;

  if ( myHyp->GetMinSize() > myHyp->GetMaxSize() )
    return error( "Bad parameters: min size > max size" );

  myMesh = &theMesh;
  SMESH_MesherHelper helper( theMesh );
  const double grading = 0.7;

  TopTools_IndexedMapOfShape edgeMap, faceMap;
  TopExp::MapShapes( theShape,                 TopAbs_EDGE, edgeMap );
  TopExp::MapShapes( theMesh.GetShapeToMesh(), TopAbs_FACE, faceMap );

  // Triangulate the shape with the given deflection ?????????
  {
    BRepMesh_IncrementalMesh im( theMesh.GetShapeToMesh(), myHyp->GetDeflection(), /*isRelatif=*/0);
  }

  // get a bnd box
  Bnd_B3d box;
  {
    Bnd_Box aBox;
    BRepBndLib::Add( theMesh.GetShapeToMesh(), aBox);
    Standard_Real TXmin, TYmin, TZmin, TXmax, TYmax, TZmax;
    aBox.Get(TXmin, TYmin, TZmin, TXmax, TYmax, TZmax);
    box.Add( gp_XYZ( TXmin, TYmin, TZmin ));
    box.Add( gp_XYZ( TXmax, TYmax, TZmax ));
  }
  // *theProgress = 0.3;

  // holder of segment size at each point
  SegSizeTree sizeTree( box, grading, myHyp->GetMinSize(), myHyp->GetMaxSize() );
  mySizeTree = & sizeTree;

  // minimal segment size that sizeTree can store with reasonable tree height
  const double minSize = Max( myHyp->GetMinSize(), 1.1 * sizeTree.GetMinSize() );


  // fill myEdges - working data of EDGEs
  {
    // sort EDGEs by length
    multimap< double, TopoDS_Edge > edgeOfLength;
    for ( int iE = 1; iE <= edgeMap.Extent(); ++iE )
    {
      const TopoDS_Edge & edge = TopoDS::Edge( edgeMap( iE ));
      if ( !SMESH_Algo::isDegenerated( edge) )
        edgeOfLength.insert( make_pair( EdgeLength( edge ), edge ));
    }
    myEdges.clear();
    myEdges.resize( edgeOfLength.size() );
    multimap< double, TopoDS_Edge >::const_iterator len2edge = edgeOfLength.begin();
    for ( int iE = 0; len2edge != edgeOfLength.end(); ++len2edge, ++iE )
    {
      const TopoDS_Edge & edge = len2edge->second;
      EdgeData& eData = myEdges[ iE ];
      eData.myC3d.Initialize( edge );
      eData.myLength  = EdgeLength( edge );
      eData.AddPoint( eData.myPoints.end(), eData.myC3d.FirstParameter() );
      eData.AddPoint( eData.myPoints.end(), eData.myC3d.LastParameter() );
    }
  }
  if ( myEdges.empty() ) return true;
  if ( _computeCanceled ) return false;

  // Take into account size of already existing segments
  SMDS_EdgeIteratorPtr segIterator = theMesh.GetMeshDS()->edgesIterator();
  while ( segIterator->more() )
  {
    const SMDS_MeshElement* seg = segIterator->next();
    sizeTree.SetSize( SMESH_TNodeXYZ( seg->GetNode( 0 )), SMESH_TNodeXYZ( seg->GetNode( 1 )));
  }
  if ( _computeCanceled ) return false;

  // Set size of segments according to the deflection

  StdMeshers_Regular_1D::_hypType = DEFLECTION;
  StdMeshers_Regular_1D::_value[ DEFLECTION_IND ] = myHyp->GetDeflection();

  list< double > params;
  for ( int iE = 0; iE < myEdges.size(); ++iE )
  {
    EdgeData& eData = myEdges[ iE ];
    //cout << "E " << theMesh.GetMeshDS()->ShapeToIndex( eData.Edge() ) << endl;

    double f = eData.First().myU, l = eData.Last().myU;
    if ( !computeInternalParameters( theMesh, eData.myC3d, eData.myLength, f,l, params, false, false ))
      continue;
    if ( params.size() <= 1 && helper.IsClosedEdge( eData.Edge() ) ) // 2 segments on a circle
    {
      params.clear();
      for ( int i = 1; i < 6; ++i )
        params.push_back(( l - f ) * i/6. );
    }
    EdgeData::TPntIter where = --eData.myPoints.end();
    list< double >::const_iterator param = params.begin();
    for ( ; param != params.end(); ++param )
      eData.AddPoint( where, *param );

    EdgeData::TPntIter pIt2 = eData.myPoints.begin(), pIt1 = pIt2++;
    for ( ; pIt2 != eData.myPoints.end(); ++pIt1, ++pIt2 )
    {
      double sz = sizeTree.SetSize( (*pIt1).myP, (*pIt2).myP );
      sz = Min( sz, myHyp->GetMaxSize() );
      pIt1->mySegSize = Min( sz, pIt1->mySegSize );
      pIt2->mySegSize = Min( sz, pIt2->mySegSize );
    }

    if ( _computeCanceled ) return false;
  }

  // Limit size of segments according to distance to closest FACE

  for ( int iF = 1; iF <= faceMap.Extent(); ++iF )
  {
    if ( _computeCanceled ) return false;

    const TopoDS_Face & face = TopoDS::Face( faceMap( iF ));
    // cout << "FACE " << iF << "/" << faceMap.Extent()
    //      << " id-" << theMesh.GetMeshDS()->ShapeToIndex( face ) << endl;

    ElementBndBoxTree triaTree( face ); // tree of FACE triangulation
    TriaTreeData*     triaSearcher = triaTree.GetTriaData();

    triaSearcher->SetSizeByTrias( sizeTree, myHyp->GetDeflection() );

    for ( int iE = 0; iE < myEdges.size(); ++iE )
    {
      EdgeData& eData = myEdges[ iE ];

      // check if the face is in topological contact with the edge
      bool isAdjFace = ( helper.IsSubShape( helper.IthVertex( 0, eData.Edge()), face ) ||
                         helper.IsSubShape( helper.IthVertex( 1, eData.Edge()), face ));

      if ( isAdjFace && triaSearcher->mySurface.GetType() == GeomAbs_Plane )
        continue;

      bool sizeDecreased = true;
      for (int iLoop = 0; sizeDecreased; ++iLoop ) //repeat until segment size along the edge becomes stable
      {
        double maxSegSize = 0;

        // get points to check distance to the face
        EdgeData::TPntIter pIt2 = eData.myPoints.begin(), pIt1 = pIt2++, pItLast;
        maxSegSize = pIt1->mySegSize = Min( pIt1->mySegSize, sizeTree.GetSize( pIt1->myP ));
        for ( ; pIt2 != eData.myPoints.end(); )
        {
          pIt2->mySegSize = Min( pIt2->mySegSize, sizeTree.GetSize( pIt2->myP ));
          double curSize  = Min( pIt1->mySegSize, pIt2->mySegSize );
          maxSegSize      = Max( pIt2->mySegSize, maxSegSize );
          if ( pIt1->myP.Distance( pIt2->myP ) > curSize )
          {
            double midU  = 0.5*( pIt1->myU + pIt2->myU );
            gp_Pnt midP  = eData.myC3d.Value( midU );
            double midSz = sizeTree.GetSize( midP );
            pIt2 = eData.myPoints.insert( pIt2, EdgeData::ProbePnt( midP, midU, midSz ));
            eData.myBBox.Add( midP.XYZ() );
          }
          else
          {
            ++pIt1, ++pIt2;
          }
        }
        // check if the face is more distant than a half of the current segment size,
        // if not, segment size is decreased

        if ( iLoop == 0 && eData.IsTooDistant( triaSearcher->myBBox, maxSegSize ))
          break;
        triaSearcher->PrepareToTriaSearch();

        //cout << "E " << theMesh.GetMeshDS()->ShapeToIndex( eData.Edge() ) << endl;
        sizeDecreased = false;
        const gp_Pnt* avoidPnt = & eData.First().myP;
        pItLast = --eData.myPoints.end();
        for ( pIt1 = eData.myPoints.begin(); pIt1 != eData.myPoints.end();  )
        {
          double distToFace =
            triaSearcher->GetMinDistInSphere( pIt1->myP, pIt1->mySegSize, isAdjFace, avoidPnt );
          double allowedSize = Max( minSize, distToFace*( 1. + grading ));
          if ( allowedSize < pIt1->mySegSize  )
          {
            // cout << "E " << theMesh.GetMeshDS()->ShapeToIndex( eData.Edge() )
            //      << "\t closure detected " << endl;
            if ( 1.1 * allowedSize < pIt1->mySegSize  )
            {
              sizeDecreased = true;
              sizeTree.SetSize( pIt1->myP, allowedSize );
              // cout << "E " << theMesh.GetMeshDS()->ShapeToIndex( eData.Edge() )
              //      << "\t SetSize " << allowedSize << " at "
              //      << pIt1->myP.X() <<", "<< pIt1->myP.Y()<<", "<<pIt1->myP.Z() << endl;
              pIt2 = pIt1;
              if ( --pIt2 != eData.myPoints.end() && pIt2->mySegSize > allowedSize )
                sizeTree.SetSize( eData.myC3d.Value( 0.6*pIt2->myU + 0.4*pIt1->myU ), allowedSize );
              pIt2 = pIt1;
              if ( ++pIt2 != eData.myPoints.end() && pIt2->mySegSize > allowedSize )
                sizeTree.SetSize( eData.myC3d.Value( 0.6*pIt2->myU + 0.4*pIt1->myU ), allowedSize );
            }
            pIt1->mySegSize = allowedSize;
          }
          ++pIt1;
          if ( pIt1 == pItLast )
            avoidPnt = & eData.Last().myP;
          else
            avoidPnt = NULL;

          if ( iLoop > 20 )
          {
#ifdef _DEBUG_
            cout << "Infinite loop in AdaptiveAlgo::Compute()" << endl;
#endif
            sizeDecreased = false;
            break;
          }
        }
      } // while ( sizeDecreased )
    } // loop on myEdges

    // *theProgress = 0.3 + 0.3 * iF / double( faceMap.Extent() );

  } // loop on faceMap

  return makeSegments();
}

//================================================================================
/*!
 * \brief Create segments
 */
//================================================================================

bool AdaptiveAlgo::makeSegments()
{
  SMESH_HypoFilter quadHyp( SMESH_HypoFilter::HasName( "QuadraticMesh" ));
  _quadraticMesh = myMesh->GetHypothesis( myEdges[0].Edge(), quadHyp, /*andAncestors=*/true );

  SMESH_MesherHelper helper( *myMesh );
  helper.SetIsQuadratic( _quadraticMesh );

  vector< double > nbSegs, params;

  for ( int iE = 0; iE < myEdges.size(); ++iE )
  {
    EdgeData& eData = myEdges[ iE ];

    // estimate roughly min segment size on the EDGE
    double edgeMinSize = myHyp->GetMaxSize();
    EdgeData::TPntIter pIt1 = eData.myPoints.begin();
    for ( ; pIt1 != eData.myPoints.end(); ++pIt1 )
      edgeMinSize = Min( edgeMinSize,
                         Min( pIt1->mySegSize, mySizeTree->GetSize( pIt1->myP )));

    const double f = eData.myC3d.FirstParameter(), l = eData.myC3d.LastParameter();
    const double parLen = l - f;
    const int  nbDivSeg = 5;
    int           nbDiv = Max( 1, int ( eData.myLength / edgeMinSize * nbDivSeg ));

    // compute nb of segments
    bool toRecompute = true;
    double maxSegSize = 0;
    size_t i = 1, segCount;
    //cout << "E " << theMesh.GetMeshDS()->ShapeToIndex( eData.Edge() ) << endl;
    while ( toRecompute ) // recompute if segment size at some point is less than edgeMinSize/nbDivSeg
    {
      nbSegs.resize( nbDiv + 1 );
      nbSegs[0] = 0;
      toRecompute = false;

      // fill nbSegs with segment size stored in EdgeData::ProbePnt::mySegSize which can
      // be less than size in mySizeTree
      pIt1 = eData.myPoints.begin();
      EdgeData::ProbePnt* pp1 = &(*pIt1), *pp2;
      for ( ++pIt1; pIt1 != eData.myPoints.end(); ++pIt1 )
      {
        pp2 = &(*pIt1);
        double size1 = Min( pp1->mySegSize, myHyp->GetMaxSize() );
        double size2 = Min( pp2->mySegSize, myHyp->GetMaxSize() );
        double r, u, du = pp2->myU - pp1->myU;
        while(( u = f + parLen * i / nbDiv ) < pp2->myU )
        {
          r = ( u - pp1->myU ) / du;
          nbSegs[i] = (1-r) * size1 + r * size2;
          ++i;
        }
        if ( i < nbSegs.size() )
          nbSegs[i] = size2;
        pp1 = pp2;
      }
      // fill nbSegs with local nb of segments
      gp_Pnt p1 = eData.First().myP, p2, pDiv = p1;
      for ( i = 1, segCount = 1; i < nbSegs.size(); ++i )
      {
        p2 = eData.myC3d.Value( f + parLen * i / nbDiv );
        double locSize = Min( mySizeTree->GetSize( p2 ), nbSegs[i] );
        double nb      = p1.Distance( p2 ) / locSize;
        // if ( nbSegs.size() < 30 )
        //   cout << "locSize " << locSize << " nb " << nb << endl;
        if ( nb > 1. )
        {
          toRecompute = true;
          edgeMinSize = locSize;
          nbDiv = int ( eData.myLength / edgeMinSize * nbDivSeg );
          break;
        }
        nbSegs[i] = nbSegs[i-1] + nb;
        p1 = p2;
        if ( nbSegs[i] >= segCount )
        {
          maxSegSize = Max( maxSegSize, pDiv.Distance( p2 ));
          pDiv = p2;
          ++segCount;
        }
      }
    }

    // compute parameters of nodes
    int nbSegFinal = Max( 1, int(floor( nbSegs.back() + 0.5 )));
    double fact = nbSegFinal / nbSegs.back();
    if ( maxSegSize / fact > myHyp->GetMaxSize() )
      fact = ++nbSegFinal / nbSegs.back();
    //cout << "nbSegs.back() " << nbSegs.back() << " nbSegFinal " << nbSegFinal << endl;
    params.clear();
    for ( i = 0, segCount = 1; segCount < nbSegFinal; ++segCount )
    {
      while ( nbSegs[i] * fact < segCount )
        ++i;
      if ( i < nbDiv )
      {
        double d = i - ( nbSegs[i] - segCount/fact ) / ( nbSegs[i] - nbSegs[i-1] );
        params.push_back( f + parLen * d / nbDiv );
        //params.push_back( f + parLen * i / nbDiv );
      }
      else
        break;
    }
    // get nodes on VERTEXes
    TopoDS_Vertex vf = helper.IthVertex( 0, eData.Edge(), false );
    TopoDS_Vertex vl = helper.IthVertex( 1, eData.Edge(), false );
    myMesh->GetSubMesh( vf )->ComputeStateEngine( SMESH_subMesh::COMPUTE );
    myMesh->GetSubMesh( vl )->ComputeStateEngine( SMESH_subMesh::COMPUTE );
    const SMDS_MeshNode * nf = VertexNode( vf, myMesh->GetMeshDS() );
    const SMDS_MeshNode * nl = VertexNode( vl, myMesh->GetMeshDS() );
    if ( !nf || !nl )
      return error("No node on vertex");

    // create segments
    helper.SetSubShape( eData.Edge() );
    helper.SetElementsOnShape( true );
    const int ID = 0;
    const SMDS_MeshNode *n1 = nf, *n2;
    for ( i = 0; i < params.size(); ++i, n1 = n2 )
    {
      gp_Pnt p2 = eData.myC3d.Value( params[i] );
      n2 = helper.AddNode( p2.X(), p2.Y(), p2.Z(), ID, params[i] );
      helper.AddEdge( n1, n2, ID, /*force3d=*/false );
    }
    helper.AddEdge( n1, nl, ID, /*force3d=*/false );

    eData.myPoints.clear();

    //*theProgress = 0.6 + 0.4 * iE / double( myEdges.size() );
    if ( _computeCanceled )
      return false;

  } // loop on EDGEs

  SMESHUtils::FreeVector( myEdges );

  return true;
}

//================================================================================
/*!
 * \brief Predict number of segments on all given EDGEs
 */
//================================================================================

bool AdaptiveAlgo::Evaluate(SMESH_Mesh &         theMesh,
                            const TopoDS_Shape & theShape,
                            MapShapeNbElems&     theResMap)
{
  // initialize fields of inherited StdMeshers_Regular_1D
  StdMeshers_Regular_1D::_hypType = DEFLECTION;
  StdMeshers_Regular_1D::_value[ DEFLECTION_IND ] = myHyp->GetDeflection();

  TopExp_Explorer edExp( theShape, TopAbs_EDGE );

  for ( ; edExp.More(); edExp.Next() )
  {
    const TopoDS_Edge & edge = TopoDS::Edge( edExp.Current() );
    StdMeshers_Regular_1D::Evaluate( theMesh, theShape, theResMap );
  }
  return true;
}
    
