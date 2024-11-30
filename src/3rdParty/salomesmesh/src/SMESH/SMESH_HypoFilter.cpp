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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : SMESH_HypoFilter.cxx
//  Module : SMESH
//
#include "SMESH_HypoFilter.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"

#include <TopExp_Explorer.hxx>

using namespace std;


//=======================================================================
//function : NamePredicate::Value
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::NamePredicate::IsOk (const SMESH_Hypothesis* aHyp,
                                            const TopoDS_Shape&     /*aShape*/ ) const
{
  return ( _name == aHyp->GetName() );
}

//=======================================================================
//function : TypePredicate::Value
//purpose  : 
//=======================================================================

int SMESH_HypoFilter::TypePredicate::Value( const SMESH_Hypothesis* aHyp ) const
{
  return aHyp->GetType();
};

//=======================================================================
//function : DimPredicate::Value
//purpose  : 
//=======================================================================

int SMESH_HypoFilter::DimPredicate::Value( const SMESH_Hypothesis* aHyp ) const
{
  return aHyp->GetDim();
}

//=======================================================================
//function : ApplicablePredicate::IsOk
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::ApplicablePredicate::IsOk(const SMESH_Hypothesis* aHyp,
                                                 const TopoDS_Shape&     /*aShape*/) const
{
  return SMESH_subMesh::IsApplicableHypotesis( aHyp, (TopAbs_ShapeEnum)_shapeType );
};

//=======================================================================
//function : IsAuxiliaryPredicate::IsOk
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::IsAuxiliaryPredicate::IsOk(const SMESH_Hypothesis* aHyp,
                                                  const TopoDS_Shape&     /*aShape*/) const
{
  return aHyp->IsAuxiliary();
};

//=======================================================================
//function : ApplicablePredicate::ApplicablePredicate
//purpose  : 
//=======================================================================

SMESH_HypoFilter::ApplicablePredicate::ApplicablePredicate( const TopoDS_Shape& theShape )
{
  _shapeType = ( theShape.IsNull() ? TopAbs_SHAPE : theShape.ShapeType());
}

//=======================================================================
//function : InstancePredicate::IsOk
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::InstancePredicate::IsOk(const SMESH_Hypothesis* aHyp,
                                               const TopoDS_Shape&     /*aShape*/) const
{
  return _hypo == aHyp;
}

//=======================================================================
//function : IsAssignedToPredicate::IsOk
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::IsAssignedToPredicate::IsOk(const SMESH_Hypothesis* aHyp,
                                                   const TopoDS_Shape&     aShape) const
{
  return ( !_mainShape.IsNull() && !aShape.IsNull() && _mainShape.IsSame( aShape ));
}

//================================================================================
/*!
 * \brief Finds shapes preferable over _shape due to sub-mesh order
 */
//================================================================================

void SMESH_HypoFilter::IsMoreLocalThanPredicate::findPreferable()
{
  const int shapeID = _mesh.GetMeshDS()->ShapeToIndex( _shape );
  const TListOfListOfInt& listOfShapeIDList = _mesh.GetMeshOrder();
  TListOfListOfInt::const_iterator listsIt = listOfShapeIDList.begin();
  for ( ; listsIt != listOfShapeIDList.end(); ++listsIt )
  {
    const TListOfInt& idList  = *listsIt;
    TListOfInt::const_iterator idIt =
      std::find( idList.begin(), idList.end(), shapeID );
    if ( idIt != idList.end() && *idIt != idList.front() )
    {
      for ( ; idIt != idList.end(); --idIt )
      {
        const TopoDS_Shape& shape = _mesh.GetMeshDS()->IndexToShape( *idIt );
        if ( !shape.IsNull())
          _preferableShapes.Add( shape );
      }
    }
  }
}

//=======================================================================
//function : IsMoreLocalThanPredicate::IsOk
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::IsMoreLocalThanPredicate::IsOk(const SMESH_Hypothesis* aHyp,
                                                      const TopoDS_Shape&     aShape) const
{
  if ( aShape.IsSame( _mesh.GetShapeToMesh() ) ||  // aHyp is global
       aShape.IsSame( _shape ))
    return false;

  if ( SMESH_MesherHelper::IsSubShape( aShape, /*mainShape=*/_shape ))
    return true;

  if ( aShape.ShapeType() == TopAbs_COMPOUND && 
       !SMESH_MesherHelper::IsSubShape( _shape, /*mainShape=*/aShape)) // issue 0020963
  {
    for ( int type = TopAbs_SOLID; type < TopAbs_SHAPE; ++type )
      if ( aHyp->GetDim() == SMESH_Gen::GetShapeDim( TopAbs_ShapeEnum( type )))
        for ( TopExp_Explorer exp( aShape, TopAbs_ShapeEnum( type )); exp.More(); exp.Next())
          if ( SMESH_MesherHelper::IsSubShape( exp.Current(), /*mainShape=*/_shape ))
            return true;
  }

  if ( _preferableShapes.Contains( aShape ))
    return true; // issue 21559, Mesh_6

  return false;
}

//=======================================================================
//function : SMESH_HypoFilter
//purpose  : 
//=======================================================================

