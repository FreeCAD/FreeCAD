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

/*!
  \class SoMFColor SoMFColor.h Inventor/fields/SoMFColor.h
  \brief The SoMFColor class is a container for SbColor values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store multiple color values (i.e. "Red Green Blue"
  triplets).

  This field supports application data sharing through a
  setValuesPointer() method. See SoMField documentation for
  information on how to use this function.

  \sa SbColor, SoSFColor

*/

// *************************************************************************

#include <Inventor/fields/SoMFColor.h>

#include <cassert>

#include <Inventor/SoInput.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFColor, SbColor, const SbColor &);

SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFColor, SbColor, float);
SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFColor, SbColor, SbColor);

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFColor::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFColor);
}


// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFColor::read1Value(SoInput * in, int idx)
{
  assert(idx < this->maxNum);
  return
    in->read(this->values[idx][0]) &&
    in->read(this->values[idx][1]) &&
    in->read(this->values[idx][2]);
}

void
SoMFColor::write1Value(SoOutput * out, int idx) const
{
  sosfvec3f_write_value(out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS


/*!
  Set \a num RGB color values, starting at index \a start.
*/
void
SoMFColor::setValues(int start, int numarg, const float rgb[][3])
{
  if(start+numarg > this->maxNum) this->makeRoom(start+numarg);
  else if(start+numarg > this->num) this->num = start+numarg;

  for(int i=0; i < numarg; i++) this->values[i+start].setValue(rgb[i]);
  this->valueChanged();
}

/*!
  Set \a num HSV color values, starting at index \a start.
*/
void
SoMFColor::setHSVValues(int start, int numarg, const float hsv[][3])
{
  if(start+numarg > this->maxNum) this->makeRoom(start+numarg);
  else if(start+numarg > this->num) this->num = start+numarg;

  for(int i=0; i < numarg; i++) this->values[i+start].setHSVValue(hsv[i]);
  this->valueChanged();
}

/*!
  Set the color array to a single value. \a vec is interpreted as
  a three element vector with the red, green and blue components,
  respectively.
*/
void
SoMFColor::setValue(const SbVec3f & vec)
{
  this->setValue(vec[0], vec[1], vec[2]);
}

/*!
  Set the color array to a single value. \a r, \a g and \a b are the
  red, green and blue components, respectively.
*/
void
SoMFColor::setValue(float r, float g, float b)
{
  this->setValue(SbColor(r, g, b));
}

/*!
  Set the color array to a single value. \a rgb is a three element
  vector with the red, green and blue components, respectively.
*/
void
SoMFColor::setValue(const float rgb[3])
{
  this->setValue(SbColor(rgb));
}

/*!
  Set the color array to a single value. \a h, \a s and \a v are the
  hue, saturation and value components, respectively.
*/
void
SoMFColor::setHSVValue(float h, float s, float v)
{
  SbColor col;
  col.setHSVValue(h, s, v);
  this->setValue(col);
}

/*!
  Set the color array to a single value. \a hsv is a three element
  vector with the hue, saturation and value components, respectively.
*/
void
SoMFColor::setHSVValue(const float hsv[3])
{
  this->setHSVValue(hsv[0], hsv[1], hsv[2]);
}

/*!
  Set the color at \a idx. \a vec is interpreted as a three element
  vector with the red, green and blue components, respectively.
*/
void
SoMFColor::set1Value(int idx, const SbVec3f & vec)
{
  this->set1Value(idx, SbColor(vec));
}

/*!
  Set the color at \a idx. \a r, \a g and \a b is the red, green and
  blue components, respectively.
*/
void
SoMFColor::set1Value(int idx, float r, float g, float b)
{
  this->set1Value(idx, SbColor(r, g, b));
}

/*!
  Set the color at \a idx. \a rgb is interpreted as a three element
  vector with the red, green and blue components, respectively.
*/
void
SoMFColor::set1Value(int idx, const float rgb[3])
{
  this->set1Value(idx, SbColor(rgb));
}

/*!
  Set the color at \a idx. \a h, \a s and \a v is the hue, saturation and
  value components, respectively.
*/
void
SoMFColor::set1HSVValue(int idx, float h, float s, float v)
{
  SbColor col;
  col.setHSVValue(h, s, v);
  this->set1Value(idx, col);
}

/*!
  Set the color at \a idx. \a hsv is a three element vector with the
  hue, saturation and value components, respectively.
*/
void
SoMFColor::set1HSVValue(int idx, const float hsv[3])
{
  this->set1HSVValue(idx, hsv[0], hsv[1], hsv[2]);
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFColor field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
