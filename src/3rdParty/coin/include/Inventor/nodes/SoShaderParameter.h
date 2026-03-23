#ifndef COIN_SOSHADERPARAMETER_H
#define COIN_SOSHADERPARAMETER_H

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

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFVec4f.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoMFVec2f.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFVec4f.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoMFMatrix.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFImage.h>

#include <Inventor/fields/SoSFVec2i32.h>
#include <Inventor/fields/SoSFVec3i32.h>
#include <Inventor/fields/SoSFVec4i32.h>
#include <Inventor/fields/SoMFVec2i32.h>
#include <Inventor/fields/SoMFVec3i32.h>
#include <Inventor/fields/SoMFVec4i32.h>

class SoGLShaderParameter;
class SoGLShaderObject;
class SoState;

// *************************************************************************

// FIXME: split to one header file for each class? 20050120 mortene.

/* **************************************************************************
 * *** SoShaderParameter ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter : public SoNode {
  typedef SoNode inherited;
  SO_NODE_ABSTRACT_HEADER(SoShaderParameter);

public:
  SoSFString name;
  SoSFInt32 identifier;

  SoINTERNAL public:
  static void initClass();

protected:
  SoShaderParameter();
  virtual ~SoShaderParameter();
};

/* **************************************************************************
 * *** SoUniformShaderParameter ***
 * **************************************************************************/
class SoUniformShaderParameterP;

class COIN_DLL_API SoUniformShaderParameter : public SoShaderParameter {
  typedef SoShaderParameter inherited;
  SO_NODE_ABSTRACT_HEADER(SoUniformShaderParameter);

  SoINTERNAL public:
  static void initClass(void);
  virtual void updateParameter(SoGLShaderObject *shaderObject) = 0;

protected:
  SoUniformShaderParameter();
  virtual ~SoUniformShaderParameter();

  SoGLShaderParameter * getGLShaderParameter(const uint32_t cachecontext);
  void ensureParameter(SoGLShaderObject * shader);

private:
  SoUniformShaderParameterP * pimpl;
};

/* **************************************************************************
 * *** SoShaderParameter1f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter1f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter1f);

public:
  SoSFFloat value;
  SoShaderParameter1f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter1f();
};

/* **************************************************************************
 * *** SoShaderParameter1i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter1i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter1i);

public:
  SoSFInt32 value;
  SoShaderParameter1i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter1i();
};

/* **************************************************************************
 * *** SoShaderParameter2f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter2f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter2f);

public:
  SoSFVec2f value;
  SoShaderParameter2f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter2f();
};

/* **************************************************************************
 * *** SoShaderParameter2i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter2i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter2i);

public:
  SoSFVec2i32 value;
  SoShaderParameter2i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter2i();
};

/* **************************************************************************
 * *** SoShaderParameter3f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter3f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter3f);

public:
  SoSFVec3f value;

  SoShaderParameter3f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter3f();
};

/* **************************************************************************
 * *** SoShaderParameter3i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter3i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter3i);

public:
  SoSFVec3i32 value;

  SoShaderParameter3i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter3i();
};

/* **************************************************************************
 * *** SoShaderParameter4f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter4f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter4f);

public:
  SoSFVec4f value;

  SoShaderParameter4f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter4f();
};

/* **************************************************************************
 * *** SoShaderParameter4i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameter4i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameter4i);

public:
  SoSFVec4i32 value;

  SoShaderParameter4i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameter4i();
};

/* **************************************************************************
 * *** SoShaderParameterArray1f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray1f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray1f);

public:
  SoMFFloat value;

  SoShaderParameterArray1f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray1f();
};

/* **************************************************************************
 * *** SoShaderParameterArray1i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray1i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray1i);

public:
  SoMFInt32 value;

  SoShaderParameterArray1i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray1i();
};

/* **************************************************************************
 * *** SoShaderParameterArray2f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray2f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray2f);

public:
  SoMFVec2f value;

  SoShaderParameterArray2f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray2f();
};

/* **************************************************************************
 * *** SoShaderParameterArray2i ***
 * **************************************************************************/
class COIN_DLL_API SoShaderParameterArray2i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray2i);

public:
  SoMFVec2i32 value;

  SoShaderParameterArray2i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray2i();
};

/* **************************************************************************
 * *** SoShaderParameterArray3f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray3f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray3f);

public:
  SoMFVec3f value;

  SoShaderParameterArray3f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray3f();
};

/* **************************************************************************
 * *** SoShaderParameterArray3i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray3i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray3i);

public:
  SoMFVec3i32 value;

  SoShaderParameterArray3i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray3i();
};

/* **************************************************************************
 * *** SoShaderParameterArray4f ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray4f : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray4f);

public:
  SoMFVec4f value;

  SoShaderParameterArray4f();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray4f();

};

/* **************************************************************************
 * *** SoShaderParameterArray4i ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterArray4i : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterArray4i);

public:
  SoMFVec4i32 value;

  SoShaderParameterArray4i();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterArray4i();
};

/* **************************************************************************
 * *** SoShaderParameterMatrix ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterMatrix : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterMatrix);

public:
  SoSFMatrix value;

  SoShaderParameterMatrix();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterMatrix();
};

/* **************************************************************************
 * *** SoShaderParameterMatrixArray ***
 * **************************************************************************/

class COIN_DLL_API SoShaderParameterMatrixArray : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderParameterMatrixArray);

public:
  SoMFMatrix value;

  SoShaderParameterMatrixArray();

  SoINTERNAL public:
  static void initClass();
  virtual void updateParameter(SoGLShaderObject *shaderObject);

protected:
  virtual ~SoShaderParameterMatrixArray();
};

/* **************************************************************************
 * *** SoShaderStateMatrixParameter ***
 * **************************************************************************/

class COIN_DLL_API SoShaderStateMatrixParameter : public SoUniformShaderParameter {
  typedef SoUniformShaderParameter inherited;
  SO_NODE_HEADER(SoShaderStateMatrixParameter);

public:

  enum MatrixType {
    MODELVIEW,
    PROJECTION,
    TEXTURE,
    MODELVIEW_PROJECTION
  };

  enum MatrixTransform{
    IDENTITY,
    TRANSPOSE,
    INVERSE,
    INVERSE_TRANSPOSE
  };

  SoSFEnum matrixType;
  SoSFEnum matrixTransform;

  SoShaderStateMatrixParameter();
  virtual ~SoShaderStateMatrixParameter();

  static void initClass(void);
  virtual void updateParameter(SoGLShaderObject *shaderObject);
  virtual void updateValue(SoState *state);

protected:
  // Unlike in other parameter classes, here value is not a field because 
  // it is updated dynamically from the state.
  SbMatrix value;
};

#endif /* ! COIN_SOSHADERPARAMETER_H */
