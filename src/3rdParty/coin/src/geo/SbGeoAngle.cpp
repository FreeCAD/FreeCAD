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
#include <Inventor/SbBasic.h>
#include <cmath>
#include <cassert>

#define ANGLE_TOLERANCE 1e-5

/*
  Default constructor radians
*/

SbGeoAngle::SbGeoAngle(const double d)
{
  this->a = d;
}

/*
  Default constructor degrees
*/

SbGeoAngle::SbGeoAngle(const double deg,
                       const double min,
                       const double sec,
                       const char direction)
{
  assert(((direction == 'N') ||
         (direction == 'S') ||
         (direction == 'E') ||
         (direction == 'W')) &&
         "direction must be either N, S, E or W");

  this->a = deg * M_PI / 180.0;
  this->a += (min / 60.0) * M_PI / 180;
  this->a += (sec / 3600.0) * M_PI / 180;
  if (direction == 'S') this->a = -this->a;
}

/*
  Copy  constructor
*/

SbGeoAngle::SbGeoAngle(const SbGeoAngle & a)
{
  this->a = a.a;
}

void
SbGeoAngle::setDegree(const int deg)
{
  this->a = deg * M_PI / 180.0;
}

/*
  Operators
 */
SbGeoAngle &
SbGeoAngle::operator=(const double d)
{
  this->a = d;
  return *this;
}

SbGeoAngle &
SbGeoAngle::operator=(SbGeoAngle ang)
{
  this->a = ang.a;
  return *this;
}

double
SbGeoAngle::rad(void) const
{
  return this->a;
}

int
SbGeoAngle::deg(void) const
{
  return int(this->degrees());
}

SbGeoAngle &
SbGeoAngle::operator+=(const SbGeoAngle & a)
{
  this->a += a.a;
  return *this;
}

 SbGeoAngle &
SbGeoAngle::operator-=(const SbGeoAngle & a)
{
  this->a -= a.a;
  return *this;
}

 SbGeoAngle
SbGeoAngle::operator-() const
{
  SbGeoAngle p;
  p.a = -this->a;
  return p;
}

 SbGeoAngle
SbGeoAngle::operator+(SbGeoAngle a) const
{
  a += *this;
  return a;
}

 SbGeoAngle
SbGeoAngle::operator-(SbGeoAngle a) const
{
  a -= *this;
  return -a;
}

SbGeoAngle
SbGeoAngle::operator+(double d) const
{
  return *this + SbGeoAngle(d);
}

SbGeoAngle
SbGeoAngle::operator-(double d) const
{
  return *this - SbGeoAngle(d);
}


SbGeoAngle &
SbGeoAngle::operator*=(double d)
{
  this->a *= d;
  return *this;
}

SbGeoAngle
SbGeoAngle::operator*(double d) const
{
  return SbGeoAngle(a*d);
}

SbGeoAngle
operator*(double d, SbGeoAngle a)
{
  a *= d;
  return a;
}

SbGeoAngle
operator+(double d, SbGeoAngle a)
{
  a += d;
  return a;
}

SbGeoAngle
operator-(double d, SbGeoAngle a)
{
  a -= d;
  return a;
}

SbGeoAngle &
SbGeoAngle::operator/=(double d)
{
  d = 1/d;
  *this *= d;
  return *this;
}

 SbGeoAngle
SbGeoAngle::operator/(double d) const
{
  d = 1/d;
  return *this * d;
}

 bool
SbGeoAngle::operator==(const SbGeoAngle & p) const
{
  return fabs(a - p.a) < ANGLE_TOLERANCE;
}

 bool
SbGeoAngle::operator!=(const SbGeoAngle & a) const
{
  return !(*this == a);
}

 bool
SbGeoAngle::operator<(const SbGeoAngle & v) const
{
  return this->a < v.a;
}


 bool
SbGeoAngle::operator>(const SbGeoAngle & v) const
{
  return this->a > v.a;
}

 bool
SbGeoAngle::operator<=(const SbGeoAngle & v) const
{
  return !(*this > v);
}

bool
SbGeoAngle::operator>=(const SbGeoAngle & v) const
{
  return !(*this < v);
}

int
SbGeoAngle::minutes() const
{
  double tmp = this->degrees();
  tmp -= (double) this->deg();

  return (int) (tmp * 60.0);
}

double
SbGeoAngle::seconds() const
{
  double tmp = this->degrees();
  tmp -= this->deg() + this->minutes() / 60.0;

  return tmp * 60.0 * 60.0;
}

double
SbGeoAngle::degrees(void) const
{
  double tmp = fmod(180.0 * (this->a / M_PI), 360.0);
  if (tmp > 180.0) {
    return tmp - 360.0;
  }
  return tmp;
}

#undef ANGLE_TOLERANCE
