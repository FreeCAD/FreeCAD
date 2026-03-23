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

#include "SbUTMProjection.h"
#include <cstdio>

SbUTMProjection::SbUTMProjection(const int utmzone,
                                 const SbGeoEllipsoid & ellipsoid,
                                 double FE, double FN)
  : inherited(ellipsoid, FE, FN),
    forcedutmzone(utmzone)
{
}

SbBool
SbUTMProjection::isUTMProjection(void) const
{
  return TRUE;
}

void
SbUTMProjection::project(const SbGeoAngle & LatRad,
                         const SbGeoAngle & LongRad,
                         double * easting,
                         double * northing) const
{
  double a = this->ellipsoid.getA();
  double eccSquared = this->ellipsoid.getEccentricitySquared();
  double k0 = 0.9996;

  double LongOrigin;
  double eccPrimeSquared;
  double N, T, C, A, M;

  //Make sure the longitude is between -180.00 .. 179.9
  double LongTemp = (int(LongRad.deg())+180)-int((int(LongRad.deg())+180)/360)*360-180; // -180.00 .. 179.9;

  //  double LatRad = Lat*deg2rad;
  //  double LongRad = LongTemp*deg2rad;
  double LongOriginRad;
  int    ZoneNumber;

  ZoneNumber = int((LongTemp + 180)/6) + 1;

  if (this->forcedutmzone != -1) {
    ZoneNumber = this->forcedutmzone;
  }

  const double deg2rad = M_PI / 180;
  //const double rad2deg = 180.0 / M_PI;
  //const double FOURTHPI = M_PI / 4;

  LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone
  LongOriginRad = LongOrigin * deg2rad;

  eccPrimeSquared = (eccSquared)/(1-eccSquared);

  N = a/sqrt(1-eccSquared*sin(LatRad)*sin(LatRad));
  T = tan(LatRad)*tan(LatRad);
  C = eccPrimeSquared*cos(LatRad)*cos(LatRad);
  A = cos(LatRad)*(LongRad-LongOriginRad);

  M = a*((1 - eccSquared/4    - 3*eccSquared*eccSquared/64  - 5*eccSquared*eccSquared*eccSquared/256)*LatRad
         - (3*eccSquared/8 + 3*eccSquared*eccSquared/32  + 45*eccSquared*eccSquared*eccSquared/1024)*sin(2.0*LatRad)
                  + (15*eccSquared*eccSquared/256 + 45*eccSquared*eccSquared*eccSquared/1024)*sin(4.0*LatRad)
                  - (35*eccSquared*eccSquared*eccSquared/3072)*sin(6.0*LatRad));

  *easting = (double)(k0*N*(A+(1-T+C)*A*A*A/6
                           + (5-18*T+T*T+72*C-58*eccPrimeSquared)*A*A*A*A*A/120)
                     + 500000.0);

  *northing = (double)(k0*(M+N*tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
         + (61-58*T+T*T+600*C-330*eccPrimeSquared)*A*A*A*A*A*A/720)));
  if (double(LatRad) < 0.0)
    *northing += 10000000.0; //10000000 meter offset for southern hemisphere
}

void
SbUTMProjection::unproject(const double UTMEasting,
                           const double UTMNorthing,
                           SbGeoAngle * Lat,
                           SbGeoAngle * Long) const
{
  double k0 = 0.9996;
  double a = this->ellipsoid.getA();
  double eccSquared = this->ellipsoid.getEccentricitySquared();
  double eccPrimeSquared;
  double e1 = (1-sqrt(1-eccSquared))/(1+sqrt(1-eccSquared));
  double N1, T1, C1, R1, D, M;
  double LongOrigin;
  double mu,/* phi1,*/ phi1Rad;
  double x, y;
  int ZoneNumber;
  //char* ZoneLetter;
  //int NorthernHemisphere; //1 for northern hemispher, 0 for southern

  x = UTMEasting - 500000.0; //remove 500,000 meter offset for longitude
  y = UTMNorthing;

  ZoneNumber = 34; // default zone
  if (this->forcedutmzone != -1) {
    ZoneNumber = this->forcedutmzone;
  }
  if (this->ellipsoid.getHemisphere() == 'S') {
    y -= 10000000.0; //remove 10,000,000 meter offset used for southern hemisphere
  }

  LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone
  eccPrimeSquared = (eccSquared)/(1-eccSquared);

  M = y / k0;
  mu = M/(a*(1-eccSquared/4-3*eccSquared*eccSquared/64-5*eccSquared*eccSquared*eccSquared/256));

  phi1Rad = mu  + (3*e1/2-27*e1*e1*e1/32)*sin(2*mu)
        + (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
        +(151*e1*e1*e1/96)*sin(6*mu);

  N1 = a/sqrt(1-eccSquared*sin(phi1Rad)*sin(phi1Rad));
  T1 = tan(phi1Rad)*tan(phi1Rad);
  C1 = eccPrimeSquared*cos(phi1Rad)*cos(phi1Rad);
  R1 = a*(1-eccSquared)/pow(1-eccSquared*sin(phi1Rad)*sin(phi1Rad), 1.5);
  D = x/(N1*k0);

  *Lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrimeSquared)*D*D*D*D/24
                                         +(61+90*T1+298*C1+45*T1*T1-252*eccPrimeSquared-3*C1*C1)*D*D*D*D*D*D/720);

  double tmp = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrimeSquared+24*T1*T1)
                *D*D*D*D*D/120)/cos(phi1Rad);

  tmp += LongOrigin * M_PI / 180.0;
  *Long = tmp;
}

void
SbUTMProjection::setUTMZone(const int utmzone)
{
  this->forcedutmzone = utmzone;
}

int
SbUTMProjection::getUTMZone(void) const
{
  return this->forcedutmzone;
}
