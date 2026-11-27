 
/*****************************************************************************
 * \file  
 *      class for automatic differentiation on scalar values and 1st 
 *      derivatives .
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
 *      $Id: rall1d.h,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $ 
 ****************************************************************************/
 
#ifndef Rall1D_H
#define Rall1D_H
#include <assert.h>
#include "utility.h"

namespace KDL {
/**
 * Rall1d contains a value, and its gradient, and defines an algebraic structure on this pair.
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
template <typename T,typename V=T,typename S=T>
class Rall1d
    {
    public:
        typedef T valuetype;
        typedef V gradienttype;
        typedef S scalartype;
    public :
        T t;        //!< value
        V grad;     //!< gradient
    public :
        INLINE Rall1d():t(),grad() {};

        T value() const {
            return t;
        }
        V deriv() const {
            return grad;
        }

        explicit INLINE  Rall1d(typename TI<T>::Arg c)
            {t=T(c);SetToZero(grad);}

        INLINE Rall1d(typename TI<T>::Arg tn, typename TI<V>::Arg afg):t(tn),grad(afg) {}

        INLINE Rall1d(const Rall1d<T,V,S>& r):t(r.t),grad(r.grad) {}
        //if one defines this constructor, it's better optimized then the
        //automatically generated one ( this one set's up a loop to copy
        // word by word.
        
        INLINE T& Value() {
            return t;
        }

        INLINE V& Gradient() {
            return grad;
        }

        INLINE static Rall1d<T,V,S> Zero() {
            Rall1d<T,V,S> tmp;
            SetToZero(tmp);
            return tmp;
        }
        INLINE static Rall1d<T,V,S> Identity() {
            Rall1d<T,V,S> tmp;
            SetToIdentity(tmp);
            return tmp;
        }

        INLINE Rall1d<T,V,S>& operator =(S c)
            {t=c;SetToZero(grad);return *this;}

        INLINE Rall1d<T,V,S>& operator =(const Rall1d<T,V,S>& r)
            {t=r.t;grad=r.grad;return *this;}

        INLINE Rall1d<T,V,S>& operator /=(const Rall1d<T,V,S>& rhs)
            {
            grad = LinComb(rhs.t,grad,-t,rhs.grad) / (rhs.t*rhs.t);
            t     /= rhs.t;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator *=(const Rall1d<T,V,S>& rhs)
            {
            LinCombR(rhs.t,grad,t,rhs.grad,grad);
            t *= rhs.t;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator +=(const Rall1d<T,V,S>& rhs)
            {
            grad +=rhs.grad;
            t    +=rhs.t;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator -=(const Rall1d<T,V,S>& rhs)
            {
            grad -= rhs.grad;
            t     -= rhs.t;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator /=(S rhs)
            {
            grad /= rhs;
            t    /= rhs;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator *=(S rhs)
            {
            grad *= rhs;
            t    *= rhs;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator +=(S rhs)
            {
            t    += rhs;
            return *this;
            }

        INLINE Rall1d<T,V,S>& operator -=(S rhs)
            {
            t    -= rhs;
            return *this;
            }



        // = operators
        /* gives warnings on cygwin 
        
         template <class T2,class V2,class S2>
         friend INLINE  Rall1d<T2,V2,S2> operator /(const Rall1d<T2,V2,S2>& lhs,const Rall1d<T2,V2,S2>& rhs);
         
         friend INLINE  Rall1d<T,V,S> operator *(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs);
         friend INLINE  Rall1d<T,V,S> operator +(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs);
         friend INLINE  Rall1d<T,V,S> operator -(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs);
         friend INLINE  Rall1d<T,V,S> operator -(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> operator *(S s,const Rall1d<T,V,S>& v);
         friend INLINE  Rall1d<T,V,S> operator *(const Rall1d<T,V,S>& v,S s);
         friend INLINE  Rall1d<T,V,S> operator +(S s,const Rall1d<T,V,S>& v);
         friend INLINE  Rall1d<T,V,S> operator +(const Rall1d<T,V,S>& v,S s);
         friend INLINE  Rall1d<T,V,S> operator -(S s,const Rall1d<T,V,S>& v);
         friend INLINE  Rall1d<T,V,S> operator -(const Rall1d<T,V,S>& v,S s);
         friend INLINE  Rall1d<T,V,S> operator /(S s,const Rall1d<T,V,S>& v);
         friend INLINE  Rall1d<T,V,S> operator /(const Rall1d<T,V,S>& v,S s);

        // = Mathematical functions that operate on Rall1d objects
         friend INLINE  Rall1d<T,V,S> exp(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> log(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> sin(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> cos(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> tan(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> sinh(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> cosh(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> sqr(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> pow(const Rall1d<T,V,S>& arg,double m) ;
         friend INLINE  Rall1d<T,V,S> sqrt(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> atan(const Rall1d<T,V,S>& x);
         friend INLINE  Rall1d<T,V,S> hypot(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x);
         friend INLINE  Rall1d<T,V,S> asin(const Rall1d<T,V,S>& x);
         friend INLINE  Rall1d<T,V,S> acos(const Rall1d<T,V,S>& x);
         friend INLINE  Rall1d<T,V,S> abs(const Rall1d<T,V,S>& x);
         friend INLINE  S Norm(const Rall1d<T,V,S>& value) ;
         friend INLINE  Rall1d<T,V,S> tanh(const Rall1d<T,V,S>& arg);
         friend INLINE  Rall1d<T,V,S> atan2(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x);
         
        // = Utility functions to improve performance

         friend INLINE  Rall1d<T,V,S> LinComb(S alfa,const Rall1d<T,V,S>& a,
            const T& beta,const Rall1d<T,V,S>& b );
        
         friend INLINE  void LinCombR(S alfa,const Rall1d<T,V,S>& a,
            const T& beta,const Rall1d<T,V,S>& b,Rall1d<T,V,S>& result );
        
        // = Setting value of a Rall1d object to 0 or 1

         friend INLINE  void SetToZero(Rall1d<T,V,S>& value);
         friend INLINE  void SetToOne(Rall1d<T,V,S>& value);
        // = Equality in an eps-interval
         friend INLINE  bool Equal(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x,double eps);
         */
    };


