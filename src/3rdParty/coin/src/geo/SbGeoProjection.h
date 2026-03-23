#ifndef COIN_SBGEOPROJECTION_H
#define COIN_SBGEOPROJECTION_H

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

#include "SbGeoAngle.h"
#include "SbGeoEllipsoid.h"

#include <Inventor/SbBasic.h>

class SbGeoProjection {
public:
  SbGeoProjection(const SbGeoEllipsoid & ellipsoid,
                  double FE = 0.0, double FN = 0.0);
  virtual ~SbGeoProjection();

  virtual SbBool isUTMProjection(void) const;

  virtual void project(const SbGeoAngle & phi,    // latitude
                       const SbGeoAngle & lambda, // longitude
                       double * easting,
                       double * northing) const = 0;

  virtual void unproject(const double easting,
                         const double northing,
                         SbGeoAngle * phi,
                         SbGeoAngle * lambda) const = 0;

  // convenience methods
  void project(const double latdeg,
               const double latmin,
               const double latsec,
               const double lngdeg,
               const double lngmin,
               const double lngsec,
               double & easting,
               double & northing) {
    SbGeoAngle lat(latdeg, latmin, latsec);
    SbGeoAngle lng(lngdeg, lngmin, lngsec);
    this->project(lat, lng, &easting, & northing);
  }

  void project(const double lat,
               const double lng,
               double & easting,
               double & northing) {
    SbGeoAngle latangle(lat, 0.0, 0.0);
    SbGeoAngle lngangle(lng, 0.0, 0.0);
    this->project(latangle, lngangle, &easting, & northing);
  }

  void unproject(const double easting,
                 const double northing,
                 double & latdeg,
                 double & latmin,
                 double & latsec,
                 double & lngdeg,
                 double & lngmin,
                 double & lngsec) {
    SbGeoAngle lat, lng;
    this->unproject(easting, northing, &lat, &lng);
    latdeg = lat.deg();
    latmin = lat.minutes();
    latsec = lat.seconds();

    lngdeg = lng.deg();
    lngmin = lng.minutes();
    lngsec = lng.seconds();
  }

  void unproject(const double easting,
                 const double northing,
                 double & latdeg,
                 double & lngdeg)
  {
    SbGeoAngle lat, lng;
    this->unproject(easting, northing, &lat, &lng);

    latdeg = lat.deg() + lat.minutes() / 60.0 + lat.seconds() / 3600.0;
    lngdeg = lng.deg() + lng.minutes() / 60.0 + lng.seconds() / 3600.0;
  }
protected:
  SbGeoEllipsoid ellipsoid;
  double FE;       // false easting
  double FN;       // false northing
};

#endif // COIN_SBGEOPROJECTION_H
