//  MEFISTO :  library to compute 2D triangulation from segmented boundaries
//
// Copyright (C) 2006-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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
//  File   : Rn.h
//  Module : SMESH
//  Authors: Frederic HECHT & Alain PERRONNET
//  Date   : 13 novembre 2006

#ifndef Rn__h
#define Rn__h

#include <gp_Pnt.hxx>      //Dans OpenCascade
#include <gp_Vec.hxx>      //Dans OpenCascade
#include <gp_Dir.hxx>      //Dans OpenCascade

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// BUT:   Definir les espaces affines R R_2 R_3 R_4 soit Rn pour n=1,2,3,4
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AUTEUR : Frederic HECHT      ANALYSE NUMERIQUE UPMC  PARIS   OCTOBRE   2000
// MODIFS : Alain    PERRONNET  ANALYSE NUMERIQUE UPMC  PARIS   NOVEMBRE  2000
//...............................................................................
#include <iostream>
#include <cmath>


template<class T> inline T Abs (const T &a){return a <0 ? -a : a;}
template<class T> inline void Echange (T& a,T& b) {T c=a;a=b;b=c;}

template<class T> inline T Min (const T &a,const T &b)  {return a < b ? a : b;}
template<class T> inline T Max (const T &a,const T & b) {return a > b ? a : b;}

template<class T> inline T Max (const T &a,const T & b,const T & c){return Max(Max(a,b),c);}
template<class T> inline T Min (const T &a,const T & b,const T & c){return Min(Min(a,b),c);}

template<class T> inline T Max (const T &a,const T & b,const T & c,const T & d)
 {return Max(Max(a,b),Max(c,d));}
template<class T> inline T Min (const T &a,const T & b,const T & c,const T & d)
 {return Min(Min(a,b),Min(c,d));}

//le type Nom des entities geometriques P L S V O
//===========
typedef char Nom[1+24];

//le type N des nombres entiers positifs
//=========
#ifndef PCLINUX64
typedef unsigned long int N;
#else 
typedef unsigned int N;
#endif

//le type Z des nombres entiers relatifs
//=========
#ifndef PCLINUX64
typedef long int Z;
#else
typedef int Z;
#endif

//le type R des nombres "reels"
//=========
typedef double R;

//le type XPoint  des coordonnees d'un pixel dans une fenetre
//==============
//typedef struct { short int x,y } XPoint;  //en fait ce type est defini dans X11-Window
                                            // #include <X11/Xlib.h>
//la classe R_2
//============
class R_2
{
  friend std::ostream& operator << (std::ostream& f, const R_2 & P)
  { f << P.x << ' ' << P.y ; return f; }
  friend std::istream& operator >> (std::istream& f, R_2 & P)
  { f >> P.x >> P.y ; return f; }

  friend std::ostream& operator << (std::ostream& f, const R_2 * P)
  { f << P->x << ' ' << P->y ; return f; }
  friend std::istream& operator >> (std::istream& f, R_2 * P)
  { f >> P->x >> P->y ; return f; }

public:
  R x,y;  //les donnees

  R_2 () :x(0),y(0) {}              //les constructeurs
  R_2 (R a,R b)   :x(a),y(b)  {}
  R_2 (R_2 A,R_2 B) :x(B.x-A.x),y(B.y-A.y)  {} //vecteur defini par 2 points

  R_2  operator+(R_2 P) const {return R_2(x+P.x,y+P.y);}     // Q+P possible
  R_2  operator+=(R_2 P)  {x += P.x;y += P.y; return *this;}// Q+=P;
  R_2  operator-(R_2 P) const {return R_2(x-P.x,y-P.y);}     // Q-P
  R_2  operator-=(R_2 P) {x -= P.x;y -= P.y; return *this;} // Q-=P;
  R_2  operator-()const  {return R_2(-x,-y);}               // -Q
  R_2  operator+()const  {return *this;}                   // +Q
  R   operator,(R_2 P)const {return x*P.x+y*P.y;} // produit scalaire (Q,P)
  R   operator^(R_2 P)const {return x*P.y-y*P.x;} // produit vectoriel Q^P
  R_2  operator*(R c)const {return R_2(x*c,y*c);}  // produit a droite  P*c
  R_2  operator*=(R c)  {x *= c; y *= c; return *this;}
  R_2  operator/(R c)const {return R_2(x/c,y/c);}  // division par un reel
  R_2  operator/=(R c)  {x /= c; y /= c; return *this;}
  R & operator[](int i) {return (&x)[i];}        // la coordonnee i
  R_2  orthogonal() {return R_2(-y,x);}    //le vecteur orthogonal dans R_2
  friend R_2 operator*(R c,R_2 P) {return P*c;}    // produit a gauche c*P
};