template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator /(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs)
    {
    return Rall1d<T,V,S>(lhs.t/rhs.t,(lhs.grad*rhs.t-lhs.t*rhs.grad)/(rhs.t*rhs.t));
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator *(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs)
    {
    return Rall1d<T,V,S>(lhs.t*rhs.t,rhs.t*lhs.grad+lhs.t*rhs.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator +(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs)
    {
    return Rall1d<T,V,S>(lhs.t+rhs.t,lhs.grad+rhs.grad);
    }


template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator -(const Rall1d<T,V,S>& lhs,const Rall1d<T,V,S>& rhs)
    {
    return Rall1d<T,V,S>(lhs.t-rhs.t,lhs.grad-rhs.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator -(const Rall1d<T,V,S>& arg)
    {
    return Rall1d<T,V,S>(-arg.t,-arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator *(S s,const Rall1d<T,V,S>& v)
    {
    return Rall1d<T,V,S>(s*v.t,s*v.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator *(const Rall1d<T,V,S>& v,S s)
    {
    return Rall1d<T,V,S>(v.t*s,v.grad*s);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator +(S s,const Rall1d<T,V,S>& v)
    {
    return Rall1d<T,V,S>(s+v.t,v.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator +(const Rall1d<T,V,S>& v,S s)
    {
    return Rall1d<T,V,S>(v.t+s,v.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator -(S s,const Rall1d<T,V,S>& v)
    {
    return Rall1d<T,V,S>(s-v.t,-v.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator -(const Rall1d<T,V,S>& v,S s)
    {
    return Rall1d<T,V,S>(v.t-s,v.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator /(S s,const Rall1d<T,V,S>& v)
    {
    return Rall1d<T,V,S>(s/v.t,(-s*v.grad)/(v.t*v.t));
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> operator /(const Rall1d<T,V,S>& v,S s)
    {
    return Rall1d<T,V,S>(v.t/s,v.grad/s);
    }


template <class T,class V,class S>
INLINE  Rall1d<T,V,S> exp(const Rall1d<T,V,S>& arg)
    {
    T v;
    v= (exp(arg.t));
    return Rall1d<T,V,S>(v,v*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> log(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(log(arg.t));
    return Rall1d<T,V,S>(v,arg.grad/arg.t);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> sin(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(sin(arg.t));
    return Rall1d<T,V,S>(v,cos(arg.t)*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> cos(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(cos(arg.t));
    return Rall1d<T,V,S>(v,-sin(arg.t)*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> tan(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(tan(arg.t));
    return Rall1d<T,V,S>(v,arg.grad/sqr(cos(arg.t)));
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> sinh(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(sinh(arg.t));
    return Rall1d<T,V,S>(v,cosh(arg.t)*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> cosh(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(cosh(arg.t));
    return Rall1d<T,V,S>(v,sinh(arg.t)*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> sqr(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=(arg.t*arg.t);
    return Rall1d<T,V,S>(v,(2.0*arg.t)*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> pow(const Rall1d<T,V,S>& arg,double m) 
    {
    T v;
    v=(pow(arg.t,m));
    return Rall1d<T,V,S>(v,(m*v/arg.t)*arg.grad);
    }

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> sqrt(const Rall1d<T,V,S>& arg)
    {
    T v;
    v=sqrt(arg.t);
    return Rall1d<T,V,S>(v, (0.5/v)*arg.grad);
    }   

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> atan(const Rall1d<T,V,S>& x)
{
    T v;
    v=(atan(x.t));
    return Rall1d<T,V,S>(v,x.grad/(1.0+sqr(x.t)));
}

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> hypot(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x)
{
    T v;
    v=(hypot(y.t,x.t));
    return Rall1d<T,V,S>(v,(x.t/v)*x.grad+(y.t/v)*y.grad);
}

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> asin(const Rall1d<T,V,S>& x)
{
    T v;
    v=(asin(x.t));
    return Rall1d<T,V,S>(v,x.grad/sqrt(1.0-sqr(x.t)));
}

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> acos(const Rall1d<T,V,S>& x)
{
    T v;
    v=(acos(x.t));
    return Rall1d<T,V,S>(v,-x.grad/sqrt(1.0-sqr(x.t)));
}

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> abs(const Rall1d<T,V,S>& x)
{
    T v;
    v=(Sign(x));
    return Rall1d<T,V,S>(v*x,v*x.grad);
}


template <class T,class V,class S>
INLINE  S Norm(const Rall1d<T,V,S>& value) 
{
    return Norm(value.t);
}

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> tanh(const Rall1d<T,V,S>& arg)
{       
    T v(tanh(arg.t));       
    return Rall1d<T,V,S>(v,arg.grad/sqr(cosh(arg.t)));
}

template <class T,class V,class S>
INLINE  Rall1d<T,V,S> atan2(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x)
{
    T v(x.t*x.t+y.t*y.t);
    return Rall1d<T,V,S>(atan2(y.t,x.t),(x.t*y.grad-y.t*x.grad)/v);
}


template <class T,class V,class S>
INLINE  Rall1d<T,V,S> LinComb(S alfa,const Rall1d<T,V,S>& a,
    const T& beta,const Rall1d<T,V,S>& b ) {
        return Rall1d<T,V,S>(
            LinComb(alfa,a.t,beta,b.t),
            LinComb(alfa,a.grad,beta,b.grad)
        );
}

template <class T,class V,class S>
INLINE  void LinCombR(S alfa,const Rall1d<T,V,S>& a,
    const T& beta,const Rall1d<T,V,S>& b,Rall1d<T,V,S>& result ) {
            LinCombR(alfa, a.t,       beta, b.t,      result.t);
            LinCombR(alfa, a.grad,    beta, b.grad,   result.grad);
}


template <class T,class V,class S>
INLINE  void SetToZero(Rall1d<T,V,S>& value)
    {
    SetToZero(value.grad);
    SetToZero(value.t);
    }
template <class T,class V,class S>
INLINE  void SetToIdentity(Rall1d<T,V,S>& value)
    {
    SetToIdentity(value.t);
    SetToZero(value.grad);
    }

template <class T,class V,class S>
INLINE  bool Equal(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x,double eps=epsilon)
{
    return (Equal(x.t,y.t,eps)&&Equal(x.grad,y.grad,eps));
}

template <class T,class V,class S>
INLINE  bool operator==(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x)
{
#ifdef KDL_USE_EQUAL
    return Equal(y, x);
#else
    return (x.t == y.t && x.grad == y.grad);
#endif
}

template <class T,class V,class S>
INLINE  bool operator!=(const Rall1d<T,V,S>& y,const Rall1d<T,V,S>& x)
{
    return !operator==(y, x);
}

}



#endif
