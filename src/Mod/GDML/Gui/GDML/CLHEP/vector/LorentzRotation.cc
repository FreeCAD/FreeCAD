// -*- C++ -*-
// $Id: LorentzRotation.cc,v 1.2 2003/08/13 20:00:14 garren Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// This is the implementation basic parts of the HepLorentzRotation class.
//
// Some ZOOM methods involving construction from columns and decomposition 
// into boost*rotation are split off into LorentzRotationC and LorentzRotationD

#ifdef GNUPRAGMA
#pragma implementation
#endif

///#include "CLHEP/Vector/defs.h"
///#include "CLHEP/Vector/LorentzRotation.h"
///#include "CLHEP/Vector/ZMxpv.h"
#include "../vector/defs.h"
#include "../vector/LorentzRotation.h"
#include "../vector/ZMxpv.h"

#include <iostream>
#include <iomanip>

namespace CLHEP  {

// ----------  Constructors and Assignment:


HepLorentzRotation & HepLorentzRotation::set
				(double bx, double by, double bz) {
  double bp2 = bx*bx + by*by + bz*bz;
  if (bp2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
    "Boost Vector supplied to set HepLorentzRotation represents speed >= c."));
  }    
  double gamma = 1.0 / sqrt(1.0 - bp2);
  double bgamma = gamma * gamma / (1.0 + gamma);
  mxx = 1.0 + bgamma * bx * bx;
  myy = 1.0 + bgamma * by * by;
  mzz = 1.0 + bgamma * bz * bz;
  mxy = myx = bgamma * bx * by;
  mxz = mzx = bgamma * bx * bz;
  myz = mzy = bgamma * by * bz;
  mxt = mtx = gamma * bx;
  myt = mty = gamma * by;
  mzt = mtz = gamma * bz;
  mtt = gamma;
  return *this;
}
/*
HepLorentzRotation & HepLorentzRotation::set 
		(const HepBoost & B, const HepRotation & R) {
  set (B.rep4x4());
  *this = matrixMultiplication ( R.rep4x4() );
  return *this;
}

HepLorentzRotation & HepLorentzRotation::set 
		(const HepRotation & R, const HepBoost & B) {
  set (R.rep4x4());
  *this = matrixMultiplication ( B.rep4x4() );
  return *this;
}
*/
// ----------  Accessors:

// ------------  Subscripting:

double HepLorentzRotation::operator () (int i, int j) const {
  if (i == 0) {
    if (j == 0) { return xx(); }
    if (j == 1) { return xy(); }
    if (j == 2) { return xz(); } 
    if (j == 3) { return xt(); } 
  } else if (i == 1) {
    if (j == 0) { return yx(); }
    if (j == 1) { return yy(); }
    if (j == 2) { return yz(); } 
    if (j == 3) { return yt(); } 
  } else if (i == 2) {
    if (j == 0) { return zx(); }
    if (j == 1) { return zy(); }
    if (j == 2) { return zz(); } 
    if (j == 3) { return zt(); } 
  } else if (i == 3) {
    if (j == 0) { return tx(); }
    if (j == 1) { return ty(); }
    if (j == 2) { return tz(); } 
    if (j == 3) { return tt(); } 
  } 
  std::cerr << "HepLorentzRotation subscripting: bad indeces "
	    << "(" << i << "," << j << ")\n";
  return 0.0;
} 

// ---------- Application:


// ---------- Comparison:

int HepLorentzRotation::compare( const HepLorentzRotation & m  ) const {
       if (mtt<m.mtt) return -1; else if (mtt>m.mtt) return 1;
  else if (mtz<m.mtz) return -1; else if (mtz>m.mtz) return 1;
  else if (mty<m.mty) return -1; else if (mty>m.mty) return 1;
  else if (mtx<m.mtx) return -1; else if (mtx>m.mtx) return 1;

  else if (mzt<m.mzt) return -1; else if (mzt>m.mzt) return 1;
  else if (mzz<m.mzz) return -1; else if (mzz>m.mzz) return 1;
  else if (mzy<m.mzy) return -1; else if (mzy>m.mzy) return 1;
  else if (mzx<m.mzx) return -1; else if (mzx>m.mzx) return 1;

  else if (myt<m.myt) return -1; else if (myt>m.myt) return 1;
  else if (myz<m.myz) return -1; else if (myz>m.myz) return 1;
  else if (myy<m.myy) return -1; else if (myy>m.myy) return 1;
  else if (myx<m.myx) return -1; else if (myx>m.myx) return 1;

  else if (mxt<m.mxt) return -1; else if (mxt>m.mxt) return 1;
  else if (mxz<m.mxz) return -1; else if (mxz>m.mxz) return 1;
  else if (mxy<m.mxy) return -1; else if (mxy>m.mxy) return 1;
  else if (mxx<m.mxx) return -1; else if (mxx>m.mxx) return 1;

  else return 0;
}


// ---------- Operations in the group of 4-Rotations

