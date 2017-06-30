#ifndef HEP_ZMXPV_H
#define HEP_ZMXPV_H

// ----------------------------------------------------------------------
//
//  ZMxpv.h    	ZMexception's ZMthrown by classes in the PhysicsVectors
//		package.  To avoid name clashes, these start with ZMxpv.
//
//  THIS FILE CONTAINS TWO VERSIONS OF THE NECESSARY CODE:
//
//  With no special defines, this file will produce code for pure CLHEP 
//  building -- no ZOOM Exceptions are involved.
//
//  To force a build using ZOOM Exceptions where the ZMthrow macros appear,
//  compile with ENABLE_ZOOM_EXCEPTIONS defined.
//
// ----------------------------------------------------------------------

//#undef  ENABLE_ZOOM_EXCEPTIONS 	// For CLHEP builds 
//#define ENABLE_ZOOM_EXCEPTIONS    	// For ZOOM  builds

// There should be some external way to control this.  We haven't found it yet.
// Right now, this must be changed by hand when going between CLHEP and ZOOM.
#undef  ENABLE_ZOOM_EXCEPTIONS 	

  // Member functions of the Vector classes are capable of ZMthrow-ing the
  // following ZMexception's:
  //
  //	ZMxPhysicsVectors	Severe	Parent exception of all ZMexceptions
  //					particular to classes in the package.
  //
  //    ZMxpvInfiniteVector     Error
  //                                    Mathematical operation will lead
  //                                    to infinity or NAN in a component
  //                                    of a result vector.
  //    ZMxpvZeroVector         Error
  //                                    A zero vector was used to specify
  //                                    a direction based on vector.unit().
  //    ZMxpvTachyonic          Error
  //                                    A relativistic kinematic function was
  //                                    taken, involving a vector representing
  //                                    a speed at or beyond that of light (=1).
  //	ZMxpvSpacelike		Error
  //					A spacelike 4-vector was used in a
  //					context where its restMass or gamma
  //					needs to be computed:  The result is
  //					formally imaginary (a zero result is
  //					supplied).
  //    ZMxpvInfinity 		Error
  //                                    Mathematical operation will lead
  //                                    to infinity as a Scalar result.
  //    ZMxpvNegativeMass	Error
  //                                    Kinematic operation, e.g. invariant
  //					mass, rendered meaningless by an input
  //					with negative time component.
  //	ZMxpvVectorInputFails	Error
  //					Input to a SpaceVector or Lorentz
  //					Vector failed due to bad format or EOF.
  //	ZMxpvParallelCols	Error
  //					Purportedly orthogonal col's supplied
  //					to form a Rotation are exactly
  //					parallel instead.
  //	ZMxpvImproperRotation	Error
  //					Orthogonal col's supplied form a
  //					refection (determinant -1) more
  //					nearly than rather than a rotation.
  //	ZMxpvImproperTransformation Error
  //                                    Orthogonalized rows supplied form a
  //                                    tachyonic boost, a reflection, or
  //                                    a combination of those flaws,
  //                                    more nearly than a proper Lorentz
  //                                    transformation.
  //	ZMxpvFixedAxis		Error
  //					Attempt to change a RotationX,
  //					RotationY, or RotationZ in such a way
  //					that the axis might no longer be X,
  //					Y, or Z respectively.
  //	ZMxpvIndexRange		Error
  //					When using the syntax of v(i) to get
  //					a vector component, i is out of range.
  //	ZMxpvNotOrthogonal	Warning
  //					Purportedly orthogonal col's supplied
  //					to form a Rotation or LT are not
  //					orthogonal within the tolerance.
  //	ZMxpvNotSymplectic	Warning
  //					A row supplied to form a Lorentz
  //					transformation has a value of restmass
  //					incorrect by more than the tolerance:
  //					It should be -1 for rows 1-3,
  //					+1 for row 4.
  //    ZMxpvAmbiguousAngle     Warning
  //                                    Method involves taking an angle against
  //                                    a reference vector of zero length, or
  //                                    phi in polar coordinates of a vector
  //                                    along the Z axis.
  //    ZMxpvNegativeR          Warning
  //                                    R of a supplied vector is negative.
  //                                    The mathematical operation done is
  //                                    still formally valid.
  //    ZMxpvUnusualTheta       Warning
  //                                    Theta supplied to construct or set
  //                                    a vector is outside the range [0,PI].
  //                                    The mathematical operation done is
  //                                    still formally valid.  But note that
  //                                    when sin(theta) < 0, phi becomes an
  //                                    angle against the -X axis.
  //______________________________________________________________________

#ifndef ENABLE_ZOOM_EXCEPTIONS 

