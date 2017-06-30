#ifndef HEP_EULERANGLES_H
#define HEP_EULERANGLES_H

// ----------------------------------------------------------------------
//
//  EulerAngles.h 	EulerAngles class --
//  			Support class for PhysicsVectors classes
//
// History:
//   09-Jan-1998  WEB  FixedTypes is now found in ZMutility
//   12-Jan-1998  WEB  PI is now found in ZMutility
//   15-Jun-1998  WEB  Added namespace support
//   02-May-2000  WEB  No global using
//   26-Jul-2000  MF   CLHEP version
//
// ----------------------------------------------------------------------

#include <iostream>
///#include "CLHEP/Vector/defs.h" 
#include "../vector/defs.h" 

namespace CLHEP {

// Declarations of classes and global methods
class HepEulerAngles;
std::ostream & operator<<(std::ostream & os, const HepEulerAngles & aa);
std::istream & operator>>(std::istream & is,       HepEulerAngles & aa);

/**
 * @author
 * @ingroup vector
 */
class HepEulerAngles {

protected:
  typedef HepEulerAngles EA;       // just an abbreviation
  static double tolerance;      // to determine relative nearness

public:

  // ----------  Constructors:
  inline HepEulerAngles();
  inline HepEulerAngles( double phi, double theta, double psi );

  // ----------  Destructor, copy constructor, assignment:
  // use C++ defaults

  // ----------  Accessors:

public:
  inline  double  getPhi() const;
  inline  double  phi()    const;
  inline  EA &       setPhi( double phi );

  inline  double  getTheta() const;
  inline  double  theta()    const;
  inline  EA &       setTheta( double theta );

  inline  double  getPsi() const;
  inline  double  psi()    const;
  inline  EA &       setPsi( double psi );

  inline EA & set( double phi, double theta, double psi );

  // ----------  Operations:

  //   comparisons:
  inline int  compare   ( const EA & ea ) const;

  inline bool operator==( const EA & ea ) const;
  inline bool operator!=( const EA & ea ) const;
  inline bool operator< ( const EA & ea ) const;
  inline bool operator<=( const EA & ea ) const;
  inline bool operator> ( const EA & ea ) const;
  inline bool operator>=( const EA & ea ) const;

  //   relative comparison:
  inline static double getTolerance();
  inline static double setTolerance( double tol );

  bool isNear ( const EA & ea, double epsilon = tolerance ) const;
  double  howNear( const EA & ea ) const;

  // ----------  I/O:

  friend std::ostream & operator<<( std::ostream & os, const EA & ea );
  friend std::istream & operator>>( std::istream & is,       EA & ea );

  // ---------- Helper methods:

protected:
    double distance( const HepEulerAngles & ex ) const;

  // ----------  Data members:
protected:
  double phi_;
  double theta_;
  double psi_;

};  // HepEulerAngles

}  // namespace CLHEP


namespace zmpv {

typedef CLHEP::HepEulerAngles EulerAngles;

}  // end of namespace zmpv

#define EULERANGLES_ICC
///#include "CLHEP/Vector/EulerAngles.icc"
#include "../vector/EulerAngles.icc"
#undef EULERANGLES_ICC

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
using namespace CLHEP;
#endif


#endif // EULERANGLES_H
