// SPDX-License-Identifier: BSD-3-Clause

#pragma once

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

#include <Inventor/fields/SoMFBool.h>
#include <Inventor/nodes/SoGroup.h>
#include <FCGlobal.h>


class GuiExport SmSwitchboard: public SoGroup
{
    using inherited = SoGroup;
    SO_NODE_HEADER(SmSwitchboard);

public:
    static void initClass();
    SmSwitchboard();
    SmSwitchboard(int numchildren);

    SoMFBool enable;

    void doAction(SoAction* action) override;
    void callback(SoCallbackAction* action) override;
    void GLRender(SoGLRenderAction* action) override;
    void pick(SoPickAction* action) override;
    void getBoundingBox(SoGetBoundingBoxAction* action) override;
    void handleEvent(SoHandleEventAction* action) override;
    void getMatrix(SoGetMatrixAction* action) override;
    void search(SoSearchAction* action) override;

protected:
    ~SmSwitchboard() override;
};