SMESH_HypoFilter::SMESH_HypoFilter()
  : myNbPredicates(0)
{
}

//=======================================================================
//function : SMESH_HypoFilter
//purpose  : 
//=======================================================================

SMESH_HypoFilter::SMESH_HypoFilter( SMESH_HypoPredicate* aPredicate, bool notNegate )
  : myNbPredicates(0)
{
  add( notNegate ? AND : AND_NOT, aPredicate );
}

//=======================================================================
//function : And
//purpose  : 
//=======================================================================

SMESH_HypoFilter & SMESH_HypoFilter::And( SMESH_HypoPredicate* aPredicate )
{
  add( AND, aPredicate );
  return *this;
}

//=======================================================================
//function : AndNot
//purpose  : 
//=======================================================================

SMESH_HypoFilter & SMESH_HypoFilter::AndNot( SMESH_HypoPredicate* aPredicate )
{
  add( AND_NOT, aPredicate );
  return *this;
}

//=======================================================================
//function : Or
//purpose  : 
//=======================================================================

SMESH_HypoFilter & SMESH_HypoFilter::Or( SMESH_HypoPredicate* aPredicate )
{
  add( OR, aPredicate );
  return *this;
}

//=======================================================================
//function : OrNot
//purpose  : Return predicates
//=======================================================================

SMESH_HypoFilter & SMESH_HypoFilter::OrNot( SMESH_HypoPredicate* aPredicate )
{
  add( OR_NOT, aPredicate );
  return *this;
}

//=======================================================================
//function : Is
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::Is(const SMESH_Hypothesis* theHypo)
{
  return new InstancePredicate( theHypo );
}

//=======================================================================
//function : IsAlgo
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::IsAlgo()
{
  return new TypePredicate( MORE, SMESHDS_Hypothesis::PARAM_ALGO );
}

//=======================================================================
//function : IsAuxiliary
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::IsAuxiliary()
{
  return new IsAuxiliaryPredicate();
}


//=======================================================================
//function : IsGlobal
//purpose  : 
//=======================================================================

 SMESH_HypoPredicate* SMESH_HypoFilter::IsGlobal(const TopoDS_Shape& theMainShape)
{
  return new IsAssignedToPredicate( theMainShape );
}

//=======================================================================
//function : IsAssignedTo
//purpose  : 
//=======================================================================

 SMESH_HypoPredicate* SMESH_HypoFilter::IsAssignedTo(const TopoDS_Shape& theShape)
{
  return new IsAssignedToPredicate( theShape );
}

//=======================================================================
//function : HasName
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::HasName(const string & theName)
{
  return new NamePredicate( theName );
}

//=======================================================================
//function : HasDim
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::HasDim(const int theDim)
{
  return new DimPredicate( EQUAL, theDim );
}

//=======================================================================
//function : IsApplicableTo
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::IsApplicableTo(const TopoDS_Shape& theShape)
{
  return new ApplicablePredicate( theShape );
}

//=======================================================================
//function : IsMoreLocalThan
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::IsMoreLocalThan(const TopoDS_Shape& theShape,
                                                       const SMESH_Mesh&   theMesh)
{
  return new IsMoreLocalThanPredicate( theShape, theMesh );
}

//=======================================================================
//function : HasType
//purpose  : 
//=======================================================================

SMESH_HypoPredicate* SMESH_HypoFilter::HasType(const int theHypType)
{
  return new TypePredicate( EQUAL, theHypType );
}

//=======================================================================
//function : IsOk
//purpose  : 
//=======================================================================

bool SMESH_HypoFilter::IsOk (const SMESH_Hypothesis* aHyp,
                             const TopoDS_Shape&     aShape) const
{
  if ( IsEmpty() )
    return true;

  bool ok = ( myPredicates[0]->_logical_op <= AND_NOT );
  for ( int i = 0; i < myNbPredicates; ++i )
  {
    bool ok2 = myPredicates[i]->IsOk( aHyp, aShape );
    switch ( myPredicates[i]->_logical_op ) {
    case AND:     ok = ok && ok2; break;
    case AND_NOT: ok = ok && !ok2; break;
    case OR:      ok = ok || ok2; break;
    case OR_NOT:  ok = ok || !ok2; break;
    default:;
    }
  }
  return ok;
}

//=======================================================================
//function : Init
//purpose  : 
//=======================================================================

SMESH_HypoFilter & SMESH_HypoFilter::Init  ( SMESH_HypoPredicate* aPredicate, bool notNegate )
{
  SMESH_HypoPredicate** pred = &myPredicates[0];
  SMESH_HypoPredicate** end  = &myPredicates[myNbPredicates];
  for ( ; pred != end; ++pred )
    delete *pred;
  myNbPredicates = 0;

  add( notNegate ? AND : AND_NOT, aPredicate );
  return *this;
}


//=======================================================================
//function : IsOk
//purpose  : 
//=======================================================================

SMESH_HypoFilter::~SMESH_HypoFilter()
{
  SMESH_HypoPredicate** pred = &myPredicates[0];
  SMESH_HypoPredicate** end  = &myPredicates[myNbPredicates];
  for ( ; pred != end; ++pred )
    delete *pred;
  myNbPredicates = 0;
}



