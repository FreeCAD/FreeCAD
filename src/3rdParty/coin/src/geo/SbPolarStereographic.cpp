/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include "SbPolarStereographic.h"
#include <cmath>
#include "SbGeoAngle.h"
#include <cstdio>

SbPolarStereographic::SbPolarStereographic(const SbGeoEllipsoid & ellipsoid,
                                           double FE, double FN)
  : inherited(ellipsoid, FE, FN)
{
  double phiF = this->ellipsoid.getLatStdParallel();
  double e = this->ellipsoid.getE();

  this->tF = (this->ellipsoid.getHemisphere() == 'N') ?
    tan(M_PI / 4.0 - phiF / 2.0) / pow((1.0 - e * sin(phiF)) / (1.0 + e * sin(phiF)), e / 2) :
    tan(M_PI / 4.0 + phiF / 2.0) / pow((1.0 + e * sin(phiF)) / (1.0 - e * sin(phiF)), e / 2);

  this->mF = cos(phiF) / pow(1.0 - e * e * sin(phiF) * sin(phiF), 0.5);
  this->k0 = (mF * (pow(pow((1.0 + e), (1.0 + e)) * pow((1.0 - e), (1.0 - e)), 0.5))) / (2 * tF);

#if 0
  printf("init projector:\n");
  printf("phiF = %f\n", phiF);
  printf("tF = %f\n", tF);
  printf("mF = %f\n", mF);
  printf("k0 = %f\n", k0);
#endif
#if 0
  printf("testing project (64, 0):\n");
  double east, north;
  SbGeoAngle phi, lambda;
  phi.setDegree(64);
  lambda.setDegree(0);
  this->project(phi, lambda, &east, &north);
  printf("result: %.1f %.1f\n", east, north);
#endif
}


void
SbPolarStereographic::project(const SbGeoAngle & phi_in,
                              const SbGeoAngle & lambda_in,
                              double * easting,
                              double * northing) const
{
  double phi = phi_in;
  double lambda = lambda_in;

  double a = this->ellipsoid.getA();
  double e = this->ellipsoid.getE();

  SbGeoAngle lambda0 = this->ellipsoid.getLongOrigin();

  double t = (this->ellipsoid.getHemisphere() == 'N') ?
    tan(M_PI / 4.0 - phi / 2.0) / pow((1.0 - e * sin(phi)) / (1.0 + e * sin(phi)) , e / 2) :
    tan(M_PI / 4.0 + phi / 2.0) / pow((1.0 + e * sin(phi)) / (1.0 - e * sin(phi)), e / 2);

  double rho = (2 * a * k0 * t) / pow((pow((1.0 + e), (1.0 + e)) * pow((1.0 - e), (1.0 - e))), 0.5);

#if 0
  printf("project:\n");
  printf("tF = %f\n", tF);
  printf("mF = %f\n", mF);
  printf("k0 = %f\n", k0);
  printf("rho = %f\n", rho);
#endif
  double dE;
  this->ellipsoid.getHemisphere() == 'N' ?
    dE = rho * sin(lambda) :
    dE = rho * sin(lambda - lambda0);

  *easting = dE + FE;

  if (this->ellipsoid.getHemisphere() == 'N') {
    *northing = FN - rho * cos(lambda - lambda0);
  }
  else {
    double dN = rho * cos(lambda - lambda0);
    *northing = dN + FN;
  }
}

void
SbPolarStereographic::unproject(const double easting,
                                const double northing,
                                SbGeoAngle * phi,
                                SbGeoAngle * lambda) const
{
  double a = this->ellipsoid.getA();
  double e = this->ellipsoid.getE();

  SbGeoAngle phiF = this->ellipsoid.getLatStdParallel();
  SbGeoAngle lambda0 = this->ellipsoid.getLongOrigin();

  double e2 = e * e;
  double e4 = e2 * e2;
  double e6 = e4 * e2;
  double e8 = e4 * e4;

  double rho = pow(pow(easting - this->FE, 2.0) + pow(northing - this->FN, 2), 0.5);
  double t = rho * (pow(pow(1.0 + e, 1.0 + e) * pow(1.0 - e, 1.0 - e), 0.5)) / (2.0 * a * this->k0);

  double X =
    (this->ellipsoid.getHemisphere() == 'N') ?
    (M_PI / 2.0) - (2.0 * atan(t)) :
    2.0 * atan2(t, 1.0) - M_PI / 2.0;

#if 0
  printf("unproject:\n");
  printf("rho = %f\n", rho);
  printf("t = %f\n", t);
  printf("X = %f\n", X);
#endif

  *phi = X + (e2 / 2.0 + (5.0 * e4) / 24.0 + e6 / 12.0 + (13.0 * e8) / 360) * sin(2 * X)
    + ((7 * e4) / 48 + (29 * e6) / 240 + (811 * e8) / 11520) * sin(4 * X)
    + ((7 * e6) / 120 + (81 * e8) / 1120) * sin(6 * X) + ((4279 * e8) / 161280) * sin(8 * X);


  this->ellipsoid.getHemisphere() == 'N' ?
    *lambda = lambda0 + atan2(easting - FE, FN - northing) :
    *lambda = lambda0 + atan2(easting - FE, northing - FN);

#if 0
  // just for testing project()
  double pe, pn;
  this->project(*phi, *lambda, &pe, &pn);
  fprintf(stderr,"projected back: %.1f %.1f (%.1f %.1f)\n", pe, pn, easting, northing);
#endif
}
