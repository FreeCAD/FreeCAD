//  MEFISTO :  library to compute 2D triangulation from segmented boundaries
//
// Copyright (C) 2006-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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
// BUT:   Definir les espaces affines R R2 R3 R4 soit Rn pour n=1,2,3,4
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

//le type Nom des entites geometriques P L S V O
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
//la classe R2
//============
class R2 
{
  friend std::ostream& operator << (std::ostream& f, const R2 & P)
  { f << P.x << ' ' << P.y ; return f; }
  friend std::istream& operator >> (std::istream& f, R2 & P)
  { f >> P.x >> P.y ; return f; }

  friend std::ostream& operator << (std::ostream& f, const R2 * P)
  { f << P->x << ' ' << P->y ; return f; }
  friend std::istream& operator >> (std::istream& f, R2 * P)
  { f >> P->x >> P->y ; return f; }

public:
  R x,y;  //les donnees

  R2 () :x(0),y(0) {}              //les constructeurs
  R2 (R a,R b)   :x(a),y(b)  {}
  R2 (R2 A,R2 B) :x(B.x-A.x),y(B.y-A.y)  {} //vecteur defini par 2 points

  R2  operator+(R2 P) const {return R2(x+P.x,y+P.y);}     // Q+P possible
  R2  operator+=(R2 P)  {x += P.x;y += P.y; return *this;}// Q+=P;
  R2  operator-(R2 P) const {return R2(x-P.x,y-P.y);}     // Q-P
  R2  operator-=(R2 P) {x -= P.x;y -= P.y; return *this;} // Q-=P;
  R2  operator-()const  {return R2(-x,-y);}               // -Q
  R2  operator+()const  {return *this;}                   // +Q
  R   operator,(R2 P)const {return x*P.x+y*P.y;} // produit scalaire (Q,P)
  R   operator^(R2 P)const {return x*P.y-y*P.x;} // produit vectoriel Q^P
  R2  operator*(R c)const {return R2(x*c,y*c);}  // produit a droite  P*c
  R2  operator*=(R c)  {x *= c; y *= c; return *this;}
  R2  operator/(R c)const {return R2(x/c,y/c);}  // division par un reel
  R2  operator/=(R c)  {x /= c; y /= c; return *this;}
  R & operator[](int i) {return (&x)[i];}        // la coordonnee i
  R2  orthogonal() {return R2(-y,x);}    //le vecteur orthogonal dans R2
  friend R2 operator*(R c,R2 P) {return P*c;}    // produit a gauche c*P
};


//la classe R3
//============
class R3
{
  friend std::ostream& operator << (std::ostream& f, const R3 & P)
  { f << P.x << ' ' << P.y << ' ' << P.z ; return f; }
  friend std::istream& operator >> (std::istream& f, R3 & P)
  { f >> P.x >> P.y >> P.z ; return f; }

  friend std::ostream& operator << (std::ostream& f, const R3 * P)
  { f << P->x << ' ' << P->y << ' ' << P->z ; return f; }
  friend std::istream& operator >> (std::istream& f, R3 * P)
  { f >> P->x >> P->y >> P->z ; return f; }

public:  
  R  x,y,z;  //les 3 coordonnees
 
  R3 () :x(0),y(0),z(0) {}  //les constructeurs
  R3 (R a,R b,R c):x(a),y(b),z(c)  {}                  //Point ou Vecteur (a,b,c)
  R3 (R3 A,R3 B):x(B.x-A.x),y(B.y-A.y),z(B.z-A.z)  {}  //Vecteur AB

  R3 (gp_Pnt P) : x(P.X()), y(P.Y()), z(P.Z()) {}      //Point     d'OpenCascade
  R3 (gp_Vec V) : x(V.X()), y(V.Y()), z(V.Z()) {}      //Vecteur   d'OpenCascade
  R3 (gp_Dir P) : x(P.X()), y(P.Y()), z(P.Z()) {}      //Direction d'OpenCascade

  R3   operator+(R3 P)const  {return R3(x+P.x,y+P.y,z+P.z);}
  R3   operator+=(R3 P)  {x += P.x; y += P.y; z += P.z; return *this;}
  R3   operator-(R3 P)const  {return R3(x-P.x,y-P.y,z-P.z);}
  R3   operator-=(R3 P)  {x -= P.x; y -= P.y; z -= P.z; return *this;}
  R3   operator-()const  {return R3(-x,-y,-z);}
  R3   operator+()const  {return *this;}
  R    operator,(R3 P)const {return  x*P.x+y*P.y+z*P.z;} // produit scalaire
  R3   operator^(R3 P)const {return R3(y*P.z-z*P.y ,P.x*z-x*P.z, x*P.y-y*P.x);} // produit vectoriel
  R3   operator*(R c)const {return R3(x*c,y*c,z*c);}
  R3   operator*=(R c)  {x *= c; y *= c; z *= c; return *this;}
  R3   operator/(R c)const {return R3(x/c,y/c,z/c);}
  R3   operator/=(R c)  {x /= c; y /= c; z /= c; return *this;}
  R  & operator[](int i) {return (&x)[i];}
  friend R3 operator*(R c,R3 P) {return P*c;}

