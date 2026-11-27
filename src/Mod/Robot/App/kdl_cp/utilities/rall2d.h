
/*****************************************************************************
 * \file  
 *      class for automatic differentiation on scalar values and 1st 
 *      derivatives and 2nd derivative.
 *       
 *  \author 
 *      Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version 
 *      ORO_Geometry V0.2
 *
 *  \par Note
 *      VC6++ contains a bug, concerning the use of inlined friend functions 
 *      in combination with namespaces.  So, try to avoid inlined friend 
 *      functions !  
 *
 *  \par History
 *      - $log$ 
 *
 *  \par Release
 *      $Id: rall2d.h,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $ 
 ****************************************************************************/

#ifndef Rall2D_H
#define Rall2D_H

#include <math.h>
#include <assert.h>
#include "utility.h"


namespace KDL {

/**
 * Rall2d contains a value, and its gradient and its 2nd derivative, and defines an algebraic 
 * structure on this pair.
 *  This template class has 3 template parameters :
 *  -   T contains the type of the value.
 *  -   V contains the type of the gradient (can be a vector-like type).  
 *  -   S defines a scalar type that can operate on Rall1d.  This is the type that 
 *      is used to give back values of Norm() etc. 
 *
 * S is useful when you recurse a Rall1d object into itself to create a 2nd, 3rd, 4th,.. 
 * derivatives. (e.g. Rall1d< Rall1d<double>, Rall1d<double>, double> ).
 *
 * S is always passed by value. 
 *
 * \par Class Type
 * Concrete implementation
 */
template <class T,class V=T,class S=T>
class Rall2d                      
    {
    public :
        T t;    //!< value
        V d;    //!< 1st derivative
        V dd;   //!< 2nd derivative
    public :
        // = Constructors
        INLINE Rall2d():t(),d(),dd() {};

        explicit INLINE  Rall2d(typename TI<T>::Arg c)
            {t=c;SetToZero(d);SetToZero(dd);}

        INLINE Rall2d(typename TI<T>::Arg tn,const V& afg):t(tn),d(afg) {SetToZero(dd);}

        INLINE Rall2d(typename TI<T>::Arg tn,const V& afg,const V& afg2):t(tn),d(afg),dd(afg2) {}

        // = Copy Constructor
        INLINE Rall2d(const Rall2d<T,V,S>& r):t(r.t),d(r.d),dd(r.dd) {}
            //if one defines this constructor, it's better optimized then the
            //automatically generated one ( that one set's up a loop to copy
            // word by word.
        
        // = Member functions to access internal structures :
        INLINE T& Value() {
            return t;
        }

        INLINE V& D() {
            return d;
        }

        INLINE V& DD() {
            return dd;
        }
        INLINE static Rall2d<T,V,S> Zero() {
            Rall2d<T,V,S> tmp;
            SetToZero(tmp);
            return tmp;
        }
        INLINE static Rall2d<T,V,S> Identity() {
            Rall2d<T,V,S> tmp;
            SetToIdentity(tmp);
            return tmp;
        }

        // = assignment operators
        INLINE Rall2d<T,V,S>& operator =(S c)
            {t=c;SetToZero(d);SetToZero(dd);return *this;}

        INLINE Rall2d<T,V,S>& operator =(const Rall2d<T,V,S>& r)
            {t=r.t;d=r.d;dd=r.dd;return *this;}

