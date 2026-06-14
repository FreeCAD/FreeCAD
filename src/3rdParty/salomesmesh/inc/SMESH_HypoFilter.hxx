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
//  File   : SMESH_HypoFilter.hxx
//  Module : SMESH
//
#ifndef SMESH_HypoFilter_HeaderFile
#define SMESH_HypoFilter_HeaderFile

#include "SMESH_SMESH.hxx"

// ===========================
// Filter of SMESH_Hypothesis
// ===========================

#include <list>
#include <string>
#include <TopoDS_Shape.hxx>
#include <TopTools_MapOfShape.hxx>

class SMESH_HypoFilter;
class SMESH_Hypothesis;
class SMESH_Mesh;

class SMESH_EXPORT SMESH_HypoPredicate {
 public:
  virtual bool IsOk(const SMESH_Hypothesis* aHyp,
                    const TopoDS_Shape&     aShape) const = 0;
  // check aHyp or/and aShape it is assigned to
  virtual ~SMESH_HypoPredicate() {}
 private:
  int _logical_op;
  friend class SMESH_HypoFilter;
};

class SMESH_EXPORT SMESH_HypoFilter: public SMESH_HypoPredicate
{
 public:
  // Create and add predicates.
  // Added predicates will be destroyed by filter when it dies
  SMESH_HypoFilter();
  explicit SMESH_HypoFilter( SMESH_HypoPredicate* aPredicate, bool notNegate = true );
  // notNegate==false means !aPredicate->IsOk()
  SMESH_HypoFilter & Init  ( SMESH_HypoPredicate* aPredicate, bool notNegate = true );
  SMESH_HypoFilter & And   ( SMESH_HypoPredicate* aPredicate );
  SMESH_HypoFilter & AndNot( SMESH_HypoPredicate* aPredicate );
  SMESH_HypoFilter & Or    ( SMESH_HypoPredicate* aPredicate );
  SMESH_HypoFilter & OrNot ( SMESH_HypoPredicate* aPredicate );

  // Create predicates
  static SMESH_HypoPredicate* IsAlgo();
  static SMESH_HypoPredicate* IsAuxiliary();
  static SMESH_HypoPredicate* IsApplicableTo(const TopoDS_Shape& theShape);
  static SMESH_HypoPredicate* IsAssignedTo(const TopoDS_Shape& theShape);
  static SMESH_HypoPredicate* Is(const SMESH_Hypothesis* theHypo);
  static SMESH_HypoPredicate* IsGlobal(const TopoDS_Shape& theMainShape);
  static SMESH_HypoPredicate* IsMoreLocalThan(const TopoDS_Shape& theShape,
                                              const SMESH_Mesh&   theMesh);
  static SMESH_HypoPredicate* HasName(const std::string & theName);
  static SMESH_HypoPredicate* HasDim(const int theDim);
  static SMESH_HypoPredicate* HasType(const int theHypType);

  bool IsEmpty() const { return myNbPredicates == 0; }

  /*!
   * \brief check aHyp or/and aShape it is assigned to
   */
  bool IsOk (const SMESH_Hypothesis* aHyp,
             const TopoDS_Shape&     aShape) const;
  /*!
   * \brief return true if contains no predicates
   */
  bool IsAny() const { return myNbPredicates > 0; }

  ~SMESH_HypoFilter();


 protected:
  // fields

  //std::list<SMESH_HypoPredicate*> myPredicates;
  SMESH_HypoPredicate* myPredicates[100];
  int                  myNbPredicates;

  // private methods

  enum Logical { AND, AND_NOT, OR, OR_NOT };
  enum Comparison { EQUAL, NOT_EQUAL, MORE, LESS };

  SMESH_HypoFilter(const SMESH_HypoFilter& other){}

  void add( Logical bool_op, SMESH_HypoPredicate* pred )
  {
    if ( pred ) {
      pred->_logical_op = bool_op;
      myPredicates[ myNbPredicates++ ] = pred;
    }
  }

  // predicates implementation

  template <typename TValue>
    struct templPredicate: public SMESH_HypoPredicate {
      Comparison _comp;
      TValue     _val;
      virtual TValue Value(const SMESH_Hypothesis* aHyp) const = 0;
      virtual bool IsOk(const SMESH_Hypothesis* aHyp, const TopoDS_Shape& ) const
      {
        if      ( _comp == EQUAL )     return _val == Value( aHyp );
        else if ( _comp == NOT_EQUAL ) return _val != Value( aHyp );
        else if ( _comp == MORE )      return _val < Value( aHyp );
        else                           return _val > Value( aHyp );
      }
    };

  struct NamePredicate : public SMESH_HypoPredicate {
    std::string _name;
    NamePredicate( std::string name ): _name(name){}
    bool IsOk(const SMESH_Hypothesis* aHyp,
              const TopoDS_Shape&     aShape) const;
  };
  
  struct TypePredicate : public templPredicate< int > {
    TypePredicate( Comparison comp, int hypType )
    { _comp = comp; _val = hypType; }
    int Value( const SMESH_Hypothesis* aHyp ) const;
  };
  
  struct DimPredicate : public templPredicate< int > {
    DimPredicate( Comparison comp, int dim )
    { _comp = comp; _val = dim; }
    int Value( const SMESH_Hypothesis* aHyp ) const;
  };
  
  struct ApplicablePredicate : public SMESH_HypoPredicate {
    int _shapeType;
    ApplicablePredicate( const TopoDS_Shape& theShape );
    bool IsOk(const SMESH_Hypothesis* aHyp,
              const TopoDS_Shape&     aShape) const;
  };
        
  struct InstancePredicate : public SMESH_HypoPredicate {
    const SMESH_Hypothesis* _hypo;
    InstancePredicate( const SMESH_Hypothesis* hypo ):_hypo(hypo){}
    bool IsOk(const SMESH_Hypothesis* aHyp,
              const TopoDS_Shape&     aShape) const;
  };
        
  struct IsAssignedToPredicate : public SMESH_HypoPredicate {
    TopoDS_Shape _mainShape;
    IsAssignedToPredicate( const TopoDS_Shape& mainShape ):_mainShape(mainShape){}
    bool IsOk(const SMESH_Hypothesis* aHyp,
              const TopoDS_Shape&     aShape) const;
  };
        
  struct IsMoreLocalThanPredicate : public SMESH_HypoPredicate {
    TopoDS_Shape        _shape;
    const SMESH_Mesh&   _mesh;
    TopTools_MapOfShape _preferableShapes;
    IsMoreLocalThanPredicate( const TopoDS_Shape& shape,
                              const SMESH_Mesh&   mesh )
      :_shape(shape),_mesh(mesh) { findPreferable(); }
    bool IsOk(const SMESH_Hypothesis* aHyp,
              const TopoDS_Shape&     aShape) const;
    void findPreferable();
  };
        
  struct IsAuxiliaryPredicate : public SMESH_HypoPredicate {
    bool IsOk(const SMESH_Hypothesis* aHyp,
              const TopoDS_Shape&     aShape) const;
  };
        
};


#endif
