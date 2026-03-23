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

#include "SbGeoEllipsoid.h"
#include <Inventor/SbString.h>
#include <cstdio>

SbGeoEllipsoid::SbGeoEllipsoid(void)
{
  this->a = 0.0;
  this->e = 0.0;
}
  // constructor
SbGeoEllipsoid::SbGeoEllipsoid(const SbString & desc, char hemisphere)
{
  if (desc == "WGS_1984" || desc == "WGS84" || desc == "WGS1984UTM") {
    this->a = 6378137.0;
    this->e = 0.081819191;
    this->eccsquared = 0.00669438;
  }
  else {
    fprintf(stderr,"Unsupported ellipsoid: %s\n",
            desc.getString());
    this->a = 6378137.0;
    this->e = 0.081819191;
    this->eccsquared = 0.00669438;
  }

  assert(hemisphere == 'N' || hemisphere == 'S');
  this->hemisphere = hemisphere;

  // polar stereographic
  if (this->hemisphere == 'N') {
    this->lambda0.setDegree(0);   // Longitude of origin
    this->phiF.setDegree(75);     // Latitude of standard parallel
  }
  else {
    this->lambda0.setDegree(180);   // Longitude of origin
    this->phiF.setDegree(-75);      // Latitude of standard parallel
  }
}

  // copy constructor
SbGeoEllipsoid::SbGeoEllipsoid(const SbGeoEllipsoid & e)
{
  *this = e;
}

SbGeoEllipsoid &
SbGeoEllipsoid::operator=(const SbGeoEllipsoid & e)
{
  this->e = e.e;
  this->a = e.a;
  this->eccsquared = e.eccsquared;
  this->hemisphere = e.hemisphere;
  this->lambda0 = e.lambda0;
  this->phiF = e.phiF;
  return *this;
}

double
SbGeoEllipsoid::getA(void) const
{
  return this->a;
}

double
SbGeoEllipsoid::getE(void) const
{
  return this->e;
}

double
SbGeoEllipsoid::getEccentricitySquared(void) const
{
  return this->eccsquared;
}

char
SbGeoEllipsoid::getHemisphere(void) const
{
  return this->hemisphere;
}

const SbGeoAngle &
SbGeoEllipsoid::getLatStdParallel(void) const
{
  return this->phiF;
}

const SbGeoAngle &
SbGeoEllipsoid::getLongOrigin(void) const
{
  return this->lambda0;
}