HepLorentzRotation
HepLorentzRotation::matrixMultiplication(const HepRep4x4 & m) const {
  return HepLorentzRotation(
    mxx*m.xx_ + mxy*m.yx_ + mxz*m.zx_ + mxt*m.tx_,
    mxx*m.xy_ + mxy*m.yy_ + mxz*m.zy_ + mxt*m.ty_,
    mxx*m.xz_ + mxy*m.yz_ + mxz*m.zz_ + mxt*m.tz_,
    mxx*m.xt_ + mxy*m.yt_ + mxz*m.zt_ + mxt*m.tt_,

    myx*m.xx_ + myy*m.yx_ + myz*m.zx_ + myt*m.tx_,
    myx*m.xy_ + myy*m.yy_ + myz*m.zy_ + myt*m.ty_,
    myx*m.xz_ + myy*m.yz_ + myz*m.zz_ + myt*m.tz_,
    myx*m.xt_ + myy*m.yt_ + myz*m.zt_ + myt*m.tt_,

    mzx*m.xx_ + mzy*m.yx_ + mzz*m.zx_ + mzt*m.tx_,
    mzx*m.xy_ + mzy*m.yy_ + mzz*m.zy_ + mzt*m.ty_,
    mzx*m.xz_ + mzy*m.yz_ + mzz*m.zz_ + mzt*m.tz_,
    mzx*m.xt_ + mzy*m.yt_ + mzz*m.zt_ + mzt*m.tt_,

    mtx*m.xx_ + mty*m.yx_ + mtz*m.zx_ + mtt*m.tx_,
    mtx*m.xy_ + mty*m.yy_ + mtz*m.zy_ + mtt*m.ty_,
    mtx*m.xz_ + mty*m.yz_ + mtz*m.zz_ + mtt*m.tz_,
    mtx*m.xt_ + mty*m.yt_ + mtz*m.zt_ + mtt*m.tt_ );
}

HepLorentzRotation & HepLorentzRotation::rotateX(double delta) {
  double c = cos (delta);
  double s = sin (delta);
  HepLorentzVector rowy = row2();
  HepLorentzVector rowz = row3();
  HepLorentzVector r2 = c * rowy - s * rowz;
  HepLorentzVector r3 = s * rowy + c * rowz;
  myx = r2.x();   myy = r2.y();   myz = r2.z();   myt = r2.t();	
  mzx = r3.x();   mzy = r3.y();   mzz = r3.z();   mzt = r3.t();	
  return *this;
}

HepLorentzRotation & HepLorentzRotation::rotateY(double delta) {
  double c = cos (delta);
  double s = sin (delta);
  HepLorentzVector rowx = row1();
  HepLorentzVector rowz = row3();
  HepLorentzVector r1 =  c * rowx + s * rowz;
  HepLorentzVector r3 = -s * rowx + c * rowz;
  mxx = r1.x();   mxy = r1.y();   mxz = r1.z();   mxt = r1.t();	
  mzx = r3.x();   mzy = r3.y();   mzz = r3.z();   mzt = r3.t();	
  return *this;
}