//la classe R_3
//============
class R_3
{
  friend std::ostream& operator << (std::ostream& f, const R_3 & P)
  { f << P.x << ' ' << P.y << ' ' << P.z ; return f; }
  friend std::istream& operator >> (std::istream& f, R_3 & P)
  { f >> P.x >> P.y >> P.z ; return f; }

  friend std::ostream& operator << (std::ostream& f, const R_3 * P)
  { f << P->x << ' ' << P->y << ' ' << P->z ; return f; }
  friend std::istream& operator >> (std::istream& f, R_3 * P)
  { f >> P->x >> P->y >> P->z ; return f; }

public:  
  R  x,y,z;  //les 3 coordonnees
 
  R_3 () :x(0),y(0),z(0) {}  //les constructeurs
  R_3 (R a,R b,R c):x(a),y(b),z(c)  {}                  //Point ou Vecteur (a,b,c)
  R_3 (R_3 A,R_3 B):x(B.x-A.x),y(B.y-A.y),z(B.z-A.z)  {}  //Vecteur AB

  R_3 (gp_Pnt P) : x(P.X()), y(P.Y()), z(P.Z()) {}      //Point     d'OpenCascade
  R_3 (gp_Vec V) : x(V.X()), y(V.Y()), z(V.Z()) {}      //Vecteur   d'OpenCascade
  R_3 (gp_Dir P) : x(P.X()), y(P.Y()), z(P.Z()) {}      //Direction d'OpenCascade

  R_3   operator+(R_3 P)const  {return R_3(x+P.x,y+P.y,z+P.z);}
  R_3   operator+=(R_3 P)  {x += P.x; y += P.y; z += P.z; return *this;}
  R_3   operator-(R_3 P)const  {return R_3(x-P.x,y-P.y,z-P.z);}
  R_3   operator-=(R_3 P)  {x -= P.x; y -= P.y; z -= P.z; return *this;}
  R_3   operator-()const  {return R_3(-x,-y,-z);}
  R_3   operator+()const  {return *this;}
  R    operator,(R_3 P)const {return  x*P.x+y*P.y+z*P.z;} // produit scalaire
  R_3   operator^(R_3 P)const {return R_3(y*P.z-z*P.y ,P.x*z-x*P.z, x*P.y-y*P.x);} // produit vectoriel
  R_3   operator*(R c)const {return R_3(x*c,y*c,z*c);}
  R_3   operator*=(R c)  {x *= c; y *= c; z *= c; return *this;}
  R_3   operator/(R c)const {return R_3(x/c,y/c,z/c);}
  R_3   operator/=(R c)  {x /= c; y /= c; z /= c; return *this;}
  R  & operator[](int i) {return (&x)[i];}
  friend R_3 operator*(R c,R_3 P) {return P*c;}

  R_3   operator=(gp_Pnt P) {return R_3(P.X(),P.Y(),P.Z());}
  R_3   operator=(gp_Dir P) {return R_3(P.X(),P.Y(),P.Z());}

  friend gp_Pnt gp_pnt(R_3 xyz) { return gp_Pnt(xyz.x,xyz.y,xyz.z); }
  //friend gp_Pnt operator=() { return gp_Pnt(x,y,z); }
  friend gp_Dir gp_dir(R_3 xyz) { return gp_Dir(xyz.x,xyz.y,xyz.z); }

  bool  DansPave( R_3 & xyzMin, R_3 & xyzMax )
    { return xyzMin.x<=x && x<=xyzMax.x &&
             xyzMin.y<=y && y<=xyzMax.y &&
             xyzMin.z<=z && z<=xyzMax.z; }
};

