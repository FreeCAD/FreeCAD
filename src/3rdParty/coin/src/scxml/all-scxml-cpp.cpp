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

// Utils
#include "SbStringConvert.cpp"

// Infrastructure
#include "ScXML.cpp"
#include "ScXMLP.cpp"
#include "ScXMLObject.cpp"
#include "ScXMLEvent.cpp"
#include "ScXMLEventTarget.cpp"
#include "ScXMLStateMachine.cpp"
#include "ScXMLDocument.cpp"

// Core Module
#include "ScXMLElt.cpp"
#include "ScXMLScxmlElt.cpp"
#include "ScXMLInitialElt.cpp"
#include "ScXMLAbstractStateElt.cpp"
#include "ScXMLStateElt.cpp"
#include "ScXMLParallelElt.cpp"
#include "ScXMLFinalElt.cpp"
#include "ScXMLOnEntryElt.cpp"
#include "ScXMLOnExitElt.cpp"
#include "ScXMLTransitionElt.cpp"
#include "ScXMLHistoryElt.cpp"

// Executable Content
#include "ScXMLExecutableElt.cpp"
#include "ScXMLEventElt.cpp"
#include "ScXMLIfElt.cpp"
#include "ScXMLElseIfElt.cpp"
#include "ScXMLElseElt.cpp"
#include "ScXMLLogElt.cpp"

// External Communication Module
#include "ScXMLSendElt.cpp"
#include "ScXMLInvokeElt.cpp"
#include "ScXMLParamElt.cpp"
#include "ScXMLFinalizeElt.cpp"
#include "ScXMLContentElt.cpp"

// Data Module
#include "ScXMLDataElt.cpp"
#include "ScXMLDataModelElt.cpp"
#include "ScXMLAssignElt.cpp"
#include "ScXMLValidateElt.cpp"

// Script Module
#include "ScXMLScriptElt.cpp"

// Anchor Module
#include "ScXMLAnchorElt.cpp"

// Expression Evaluation
#include "ScXMLEvaluator.cpp"

#include "ScXMLMinimumEvaluator.cpp"
#include "eval-minimum-tab.cpp"
#include "eval-minimum.cpp"
#include "ScXMLXPathEvaluator.cpp"
#include "ScXMLECMAScriptEvaluator.cpp"

