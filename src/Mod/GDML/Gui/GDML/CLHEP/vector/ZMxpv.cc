// ----------------------------------------------------------------------
//
//  ZMxpv.cc   	Support for Vector package in the ZOOM context, where
//		probelmatic conditions are handled via ZOOM Exceptions.
//
//		In the CLHEP context (that is, unless ENABLE_ZOOM_EXCEPTIONS
//		is defined) this file is content-free.
//
//		ZMexception's are ZMthrown by classes in the PhysicsVectors
//		package.  (To avoid name clashes, these start with ZMxpv.)
//		Each possible such exception must be provided with some
//		defining properties:  thowe are in this file.
//
// History:
//   19-Nov-1997  MF    Initial version, to enable the ZMthrow exceptions
//			in SpaceVector.
//   15-Jun-1998  WEB	Added namespace support
//   08-Jan-2001  MF	Moved into CLHEP
//   09-Oct-2003  MF    Major addition:  Exception class defs for CLHEP case
//
// ----------------------------------------------------------------------


///#include "CLHEP/Vector/ZMxpv.h"
#include "ZMxpv.h"

#ifndef ENABLE_ZOOM_EXCEPTIONS

CLHEP_vector_exception::CLHEP_vector_exception 
		( const std::string & s ) throw() : message(s) {}		

const char* 
CLHEP_vector_exception::what() const throw() { 
  static std::string answer;
  answer  = name();
  answer += ": ";
  answer += message;  
  return answer.c_str(); 
}

#define CLHEP_vector_exception_methods(NAME)				    \
  NAME::NAME(const std::string & s) throw() : CLHEP_vector_exception(s) {}  \
  const char* NAME::name() const throw() {				    \
    return #NAME;							    \
  }

CLHEP_vector_exception_methods( ZMxPhysicsVectors )
CLHEP_vector_exception_methods( ZMxpvSpacelike )
CLHEP_vector_exception_methods( ZMxpvNegativeMass )
CLHEP_vector_exception_methods( ZMxpvVectorInputFails )
CLHEP_vector_exception_methods( ZMxpvIndexRange )
CLHEP_vector_exception_methods( ZMxpvFixedAxis )

CLHEP_vector_exception_methods( ZMxpvTachyonic )
CLHEP_vector_exception_methods( ZMxpvZeroVector )
CLHEP_vector_exception_methods( ZMxpvImproperTransformation )
CLHEP_vector_exception_methods( ZMxpvInfiniteVector )
CLHEP_vector_exception_methods( ZMxpvInfinity )
CLHEP_vector_exception_methods( ZMxpvImproperRotation )
CLHEP_vector_exception_methods( ZMxpvAmbiguousAngle )

CLHEP_vector_exception_methods( ZMxpvNegativeR )
CLHEP_vector_exception_methods( ZMxpvUnusualTheta )
CLHEP_vector_exception_methods( ZMxpvParallelCols )
CLHEP_vector_exception_methods( ZMxpvNotOrthogonal )
CLHEP_vector_exception_methods( ZMxpvNotSymplectic )

#endif // endif for ifndef ENABLE_ZOOM_EXCEPTIONS 

// ========================================================================
// ========================================================================
// ========================================================================

#ifdef ENABLE_ZOOM_EXCEPTIONS

ZM_BEGIN_NAMESPACE( zmpv )	/*  namespace zmpv  {  */


ZMexClassInfo ZMxPhysicsVectors::_classInfo (
	"ZMxPhysicsVectors",
	"PhysicsVectors",
	ZMexSEVERE );
// General Exception in a PhysicsVectors routine

ZMexClassInfo ZMxpvInfiniteVector::_classInfo (
	"InfiniteVector",
	"PhysicsVectors",
	ZMexERROR );
// Infinite vector component

ZMexClassInfo ZMxpvZeroVector::_classInfo (
	"ZeroVector",
	"PhysicsVectors",
	ZMexERROR );
// Zero Vector cannot be converted to Unit Vector

ZMexClassInfo ZMxpvTachyonic::_classInfo (
	"Tachyonic",
	"PhysicsVectors",
	ZMexERROR );
// Relativistic method using vector representing speed greater than light

ZMexClassInfo ZMxpvSpacelike::_classInfo (
	"Spacelike",
	"PhysicsVectors",
	ZMexERROR );
// Spacelike 4-vector used in context where rest mass or gamma needs computing

ZMexClassInfo ZMxpvInfinity::_classInfo (
	"Infinity",
	"PhysicsVectors",
	ZMexERROR );
// Mathematical operation will lead to infinity as a Scalar result

ZMexClassInfo ZMxpvNegativeMass::_classInfo (
	"NegativeMass",
	"PhysicsVectors",
	ZMexERROR );
// Kinematic operation was rendered meaningless by an input with negative t

ZMexClassInfo ZMxpvAmbiguousAngle::_classInfo (
	"AmbiguousAngle",
	"PhysicsVectors",
	ZMexWARNING );
// Angle requested ill-defined, due to null or collinear vectors

ZMexClassInfo ZMxpvNegativeR::_classInfo (
	"NegativeR",
	"PhysicsVectors",
	ZMexWARNING );
// Negative value supplied for vector magnitude

ZMexClassInfo ZMxpvUnusualTheta::_classInfo (
	"UnusualTheta",
	"PhysicsVectors",
	ZMexWARNING );
// Theta supplied for polar coordinates outside of [0,PI]

ZMexClassInfo ZMxpvVectorInputFails::_classInfo (
	"VectorInputFails",
	"PhysicsVectors",
	ZMexERROR );
// Theta supplied for polar coordinates outside of [0,PI]

ZMexClassInfo ZMxpvParallelCols::_classInfo (
	"ParallelCols",
	"PhysicsVectors",
	ZMexERROR );
// Col's supplied to form a Rotation are parallel instead of orthogonal

ZMexClassInfo ZMxpvImproperRotation::_classInfo (
	"ImproperRotation",
	"PhysicsVectors",
	ZMexERROR );
// Col's supplied to form a Rotation form a (rotation+reflection) instead

ZMexClassInfo ZMxpvImproperTransformation::_classInfo (
	"ImproperRotation",
	"PhysicsVectors",
	ZMexERROR );
// Rows supplied to form a LorentzTransformation form tachyonic or reflection

ZMexClassInfo ZMxpvNotOrthogonal::_classInfo (
	"NotOrthogonal",
	"PhysicsVectors",
	ZMexWARNING );
// Col's supplied to form a Rotation or LorentzTransformation are not orthogonal

ZMexClassInfo ZMxpvNotSymplectic::_classInfo (
	"NotSymplectic",
	"PhysicsVectors",
	ZMexWARNING );
// A row supplied as part of a LorentzTransformation has wrong restmass()


ZMexClassInfo ZMxpvFixedAxis::_classInfo (
	"FixedAxis",
	"PhysicsVectors",
	ZMexERROR );
// An attempt to change the axis is of a rotation fixed to be about X Y or Z.

ZMexClassInfo ZMxpvIndexRange::_classInfo (
	"IndexRange",
	"PhysicsVectors",
	ZMexERROR );
// An attempt to access a vector in the p(i) notation, where i is out of range.


ZM_END_NAMESPACE( zmpv )	/*  }  // namespace zmpv  */

#endif  // endif for ifdef ENAMBLE_ZOOM_EXCEPTIONS
