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

// FIXME: for some odd reason, order is important between these
// two. if SoBase.cpp is compiled before SoDebug.cpp, we get a weird
// error from the SbHash usage in SoDebug.cpp. investigate.  -mortene.

// This is caused by the SbHashFunc(void *) being declared only when
// used. Hashing on (void *) is extremely dangerous, since we don't
// have any control over which kind of types are hashed this way.
// Indeed the very occurrence of this problem is a symptom of the
// problems of hashing on (void *). Since all the implementations for
// (class *) are identical regardless of class, there shouldn't arise
// any problems from doing this, but if we change the implementations,
// we should make very sure that the correct function is picked as the
// hash function when doing inclusion like this. - BFG 20081117

#include "SoDebug.cpp"
#include "SoBase.cpp"

#include "AudioTools.cpp"
#include "CoinResources.cpp"
#include "CoinStaticObjectInDLL.cpp"
#include "SoAudioDevice.cpp"
#include "SoBaseP.cpp"
#include "SoChildList.cpp"
#include "SoCompactPathList.cpp"
#include "SoConfigSettings.cpp"
#include "SoContextHandler.cpp"
#include "SoDB.cpp"
#include "SoDBP.cpp"
#include "SoEventManager.cpp"
#include "SoFullPath.cpp"
#include "SoGenerate.cpp"
#include "SoGlyph.cpp"
#include "SoInteraction.cpp"
#include "SoJavaScriptEngine.cpp"
#include "SoLightPath.cpp"
#include "SoLockManager.cpp"
#include "SoNormalGenerator.cpp"
#include "SoNotRec.cpp"
#include "SoNotification.cpp"
#include "SoPath.cpp"
#include "SoPick.cpp"
#include "SoPickedPoint.cpp"
#include "SoPrimitiveVertex.cpp"
#include "SoProto.cpp"
#include "SoProtoInstance.cpp"
#include "SoSceneManager.cpp"
#include "SoSceneManagerP.cpp"
#include "SoShaderGenerator.cpp"
#include "SoState.cpp"
#include "SoTempPath.cpp"
#include "SoType.cpp"
