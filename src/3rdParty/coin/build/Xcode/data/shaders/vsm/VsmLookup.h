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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

#ifndef SO_VSMLOOKUP_GLSL_H
#define SO_VSMLOOKUP_GLSL_H

static const char VSMLOOKUP_shadersource[] =
  "float VsmLookup(in vec4 map, in float dist, in float epsilon, float bleedthreshold)\n"
  "{\n"
  "  float mapdist = map.x;\n"
  "\n"
  "  // replace 0.0 with some factor > 0.0 to make the light affect even parts in shadow\n"
  "  float lit_factor = dist <= mapdist ? 1.0 : 0.0;\n"
  "  float E_x2 = map.y;\n"
  "  float Ex_2 = mapdist * mapdist;\n"
  "  float variance = min(max(E_x2 - Ex_2, 0.0) + epsilon, 1.0);\n"
  "\n"
  "  float m_d = mapdist - dist;\n"
  "  float p_max = variance / (variance + m_d * m_d);\n"
  "\n"
  "  p_max *= smoothstep(bleedthreshold, 1.0, p_max);\n"
  "\n"
  "  return max(lit_factor, p_max);\n"
  "}\n";

#endif /* ! SO_VSMLOOKUP_GLSL_H */