//la classe R_4
//============
class R_4: public R_3
{
  friend std::ostream& operator <<(std::ostream& f, const R_4 & P )
  { f << P.x << ' ' << P.y << ' ' << P.z << ' ' << P.omega; return f; }
  friend std::istream& operator >>(std::istream& f,  R_4 & P)
  { f >> P.x >>  P.y >>  P.z >> P.omega ; return f; }

  friend std::ostream& operator <<(std::ostream& f, const R_4 * P )
  { f << P->x << ' ' << P->y << ' ' << P->z << ' ' << P->omega; return f; }
  friend std::istream& operator >>(std::istream& f,  R_4 * P)
  { f >> P->x >>  P->y >>  P->z >> P->omega ; return f; }

public:  
  R  omega;  //la donnee du poids supplementaire
 
  R_4 () :omega(1.0) {}  //les constructeurs
  R_4 (R a,R b,R c,R d):R_3(a,b,c),omega(d) {}
  R_4 (R_4 A,R_4 B) :R_3(B.x-A.x,B.y-A.y,B.z-A.z),omega(B.omega-A.omega) {}

  R_4   operator+(R_4 P)const  {return R_4(x+P.x,y+P.y,z+P.z,omega+P.omega);}
  R_4   operator+=(R_4 P)  {x += P.x;y += P.y;z += P.z;omega += P.omega;return *this;}
  R_4   operator-(R_4 P)const  {return R_4(x-P.x,y-P.y,z-P.z,omega-P.omega);}
  R_4   operator-=(R_4 P) {x -= P.x;y -= P.y;z -= P.z;omega -= P.omega;return *this;}
  R_4   operator-()const  {return R_4(-x,-y,-z,-omega);}
  R_4   operator+()const  {return *this;}
  R    operator,(R_4 P)const {return  x*P.x+y*P.y+z*P.z+omega*P.omega;} // produit scalaire
  R_4   operator*(R c)const {return R_4(x*c,y*c,z*c,omega*c);}
  R_4   operator*=(R c)  {x *= c; y *= c; z *= c; omega *= c; return *this;}
  R_4   operator/(R c)const {return R_4(x/c,y/c,z/c,omega/c);}
  R_4   operator/=(R c)  {x /= c; y /= c; z /= c; omega /= c; return *this;}
  R  & operator[](int i) {return (&x)[i];}
  friend R_4 operator*(R c,R_4 P) {return P*c;}
};

//quelques fonctions supplementaires sur ces classes
//==================================================
inline R Aire2d(const R_2 A,const R_2 B,const R_2 C){return (B-A)^(C-A);}
inline R Angle2d(R_2 P){ return atan2(P.y,P.x);}

inline R Norme2_2(const R_2 & A){ return (A,A);}
inline R Norme2(const R_2 & A){ return sqrt((A,A));}
inline R NormeInfinie(const R_2 & A){return Max(Abs(A.x),Abs(A.y));}

inline R Norme2_2(const R_3 & A){ return (A,A);}
inline R Norme2(const R_3 & A){ return sqrt((A,A));}
inline R NormeInfinie(const R_3 & A){return Max(Abs(A.x),Abs(A.y),Abs(A.z));}

inline R Norme2_2(const R_4 & A){ return (A,A);}
inline R Norme2(const R_4 & A){ return sqrt((A,A));}
inline R NormeInfinie(const R_4 & A){return Max(Abs(A.x),Abs(A.y),Abs(A.z),Abs(A.omega));}

inline R_2 XY(R_3 P) {return R_2(P.x, P.y);}  //restriction a R_2 d'un R_3 par perte de z
inline R_3 Min(R_3 P, R_3 Q)
{return R_3(P.x<Q.x ? P.x : Q.x, P.y<Q.y ? P.y : Q.y, P.z<Q.z ? P.z : Q.z);} //Pt de xyz Min
inline R_3 Max(R_3 P, R_3 Q)
{return R_3(P.x>Q.x ? P.x : Q.x, P.y>Q.y ? P.y : Q.y, P.z>Q.z ? P.z : Q.z);} //Pt de xyz Max

#endif