HepLorentzRotation & HepLorentzRotation::rotateZ(double delta) {
  double c = cos (delta);
  double s = sin (delta);
  HepLorentzVector rowx = row1();
  HepLorentzVector rowy = row2();
  HepLorentzVector r1 = c * rowx - s * rowy;
  HepLorentzVector r2 = s * rowx + c * rowy;
  mxx = r1.x();   mxy = r1.y();   mxz = r1.z();   mxt = r1.t();
  myx = r2.x();   myy = r2.y();   myz = r2.z();   myt = r2.t();
  return *this;
}
/*
HepLorentzRotation & HepLorentzRotation::boostX(double beta) {
  double b2 = beta*beta;
  if (b2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
    "Beta supplied to HepLorentzRotation::boostX represents speed >= c."));
  }    
  double g  = 1.0/sqrt(1.0-b2);
  double bg = beta*g;
  HepLorentzVector rowx = row1();
  HepLorentzVector rowt = row4();
  HepLorentzVector r1 =  g * rowx + bg * rowt;
  HepLorentzVector r4 = bg * rowx +  g * rowt;
  mxx = r1.x();   mxy = r1.y();   mxz = r1.z();   mxt = r1.t();	
  mtx = r4.x();   mty = r4.y();   mtz = r4.z();   mtt = r4.t();	
  return *this;
}

HepLorentzRotation & HepLorentzRotation::boostY(double beta) {
  double b2 = beta*beta;
  if (b2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
    "Beta supplied to HepLorentzRotation::boostY represents speed >= c."));
  }    
  double g  = 1.0/sqrt(1.0-b2);
  double bg = beta*g;
  HepLorentzVector rowy = row2();
  HepLorentzVector rowt = row4();
  HepLorentzVector r2 =  g * rowy + bg * rowt;
  HepLorentzVector r4 = bg * rowy +  g * rowt;
  myx = r2.x();   myy = r2.y();   myz = r2.z();   myt = r2.t();	
  mtx = r4.x();   mty = r4.y();   mtz = r4.z();   mtt = r4.t();	
  return *this;
}

HepLorentzRotation & HepLorentzRotation::boostZ(double beta) {
  double b2 = beta*beta;
  if (b2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
    "Beta supplied to HepLorentzRotation::boostZ represents speed >= c."));
  }    
  double g  = 1.0/sqrt(1.0-b2);
  double bg = beta*g;
  HepLorentzVector rowz = row3();
  HepLorentzVector rowt = row4();
  HepLorentzVector r3 =  g * rowz + bg * rowt;
  HepLorentzVector r4 = bg * rowz +  g * rowt;
  mtx = r4.x();   mty = r4.y();   mtz = r4.z();   mtt = r4.t();	
  mzx = r3.x();   mzy = r3.y();   mzz = r3.z();   mzt = r3.t();	
  return *this;
}
*/
std::ostream & HepLorentzRotation::print( std::ostream & os ) const {
//  using std::setw;
//  using std::setprecision;
  os << "\n   [ ( " <<
        std::setw(11) << std::setprecision(6) << xx() << "   " <<
        std::setw(11) << std::setprecision(6) << xy() << "   " <<
        std::setw(11) << std::setprecision(6) << xz() << "   " <<
        std::setw(11) << std::setprecision(6) << xt() << ")\n"
     << "     ( " <<
        std::setw(11) << std::setprecision(6) << yx() << "   " <<
        std::setw(11) << std::setprecision(6) << yy() << "   " <<
        std::setw(11) << std::setprecision(6) << yz() << "   " <<
        std::setw(11) << std::setprecision(6) << yt() << ")\n"
     << "     ( " <<
        std::setw(11) << std::setprecision(6) << zx() << "   " <<
        std::setw(11) << std::setprecision(6) << zy() << "   " <<
        std::setw(11) << std::setprecision(6) << zz() << "   " <<
        std::setw(11) << std::setprecision(6) << zt() << ")\n"
     << "     ( " <<
        std::setw(11) << std::setprecision(6) << tx() << "   " <<
        std::setw(11) << std::setprecision(6) << ty() << "   " <<
        std::setw(11) << std::setprecision(6) << tz() << "   " <<
        std::setw(11) << std::setprecision(6) << tt() << ") ]\n";
  return os;
}

HepLorentzRotation operator* ( const HepRotation & r,
                               const HepLorentzRotation & lt) {
  r.rep4x4();
  lt.rep4x4();
  return HepLorentzRotation( HepRep4x4(
         r.xx()*lt.xx() + r.xy()*lt.yx() + r.xz()*lt.zx() + r.xt()*lt.tx(),
	 r.xx()*lt.xy() + r.xy()*lt.yy() + r.xz()*lt.zy() + r.xt()*lt.ty(),
	 r.xx()*lt.xz() + r.xy()*lt.yz() + r.xz()*lt.zz() + r.xt()*lt.tz(),
	 r.xx()*lt.xt() + r.xy()*lt.yt() + r.xz()*lt.zt() + r.xt()*lt.tt(),

         r.yx()*lt.xx() + r.yy()*lt.yx() + r.yz()*lt.zx() + r.yt()*lt.tx(),
         r.yx()*lt.xy() + r.yy()*lt.yy() + r.yz()*lt.zy() + r.yt()*lt.ty(),
         r.yx()*lt.xz() + r.yy()*lt.yz() + r.yz()*lt.zz() + r.yt()*lt.tz(),
         r.yx()*lt.xt() + r.yy()*lt.yt() + r.yz()*lt.zt() + r.yt()*lt.tt(),

         r.zx()*lt.xx() + r.zy()*lt.yx() + r.zz()*lt.zx() + r.zt()*lt.tx(),
         r.zx()*lt.xy() + r.zy()*lt.yy() + r.zz()*lt.zy() + r.zt()*lt.ty(),
         r.zx()*lt.xz() + r.zy()*lt.yz() + r.zz()*lt.zz() + r.zt()*lt.tz(),
         r.zx()*lt.xt() + r.zy()*lt.yt() + r.zz()*lt.zt() + r.zt()*lt.tt(),

         r.tx()*lt.xx() + r.ty()*lt.yx() + r.tz()*lt.zx() + r.tt()*lt.tx(),
         r.tx()*lt.xy() + r.ty()*lt.yy() + r.tz()*lt.zy() + r.tt()*lt.ty(),
         r.tx()*lt.xz() + r.ty()*lt.yz() + r.tz()*lt.zz() + r.tt()*lt.tz(),
         r.tx()*lt.xt() + r.ty()*lt.yt() + r.tz()*lt.zt() + r.tt()*lt.tt() ) );
}


const HepLorentzRotation HepLorentzRotation::IDENTITY;

}  // namespace CLHEP
