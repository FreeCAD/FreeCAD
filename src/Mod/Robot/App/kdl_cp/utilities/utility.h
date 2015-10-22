/***************************************************************************** 
 *  \author 
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version 
 *		ORO_Geometry V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: utility.h,v 1.1.1.1.2.4 2003/07/18 14:58:36 psoetens Exp $
 *		$Name:  $ 
 *  \file
 *    Included by most lrl-files to provide some general
 *    functions and macro definitions.
 *  
 *  \par history
 *   - changed layout of the comments to accomodate doxygen
 */


#ifndef KDL_UTILITY_H
#define KDL_UTILITY_H

#include "kdl-config.h"
#include <cstdlib>
#include <cassert>
#include <cmath>


/////////////////////////////////////////////////////////////
// configurable options for the frames library.

#ifdef KDL_INLINE
    #ifdef _MSC_VER
        // Microsoft Visual C
        #define IMETHOD __forceinline
    #else
        // Some other compiler, e.g. gcc
        #define IMETHOD inline
    #endif
#else
    #define IMETHOD
#endif



//! turn on or off frames bounds checking. If turned on, assert() can still
//! be turned off with -DNDEBUG.
#ifdef KDL_INDEX_CHECK
    #define FRAMES_CHECKI(a) assert(a)
#else
    #define FRAMES_CHECKI(a)
#endif


namespace KDL {

#ifdef __GNUC__
    // so that sin,cos can be overloaded and complete 
    // resolution of overloaded functions work.
    using ::sin;
    using ::cos;
    using ::exp;
    using ::log;
    using ::sin;
    using ::cos;
    using ::tan;
    using ::sinh;
    using ::cosh;
    using ::pow;
    using ::sqrt;
    using ::atan;
    using ::hypot;
    using ::asin;
    using ::acos;
    using ::tanh;
    using ::atan2;
#endif
#ifndef __GNUC__
    //only real solution : get Rall1d and varia out of namespaces.
    #pragma warning (disable:4786)

    inline double sin(double a) {
        return ::sin(a);
    }
    
    inline double cos(double a) {
        return ::cos(a);
    }
    inline double exp(double a) {
        return ::exp(a);
    }
    inline double log(double a) {
        return ::log(a);
    }
    inline double tan(double a) {
        return ::tan(a);
    }
    inline double cosh(double a) {
        return ::cosh(a);
    }
    inline double sinh(double a) {
        return ::sinh(a);
    }
    inline double sqrt(double a) {
        return ::sqrt(a);
    }
    inline double atan(double a) {
        return ::atan(a);
    }
    inline double acos(double a) {
        return ::acos(a);
    }
    inline double asin(double a) {
        return ::asin(a);
    }
    inline double tanh(double a) {
        return ::tanh(a);
    }
    inline double pow(double a,double b) {
        return ::pow(a,b);
    }
    inline double atan2(double a,double b) {
        return ::atan2(a,b);
    }
#endif  





/** 
 * Auxiliary class for argument types (Trait-template class )
 * 
 * Is used to pass doubles by value, and arbitrary objects by const reference.
 * This is TWICE as fast (2 x less memory access) and avoids bugs in VC6++ concerning
 * the assignment of the result of intrinsic functions to const double&-typed variables,
 * and optimization on.
 */
template <class T>
class TI
{
    public:
        typedef const T& Arg; //!< Arg is used for passing the element to a function.
};

template <>
class TI<double> {
public:
    typedef double Arg;
};

template <>
class TI<int> {
public:
    typedef int Arg;
};





/** 
 * /note linkage
 * Something fishy about the difference between C++ and C
 * in C++ const values default to INTERNAL linkage, in C they default
 * to EXTERNAL linkage.  Here the constants should have EXTERNAL linkage
 * because they, for at least some of them, can be changed by the user.
 * If you want to explicitly declare internal linkage, use "static".
 */
//! 
extern int          STREAMBUFFERSIZE;

//! maximal length of a file name
extern int          MAXLENFILENAME;

//! the value of pi
extern const double PI;

//! the value pi/180
extern const double deg2rad;

//! the value 180/pi
extern const double rad2deg;

//! default precision while comparing with Equal(..,..) functions. Initialized at 0.0000001.
extern double     epsilon;

//! the number of derivatives used in the RN-... objects.
extern int          VSIZE;



#ifndef _MFC_VER
#undef max
inline double max(double a,double b) {
    if (b<a) 
        return a;
    else
        return b;
}

#undef min
inline double min(double a,double b) {
    if (b<a) 
        return b;
    else
        return a;
}
#endif


#ifdef _MSC_VER
    //#pragma inline_depth( 255 )
    //#pragma inline_recursion( on )
    #define INLINE __forceinline
    //#define INLINE inline
#else
    #define INLINE inline
#endif


inline double LinComb(double alfa,double a,
        double beta,double b ) {
            return alfa*a+beta*b;
}

inline void LinCombR(double alfa,double a,
        double beta,double b,double& result ) {
            result=alfa*a+beta*b;
         }

//! to uniformly set double, RNDouble,Vector,... objects to zero in template-classes
inline void SetToZero(double& arg) {
    arg=0;
}

//! to uniformly set double, RNDouble,Vector,... objects to the identity element in template-classes
inline void SetToIdentity(double& arg) {
    arg=1;
}

inline double sign(double arg) {
    return (arg<0)?(-1):(1);
}

inline double sqr(double arg) { return arg*arg;}
inline double Norm(double arg) {
    return fabs(  (double)arg );
}

#if defined __WIN32__ && !defined __GNUC__
inline double hypot(double y,double x) { return ::_hypot(y,x);}
inline double abs(double x) { return ::fabs(x);}
#endif

// compares whether 2 doubles are equal in an eps-interval.
// Does not check whether a or b represents numbers
// On VC6, if a/b is -INF, it returns false;
inline bool Equal(double a,double b,double eps=epsilon)
{
    double tmp=(a-b);
    return ((eps>tmp)&& (tmp>-eps) );
}

inline void random(double& a) {
	a = 1.98*rand()/(double)RAND_MAX -0.99;
}

inline void posrandom(double& a) {
	a = 0.001+0.99*rand()/(double)RAND_MAX;
}

inline double diff(double a,double b,double dt) {
	return (b-a)/dt;
}
//inline float diff(float a,float b,double dt) {
//return (b-a)/dt;
//}
inline double addDelta(double a,double da,double dt) {
	return a+da*dt;
}

//inline float addDelta(float a,float da,double dt) {
//	return a+da*dt;
//}


}



#endif