        INLINE Rall2d<T,V,S>& operator /=(const Rall2d<T,V,S>& rhs)
            {
            t /= rhs.t;
            d = (d-t*rhs.d)/rhs.t;
            dd= (dd - S(2)*d*rhs.d-t*rhs.dd)/rhs.t;
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator *=(const Rall2d<T,V,S>& rhs)
            {
            t *= rhs.t;
            d  = (d*rhs.t+t*rhs.d);
            dd = (dd*rhs.t+S(2)*d*rhs.d+t*rhs.dd);
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator +=(const Rall2d<T,V,S>& rhs)
            {
            t    +=rhs.t;
            d +=rhs.d;
            dd+=rhs.dd;
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator -=(const Rall2d<T,V,S>& rhs)
            {
            t     -= rhs.t;
            d     -= rhs.d;
            dd    -= rhs.dd;
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator /=(S rhs)
            {
            t    /= rhs;
            d    /= rhs;
            dd   /= rhs;
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator *=(S rhs)
            {
            t    *= rhs;
            d    *= rhs;
            dd   *= rhs;
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator -=(S rhs)
            {
            t    -= rhs;
            return *this;
            }

        INLINE Rall2d<T,V,S>& operator +=(S rhs)
            {
            t    += rhs;
            return *this;
            }

        // = Operators between Rall2d objects
/*
         friend INLINE Rall2d<T,V,S> operator /(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs);
         friend INLINE Rall2d<T,V,S> operator *(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs);
         friend INLINE Rall2d<T,V,S> operator +(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs);
         friend INLINE Rall2d<T,V,S> operator -(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs);
         friend INLINE Rall2d<T,V,S> operator -(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> operator *(S s,const Rall2d<T,V,S>& v);
         friend INLINE Rall2d<T,V,S> operator *(const Rall2d<T,V,S>& v,S s);
         friend INLINE Rall2d<T,V,S> operator +(S s,const Rall2d<T,V,S>& v);
         friend INLINE Rall2d<T,V,S> operator +(const Rall2d<T,V,S>& v,S s);
         friend INLINE Rall2d<T,V,S> operator -(S s,const Rall2d<T,V,S>& v);
         friend INLINE INLINE Rall2d<T,V,S> operator -(const Rall2d<T,V,S>& v,S s);
         friend INLINE Rall2d<T,V,S> operator /(S s,const Rall2d<T,V,S>& v);
         friend INLINE Rall2d<T,V,S> operator /(const Rall2d<T,V,S>& v,S s);

        // = Mathematical functions that operate on Rall2d objects

         friend INLINE Rall2d<T,V,S> exp(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> log(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> sin(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> cos(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> tan(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> sinh(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> cosh(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> tanh(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> sqr(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> pow(const Rall2d<T,V,S>& arg,double m) ;
         friend INLINE Rall2d<T,V,S> sqrt(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> asin(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> acos(const Rall2d<T,V,S>& arg);
         friend INLINE Rall2d<T,V,S> atan(const Rall2d<T,V,S>& x);
         friend INLINE Rall2d<T,V,S> atan2(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x);
         friend INLINE Rall2d<T,V,S> abs(const Rall2d<T,V,S>& x);
         friend INLINE Rall2d<T,V,S> hypot(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x);
        // returns sqrt(y*y+x*x), but is optimized for accuracy and speed.
         friend INLINE S Norm(const Rall2d<T,V,S>& value) ;
        // returns Norm( value.Value() ).

        // = Some utility functions to improve performance
        // (should also be declared on primitive types to improve uniformity
         friend INLINE Rall2d<T,V,S> LinComb(S alfa,const Rall2d<T,V,S>& a,
            TI<T>::Arg beta,const Rall2d<T,V,S>& b );
         friend INLINE void LinCombR(S alfa,const Rall2d<T,V,S>& a,
            TI<T>::Arg beta,const Rall2d<T,V,S>& b,Rall2d<T,V,S>& result );
        // = Setting value of a Rall2d object to 0 or 1
         friend INLINE void SetToZero(Rall2d<T,V,S>& value);
         friend INLINE void SetToOne(Rall2d<T,V,S>& value);
        // = Equality in an eps-interval
         friend INLINE bool Equal(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x,double eps);
         */
    };





// = Operators between Rall2d objects
template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator /(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs)
    {
    Rall2d<T,V,S> tmp;
    tmp.t = lhs.t/rhs.t;
    tmp.d = (lhs.d-tmp.t*rhs.d)/rhs.t;
    tmp.dd= (lhs.dd-S(2)*tmp.d*rhs.d-tmp.t*rhs.dd)/rhs.t;
    return tmp;
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator *(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs)
    {
    Rall2d<T,V,S> tmp;
    tmp.t  = lhs.t*rhs.t;
    tmp.d  = (lhs.d*rhs.t+lhs.t*rhs.d);
    tmp.dd = (lhs.dd*rhs.t+S(2)*lhs.d*rhs.d+lhs.t*rhs.dd);
    return tmp;
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator +(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs)
    {
    return Rall2d<T,V,S>(lhs.t+rhs.t,lhs.d+rhs.d,lhs.dd+rhs.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator -(const Rall2d<T,V,S>& lhs,const Rall2d<T,V,S>& rhs)
    {
    return Rall2d<T,V,S>(lhs.t-rhs.t,lhs.d-rhs.d,lhs.dd-rhs.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator -(const Rall2d<T,V,S>& arg)
    {
    return Rall2d<T,V,S>(-arg.t,-arg.d,-arg.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator *(S s,const Rall2d<T,V,S>& v)
    {
    return Rall2d<T,V,S>(s*v.t,s*v.d,s*v.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator *(const Rall2d<T,V,S>& v,S s)
    {
    return Rall2d<T,V,S>(v.t*s,v.d*s,v.dd*s);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator +(S s,const Rall2d<T,V,S>& v)
    {
    return Rall2d<T,V,S>(s+v.t,v.d,v.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator +(const Rall2d<T,V,S>& v,S s)
    {
    return Rall2d<T,V,S>(v.t+s,v.d,v.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator -(S s,const Rall2d<T,V,S>& v)
    {
    return Rall2d<T,V,S>(s-v.t,-v.d,-v.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator -(const Rall2d<T,V,S>& v,S s)
    {
    return Rall2d<T,V,S>(v.t-s,v.d,v.dd);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator /(S s,const Rall2d<T,V,S>& rhs)
    {
    Rall2d<T,V,S> tmp;
    tmp.t = s/rhs.t;
    tmp.d = (-tmp.t*rhs.d)/rhs.t;
    tmp.dd= (-S(2)*tmp.d*rhs.d-tmp.t*rhs.dd)/rhs.t;
    return tmp;
}


template <class T,class V,class S>
INLINE  Rall2d<T,V,S> operator /(const Rall2d<T,V,S>& v,S s)
    {
    return Rall2d<T,V,S>(v.t/s,v.d/s,v.dd/s);
    }


template <class T,class V,class S>
INLINE  Rall2d<T,V,S> exp(const Rall2d<T,V,S>& arg)
    {
    Rall2d<T,V,S> tmp;
    tmp.t  = exp(arg.t);
    tmp.d  = tmp.t*arg.d;
    tmp.dd = tmp.d*arg.d+tmp.t*arg.dd;
    return tmp;
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> log(const Rall2d<T,V,S>& arg)
    {
    Rall2d<T,V,S> tmp;
    tmp.t  = log(arg.t);
    tmp.d  = arg.d/arg.t;
    tmp.dd = (arg.dd-tmp.d*arg.d)/arg.t;
    return tmp;
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> sin(const Rall2d<T,V,S>& arg)
    {
    T v1 = sin(arg.t);
    T v2 = cos(arg.t);
    return Rall2d<T,V,S>(v1,v2*arg.d,v2*arg.dd - (v1*arg.d)*arg.d );
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> cos(const Rall2d<T,V,S>& arg)
    {
    T v1 = cos(arg.t);
    T v2 = -sin(arg.t);
    return Rall2d<T,V,S>(v1,v2*arg.d, v2*arg.dd - (v1*arg.d)*arg.d);
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> tan(const Rall2d<T,V,S>& arg)
    {
    T v1 = tan(arg.t);
    T v2 = S(1)+sqr(v1);
    return Rall2d<T,V,S>(v1,v2*arg.d, v2*(arg.dd+(S(2)*v1*sqr(arg.d))));
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> sinh(const Rall2d<T,V,S>& arg)
    {
    T v1 = sinh(arg.t);
    T v2 = cosh(arg.t);
    return Rall2d<T,V,S>(v1,v2*arg.d,v2*arg.dd + (v1*arg.d)*arg.d );
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> cosh(const Rall2d<T,V,S>& arg)
    {
    T v1 = cosh(arg.t);
    T v2 = sinh(arg.t);
    return Rall2d<T,V,S>(v1,v2*arg.d,v2*arg.dd + (v1*arg.d)*arg.d );
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> tanh(const Rall2d<T,V,S>& arg)
    {       
    T v1 = tanh(arg.t);
    T v2 = S(1)-sqr(v1);
    return Rall2d<T,V,S>(v1,v2*arg.d, v2*(arg.dd-(S(2)*v1*sqr(arg.d))));
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> sqr(const Rall2d<T,V,S>& arg)
    {
    return Rall2d<T,V,S>(arg.t*arg.t,
                        (S(2)*arg.t)*arg.d,
                        S(2)*(sqr(arg.d)+arg.t*arg.dd)
                        );
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> pow(const Rall2d<T,V,S>& arg,double m) 
    {
    Rall2d<T,V,S> tmp;
    tmp.t = pow(arg.t,m);
    T v2  = (m/arg.t)*tmp.t;
    tmp.d  = v2*arg.d;
    tmp.dd = (S((m-1))/arg.t)*tmp.d*arg.d + v2*arg.dd;
    return tmp;
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> sqrt(const Rall2d<T,V,S>& arg)
    {
    /* By inversion of sqr(x) :*/
    Rall2d<T,V,S> tmp;
    tmp.t  = sqrt(arg.t);
    tmp.d  = (S(0.5)/tmp.t)*arg.d;
    tmp.dd = (S(0.5)*arg.dd-sqr(tmp.d))/tmp.t;
    return tmp;
    }

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> asin(const Rall2d<T,V,S>& arg)
{
    /* By inversion of sin(x) */
    Rall2d<T,V,S> tmp;
    tmp.t  = asin(arg.t);
    T v = cos(tmp.t);
    tmp.d  = arg.d/v;
    tmp.dd = (arg.dd+arg.t*sqr(tmp.d))/v;
    return tmp;
}

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> acos(const Rall2d<T,V,S>& arg)
{
    /* By inversion of cos(x) */
    Rall2d<T,V,S> tmp;
    tmp.t  = acos(arg.t);
    T v = -sin(tmp.t);
    tmp.d  = arg.d/v;
    tmp.dd = (arg.dd+arg.t*sqr(tmp.d))/v;
    return tmp;

}

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> atan(const Rall2d<T,V,S>& x)
{
    /* By inversion of tan(x) */
    Rall2d<T,V,S> tmp;
    tmp.t  = atan(x.t);
    T v    = S(1)+sqr(x.t);
    tmp.d  = x.d/v;
    tmp.dd = x.dd/v-(S(2)*x.t)*sqr(tmp.d);
    return tmp;
}

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> atan2(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x)
{
    Rall2d<T,V,S> tmp;
    tmp.t  = atan2(y.t,x.t);
    T v    = sqr(y.t)+sqr(x.t);
    tmp.d  = (x.t*y.d-x.d*y.t)/v;
    tmp.dd = ( x.t*y.dd-x.dd*y.t-S(2)*(x.t*x.d+y.t*y.d)*tmp.d ) / v;
    return tmp;
}

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> abs(const Rall2d<T,V,S>& x)
{
    T v(Sign(x));
    return Rall2d<T,V,S>(v*x,v*x.d,v*x.dd);
}

template <class T,class V,class S>
INLINE  Rall2d<T,V,S> hypot(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x)
{
    Rall2d<T,V,S> tmp;
    tmp.t  = hypot(y.t,x.t);
    tmp.d  = (x.t*x.d+y.t*y.d)/tmp.t;
    tmp.dd = (sqr(x.d)+x.t*x.dd+sqr(y.d)+y.t*y.dd-sqr(tmp.d))/tmp.t;
    return tmp;
}
// returns sqrt(y*y+x*x), but is optimized for accuracy and speed.

template <class T,class V,class S>
INLINE  S Norm(const Rall2d<T,V,S>& value) 
{
    return Norm(value.t);
}
// returns Norm( value.Value() ).


// (should also be declared on primitive types to improve uniformity
template <class T,class V,class S>
INLINE  Rall2d<T,V,S> LinComb(S alfa,const Rall2d<T,V,S>& a,
    const T& beta,const Rall2d<T,V,S>& b ) {
        return Rall2d<T,V,S>(
            LinComb(alfa,a.t,beta,b.t),
            LinComb(alfa,a.d,beta,b.d),
            LinComb(alfa,a.dd,beta,b.dd) 
        );
}

template <class T,class V,class S>
INLINE  void LinCombR(S alfa,const Rall2d<T,V,S>& a,
    const T& beta,const Rall2d<T,V,S>& b,Rall2d<T,V,S>& result ) {
            LinCombR(alfa, a.t,       beta, b.t,      result.t);
            LinCombR(alfa, a.d,    beta, b.d,   result.d);
            LinCombR(alfa, a.dd,    beta, b.dd,   result.dd);
}

template <class T,class V,class S>
INLINE  void SetToZero(Rall2d<T,V,S>& value)
    {
    SetToZero(value.t);
    SetToZero(value.d);
    SetToZero(value.dd);
    }

template <class T,class V,class S>
INLINE  void SetToIdentity(Rall2d<T,V,S>& value)
    {
    SetToZero(value.d);
    SetToIdentity(value.t);
    SetToZero(value.dd);
    }

template <class T,class V,class S>
INLINE  bool Equal(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x,double eps=epsilon)
{
    return (Equal(x.t,y.t,eps)&&
            Equal(x.d,y.d,eps)&&
            Equal(x.dd,y.dd,eps)
            );
}

template <class T,class V,class S>
INLINE  bool operator==(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x)
{
#ifdef KDL_USE_EQUAL
    return Equal(y, x);
#else
    return (x.t == y.t &&
            x.d == y.d &&
            x.dd == y.dd);
#endif

}

template <class T,class V,class S>
INLINE  bool operator!=(const Rall2d<T,V,S>& y,const Rall2d<T,V,S>& x)
{
    return !operator==(y, x);
}

}


#endif
