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

#ifndef COIN_SOBOOLOPERATION_H
#define COIN_SOBOOLOPERATION_H

#include <Inventor/engines/SoSubEngine.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoMFEnum.h>


class COIN_DLL_API SoBoolOperation : public SoEngine {
  typedef SoEngine inherited;

  SO_ENGINE_HEADER(SoBoolOperation);

public:
  enum Operation {
    CLEAR, SET,
    A, NOT_A,
    B, NOT_B,
    A_OR_B, NOT_A_OR_B, A_OR_NOT_B, NOT_A_OR_NOT_B,
    A_AND_B, NOT_A_AND_B, A_AND_NOT_B, NOT_A_AND_NOT_B,
    A_EQUALS_B, A_NOT_EQUALS_B
  };

  SoMFBool a;
  SoMFBool b;
  SoMFEnum operation;

  SoEngineOutput output;  //SoMFBool
  SoEngineOutput inverse; //SoMFBool

  SoBoolOperation();

  static void initClass();

protected:
  virtual ~SoBoolOperation(void);

private:
  virtual void evaluate();
};

#endif // !COIN_SOBOOLOPERATION_H