//  This is the CLHEP version.  When compiled for CLHEP, the basic CLHEP 
//  Vector classes will not (at least for now) depend on ZOOM Exceptions.  
//  Though this header lists the various sorts of Exceptions that could be 
//  thrown, ZMthrow.h in the pure CLHEP context will make ZMthrowC  
//  do what CLHEP has always done:  whine to cerr about the problem 
//  and continue.
//  ZMthrowA will whine to cerr and throw an exception; by catching the
//  exception as a std::exception, the outside code can call e.what() to 
//  find the message string.
//
//	If CLHEP ever embraces the ZOOM Exceptions mechanism, we will simply
//	modify this file.

#include <string>
#include <exception>

#define ZMthrowA(A) do { std::cerr << A.name() << " thrown:\n" 	   \
             <<   A.what() << "\n" 					   \
	     << "at line " << __LINE__ << " in file " << __FILE__ << "\n"; \
  throw A;} while (0)

#define ZMthrowC(A) do { std::cerr << A.name() << ":\n" 		   \
             <<   A.what() << "\n" 					   \
	     << "at line " << __LINE__ << " in file " << __FILE__ << "\n"; \
  } while (0)

class CLHEP_vector_exception : public std::exception {
public:
    CLHEP_vector_exception ( const std::string & s ) throw();
    virtual const char* what() const throw();
    virtual const char* name() const throw() = 0; 
    virtual ~CLHEP_vector_exception() throw() {} 
  private:								
    std::string message;  
};

#define CLHEP_vector_exception_header(NAME) 				\
  class NAME : public CLHEP_vector_exception {				\
  public:								\
    NAME ( const std::string & s ) throw();				\
    virtual const char* name() const throw();  				\
    virtual ~NAME() throw() {}						\
  };


// The following exceptions might be encountered via ZMtrhowA

CLHEP_vector_exception_header( ZMxPhysicsVectors )
CLHEP_vector_exception_header( ZMxpvSpacelike )
CLHEP_vector_exception_header( ZMxpvNegativeMass )
CLHEP_vector_exception_header( ZMxpvVectorInputFails )
CLHEP_vector_exception_header( ZMxpvIndexRange )
CLHEP_vector_exception_header( ZMxpvFixedAxis )

// The following are sometimes ZMthrowA and sometimes ZMthrowC

CLHEP_vector_exception_header( ZMxpvTachyonic )
CLHEP_vector_exception_header( ZMxpvZeroVector )
CLHEP_vector_exception_header( ZMxpvImproperTransformation )
CLHEP_vector_exception_header( ZMxpvInfiniteVector )
CLHEP_vector_exception_header( ZMxpvInfinity )
CLHEP_vector_exception_header( ZMxpvImproperRotation )
CLHEP_vector_exception_header( ZMxpvAmbiguousAngle )

// THe following won't throw; they are encountered via ZMthrowC

CLHEP_vector_exception_header( ZMxpvNegativeR )
CLHEP_vector_exception_header( ZMxpvUnusualTheta )
CLHEP_vector_exception_header( ZMxpvParallelCols )
CLHEP_vector_exception_header( ZMxpvNotOrthogonal )
CLHEP_vector_exception_header( ZMxpvNotSymplectic )

#endif // endif for ifndef ENABLE_ZOOM_EXCEPTIONS 

// =============================================================
// =============================================================
// =============================================================

#ifdef ENABLE_ZOOM_EXCEPTIONS 

//  This is the ZOOM version.  When compiled for ZOOM, even the basic CLHEP 
//  Vector classes will depend on ZOOM Exceptions.  
//  Though in the CLHEP context methods use ZMthrowA and ZMthrowC, these
//  in the ZOOM context become ZMthrow.
//
//  Either this file or ZMxpvCLHEP.h is copied to become ZMxpv.h, depending 
//  on whether this is a ZOOM or a CLHEP build.  
//

#ifndef ZMEXCEPTIONS_H
  #include "Exceptions/ZMexception.h"
  #include "Exceptions/ZMthrow.h"
#endif
using namespace zmex;

namespace zmpv  {

ZMexStandardDefinition (ZMexception, ZMxPhysicsVectors);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvInfiniteVector);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvZeroVector);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvTachyonic);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvSpacelike);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvInfinity);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvNegativeMass);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvAmbiguousAngle);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvNegativeR);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvUnusualTheta);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvVectorInputFails);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvParallelCols);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvImproperRotation);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvImproperTransformation);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvIndexRange);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvNotOrthogonal);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvNotSymplectic);
ZMexStandardDefinition (ZMxPhysicsVectors, ZMxpvFixedAxis);

#define ZMthrowA(A) ZMthrow(A)
#define ZMthrowC(A) ZMthrow(A)

}  // namespace zmpv

#endif // ENABLE_ZOOM_EXCEPTIONS

#endif // HEP_ZMXPV_H
