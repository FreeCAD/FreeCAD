#ifndef COIN_SOVECTORIZEITEMS_H
#define COIN_SOVECTORIZEITEMS_H

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

// Some simple classes to store vectorized items. Inlined and with
// public data members.

#include <Inventor/SbBasic.h>
#include <Inventor/SbName.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>

class SoVectorizeItem {
public:
  SoVectorizeItem() {
    this->type = UNDEFINED;
    this->depth = 0.0f;
  }
  // quick and easy type system
  enum Type {
    UNDEFINED,
    LINE,
    TRIANGLE,
    TEXT,
    POINT,
    IMAGE
  };
  int type;
  float depth; // for depth sorting
};

class SoVectorizePoint : public SoVectorizeItem {
public:
  SoVectorizePoint(void) {
    this->type = POINT;
    this->size = 1.0f;
  }
  int vidx;       // index to BSPtree coordinate
  float size;     // Coin size (pixels)
  uint32_t col;
};

class SoVectorizeTriangle : public SoVectorizeItem {
public:
  SoVectorizeTriangle(void) {
    this->type = TRIANGLE;
  }
  int vidx[3];      // indices to BSPtree coordinates
  uint32_t col[3];
};

class SoVectorizeLine : public SoVectorizeItem {
public:
  SoVectorizeLine(void) {
    this->type = LINE;
    this->pattern = 0xffff;
    this->width = 1.0f;
  }
  int vidx[2];       // indices to BSPtree coordinates
  uint32_t col[2];
  uint16_t pattern;  // Coin line pattern
  float width;       // Coin line width (pixels)
};

class SoVectorizeText : public SoVectorizeItem {
public:
  SoVectorizeText(void) {
    this->type = TEXT;
  }

  enum Justification {
    LEFT,
    RIGHT,
    CENTER
  };

  SbName fontname;
  float fontsize;    // size in normalized coordinates
  SbString string;
  SbVec2f pos;       // pos in normalized coordinates
  uint32_t col;
  Justification justification;
};

class SoVectorizeImage : public SoVectorizeItem {
public:
  SoVectorizeImage(void) {
    this->type = IMAGE;
  }

  SbVec2f pos;        // pos in normalized coordinates
  SbVec2f size;       // size in normalized coordinates

  struct Image {
    const unsigned char * data;
    SbVec2s size;
    int nc;
  } image;
};

#endif // COIN_SOVECTORIZEITEMS_H
