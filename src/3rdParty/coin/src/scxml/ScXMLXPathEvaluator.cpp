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

#include <Inventor/scxml/ScXMLXPathEvaluator.h>
#include "coindefs.h"

/*!
  \class ScXMLXPathEvaluator ScXMLXPathEvaluator.h Inventor/scxml/ScXMLXPathEvaluator.h
  \brief implements the XPath evaluator.

  \ingroup coin_scxml
*/

#include <cassert>

class ScXMLXPathEvaluator::PImpl {
public:
};

SCXML_OBJECT_SOURCE(ScXMLXPathEvaluator);

void
ScXMLXPathEvaluator::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLXPathEvaluator, ScXMLEvaluator, "ScXMLEvaluator");
}

void
ScXMLXPathEvaluator::cleanClass(void)
{
  ScXMLXPathEvaluator::classTypeId = SoType::badType();
}

ScXMLXPathEvaluator::ScXMLXPathEvaluator(void)
{
}

ScXMLXPathEvaluator::~ScXMLXPathEvaluator(void)
{
}

ScXMLDataObj *
ScXMLXPathEvaluator::evaluate(const char * COIN_UNUSED_ARG(expression)) const
{
  // FIXME: not implemented
  return NULL;
}

SbBool
ScXMLXPathEvaluator::setAtLocation(const char * COIN_UNUSED_ARG(location), ScXMLDataObj * COIN_UNUSED_ARG(obj))
{
  // FIXME: not implemented
  return FALSE;
}

ScXMLDataObj *
ScXMLXPathEvaluator::locate(const char * COIN_UNUSED_ARG(location)) const
{
  // FIXME: not implemented
  return NULL;
}

