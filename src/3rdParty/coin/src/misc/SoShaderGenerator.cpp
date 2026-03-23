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
  \class SoShaderGenerator SoShaderGenerator.h /misc/SoShaderGenerator.h
  \brief The SoShaderGenerator class is used for simplifying the process of generating shader scripts.

  Currently only GLSL scripts are supported.

  \ingroup coin_shaders
*/

#include "misc/SoShaderGenerator.h"

#include <Inventor/SbName.h>
#include <Inventor/errors/SoDebugError.h>

#include "shaders/SoShader.h"

/*!
  Constructor.
*/
SoShaderGenerator::SoShaderGenerator(void)
  : dirty(FALSE)
{
}

/*!
  Destructor.
*/
SoShaderGenerator::~SoShaderGenerator()
{
}

void 
SoShaderGenerator::reset(const SbBool freeoldstrings)
{
  this->version.makeEmpty(freeoldstrings);
  this->defines.makeEmpty(freeoldstrings);
  this->declarations.makeEmpty(freeoldstrings);
  this->functions.makeEmpty(freeoldstrings);
  this->main.makeEmpty(freeoldstrings);
  this->combined.makeEmpty(freeoldstrings);
  this->dirty = FALSE;
}


void 
SoShaderGenerator::setVersion(const SbString & str)
{
  this->version = str;
  this->version += "\n";
}

/*!
  Adds a define to the shader program.
*/
void 
SoShaderGenerator::addDefine(const SbString & str, const SbBool checkexists)
{
  if (!checkexists || (this->defines.find(str) < 0)) {
    this->dirty = TRUE;
    this->defines += str;
    this->defines += "\n";
  }
}

/*!
  Adds a declaration (varying or uniform) to the script.
*/
void 
SoShaderGenerator::addDeclaration(const SbString & str, const SbBool checkexists)
{
  if (!checkexists || (this->declarations.find(str) < 0)) {
    this->dirty = TRUE;
    this->declarations += str;
    this->declarations += "\n";
  }
}

/*!
  Adds a function to the script.
*/
void 
SoShaderGenerator::addFunction(const SbString & str, const SbBool checkexists)
{
  if (!checkexists || (this->functions.find(str) < 0)) {
    this->dirty = TRUE;
    this->functions += str;
    this->functions += "\n";
  }
}

/*!
  Adds a named function to the script.
*/
void 
SoShaderGenerator::addNamedFunction(const SbName & name, const SbBool checkexists)
{
  const char * func = SoShader::getNamedScript(name, SoShader::GLSL_SHADER);
  
  if (func) {
    this->addFunction(SbString(func), checkexists);
  }
  else {
    SoDebugError::postWarning("SoShaderGenerator::addNamedFunction",
                              "Unknown named script: %s",
                              name.getString());
  }
}

/*!
  Add a statement to the main function.
*/
void 
SoShaderGenerator::addMainStatement(const SbString & str)
{
  this->dirty = TRUE;
  this->main += str;
  this->main += "\n";
}

/*!
  Returns the complete shader program.
*/
const SbString &
SoShaderGenerator::getShaderProgram(void)
{
  if (this->dirty) {
    this->combined.makeEmpty(FALSE);
    this->combined += this->version;
    this->combined += this->defines;
  
    this->combined += this->declarations;
    this->combined += this->functions;
    this->combined += SbString("void main(void) {\n");
    this->combined += this->main;
    this->combined += "}\n";
  }
  return this->combined;
}
