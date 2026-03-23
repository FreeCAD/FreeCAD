#ifndef COIN_SBGEOANGLE_H
#define COIN_SBGEOANGLE_H

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

class SbGeoAngle {
public:

  SbGeoAngle(const double d = 0.0);
  SbGeoAngle(const double deg,
             const double min,
             const double sec,
             const char direction = 'N');

  SbGeoAngle(const SbGeoAngle & ang);

  SbGeoAngle & operator=(const double d);
  SbGeoAngle & operator=(SbGeoAngle a);

  void setDegree(const int d);

  double rad(void) const;
  int    deg(void) const;
  int    minutes(void) const;
  double seconds(void) const;

  operator double() const { return this->rad(); }

  SbGeoAngle & operator+=(const SbGeoAngle & a);
  SbGeoAngle & operator-=(const SbGeoAngle & a);
  SbGeoAngle operator-() const;
  SbGeoAngle operator+(SbGeoAngle a) const;
  SbGeoAngle operator-(SbGeoAngle a) const;
  SbGeoAngle operator+(double d) const;
  SbGeoAngle operator-(double d) const;
  friend SbGeoAngle operator+(double d, SbGeoAngle a);
  friend SbGeoAngle operator-(double d, SbGeoAngle a);

  SbGeoAngle & operator*=(double d);
  SbGeoAngle operator*(double d) const;
  friend SbGeoAngle operator*(double d, SbGeoAngle a);

  SbGeoAngle & operator/=(double d);
  SbGeoAngle operator/(double d) const;

  bool operator==(const SbGeoAngle & p) const;
  bool operator!=(const SbGeoAngle & a) const;

  bool operator<(const SbGeoAngle & v) const;
  bool operator>(const SbGeoAngle & v) const;
  bool operator<=(const SbGeoAngle & v) const;
  bool operator>=(const SbGeoAngle & v) const;

private:

  double degrees(void) const;
  double a;
};

#endif // ANGLE_H
