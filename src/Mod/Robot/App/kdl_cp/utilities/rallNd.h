#ifndef RALLND_H
#define RALLND_H

#include "rall1d.h"
#include "rall1d_io.h"
#include "rall2d.h"
#include "rall2d_io.h"
/**
 * The Rall1d class allows for a 24-line implementation of rall numbers
 * generalized to the Nth derivative !
 * The efficiency is not very good for high derivatives.
 * This could be improved by also using Rall2d 
 */
/*
template <int N>
class RallNd :
	public Rall1d< RallNd<N-1>, RallNd<N-1>, double >
{
public:
	RallNd() {}
	RallNd(const Rall1d< RallNd<N-1>, RallNd<N-1>,double>& arg) : 
		Rall1d< RallNd<N-1>, RallNd<N-1>,double>(arg) {}
	RallNd(double value,double d[]) {
		this->t    = RallNd<N-1>(value,d);
		this->grad = RallNd<N-1>(d[0],&d[1]);
	}
};

template <>
class RallNd<1> : public Rall1d<double>  {
public:
	RallNd() {}
	RallNd(const Rall1d<double>& arg) :
		Rall1d<double,double,double>(arg) {}
	RallNd(double value,double d[]) {
		t    = value;
		grad = d[0];
	}
};
*/
/**
 * to be checked..
 */

/**
 * Als je tot 3de orde een efficiente berekening kan doen,
 * dan kan je tot een willekeurige orde alles efficient berekenen
 * 0 1 2 3
 * ==\> 1 2 3 4
 * ==\> 3 4 5 6    
 * 4 5 6 7
 *
 * de aangeduide berekeningen zijn niet noodzakelijk, en er is dan niets
 * verniet berekend in de recursieve implementatie.  
 * of met 2de orde over 1ste order : kan ook efficient :
 * 0 1
 * ==\>1 2
 * 2 3
 */
// N>2:
template <int N>
class RallNd :
	public Rall2d< RallNd<N-2>, RallNd<N-2>, double >
{
public:
	RallNd() {}
	RallNd(const Rall2d< RallNd<N-2>, RallNd<N-2>,double>& arg) : 
		Rall2d< RallNd<N-2>, RallNd<N-2>,double>(arg) {}
	RallNd(double value,double d[]) {
		this->t    = RallNd<N-2>(value,d);    // 0 1 2
		this->d    = RallNd<N-2>(d[0],&d[1]); // 1 2 3 iseigenlijk niet nodig
		this->dd   = RallNd<N-2>(d[1],&d[2]); // 2 3 4
	}
};

template <>
class RallNd<2> : public Rall2d<double>  {
public:
	RallNd() {} /* (dwz. met evenveel numerieke operaties als een  */
	RallNd(const Rall2d<double>& arg) :
		Rall2d<double>(arg) {}
	RallNd(double value,double d[]) {
		t    = value;
		d = d[0];
		dd= d[1];
	}
};

template <>
class RallNd<1> : public Rall1d<double>  {
public:
	RallNd() {}
	RallNd(const Rall1d<double>& arg) :
		Rall1d<double>(arg) {}
	RallNd(double value,double d[]) {
		t    = value;
		grad = d[0];
	}
};

#endif