  R3   operator=(gp_Pnt P) {return R3(P.X(),P.Y(),P.Z());}
  R3   operator=(gp_Dir P) {return R3(P.X(),P.Y(),P.Z());}

  friend gp_Pnt gp_pnt(R3 xyz) { return gp_Pnt(xyz.x,xyz.y,xyz.z); }
  //friend gp_Pnt operator=() { return gp_Pnt(x,y,z); }
  friend gp_Dir gp_dir(R3 xyz) { return gp_Dir(xyz.x,xyz.y,xyz.z); }

  bool  DansPave( R3 & xyzMin, R3 & xyzMax )
    { return xyzMin.x<=x && x<=xyzMax.x &&
             xyzMin.y<=y && y<=xyzMax.y &&
             xyzMin.z<=z && z<=xyzMax.z; }
};

//la classe R4
//============
class R4: public R3
{
  friend std::ostream& operator <<(std::ostream& f, const R4 & P )
  { f << P.x << ' ' << P.y << ' ' << P.z << ' ' << P.omega; return f; }
  friend istream& operator >>(istream& f,  R4 & P)
  { f >> P.x >>  P.y >>  P.z >> P.omega ; return f; }

  friend std::ostream& operator <<(std::ostream& f, const R4 * P )
  { f << P->x << ' ' << P->y << ' ' << P->z << ' ' << P->omega; return f; }
  friend istream& operator >>(istream& f,  R4 * P)
  { f >> P->x >>  P->y >>  P->z >> P->omega ; return f; }

public:  
  R  omega;  //la donnee du poids supplementaire
 
  R4 () :omega(1.0) {}  //les constructeurs
  R4 (R a,R b,R c,R d):R3(a,b,c),omega(d) {}
  R4 (R4 A,R4 B) :R3(B.x-A.x,B.y-A.y,B.z-A.z),omega(B.omega-A.omega) {}

  R4   operator+(R4 P)const  {return R4(x+P.x,y+P.y,z+P.z,omega+P.omega);}
  R4   operator+=(R4 P)  {x += P.x;y += P.y;z += P.z;omega += P.omega;return *this;}
  R4   operator-(R4 P)const  {return R4(x-P.x,y-P.y,z-P.z,omega-P.omega);}
  R4   operator-=(R4 P) {x -= P.x;y -= P.y;z -= P.z;omega -= P.omega;return *this;}
  R4   operator-()const  {return R4(-x,-y,-z,-omega);}
  R4   operator+()const  {return *this;}
  R    operator,(R4 P)const {return  x*P.x+y*P.y+z*P.z+omega*P.omega;} // produit scalaire
  R4   operator*(R c)const {return R4(x*c,y*c,z*c,omega*c);}
  R4   operator*=(R c)  {x *= c; y *= c; z *= c; omega *= c; return *this;}
  R4   operator/(R c)const {return R4(x/c,y/c,z/c,omega/c);}
  R4   operator/=(R c)  {x /= c; y /= c; z /= c; omega /= c; return *this;}
  R  & operator[](int i) {return (&x)[i];}
  friend R4 operator*(R c,R4 P) {return P*c;}
};

//quelques fonctions supplementaires sur ces classes
//==================================================
inline R Aire2d(const R2 A,const R2 B,const R2 C){return (B-A)^(C-A);} 
inline R Angle2d(R2 P){ return atan2(P.y,P.x);}

inline R Norme2_2(const R2 & A){ return (A,A);}
inline R Norme2(const R2 & A){ return sqrt((A,A));}
inline R NormeInfinie(const R2 & A){return Max(Abs(A.x),Abs(A.y));}

inline R Norme2_2(const R3 & A){ return (A,A);}
inline R Norme2(const R3 & A){ return sqrt((A,A));}
inline R NormeInfinie(const R3 & A){return Max(Abs(A.x),Abs(A.y),Abs(A.z));}

inline R Norme2_2(const R4 & A){ return (A,A);}
inline R Norme2(const R4 & A){ return sqrt((A,A));}
inline R NormeInfinie(const R4 & A){return Max(Abs(A.x),Abs(A.y),Abs(A.z),Abs(A.omega));}

inline R2 XY(R3 P) {return R2(P.x, P.y);}  //restriction a R2 d'un R3 par perte de z
inline R3 Min(R3 P, R3 Q) 
{return R3(P.x<Q.x ? P.x : Q.x, P.y<Q.y ? P.y : Q.y, P.z<Q.z ? P.z : Q.z);} //Pt de xyz Min
inline R3 Max(R3 P, R3 Q) 
{return R3(P.x>Q.x ? P.x : Q.x, P.y>Q.y ? P.y : Q.y, P.z>Q.z ? P.z : Q.z);} //Pt de xyz Max

#endif
