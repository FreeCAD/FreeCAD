// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MillPathLine.h"

#include "Shader.h"

// include this last as the defines can mess up other includes
#include "OpenGlWrapper.h"

namespace MillSim
{

void MillPathLine::GenerateModel()
{
    mNumVerts = MillPathPointsBuffer.size();
    void* vbuffer = MillPathPointsBuffer.data();

    // vertex buffer
    glGenBuffers(1, &mVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glBufferData(GL_ARRAY_BUFFER, mNumVerts * sizeof(MillPathPosition), vbuffer, GL_STATIC_DRAW);

    // free
    MillPathPointsBuffer.clear();
}

void MillPathLine::SetupVertexAttibs()
{
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(MillPathPosition),
        (void*)offsetof(MillPathPosition, X)
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        1,
        GL_INT,
        GL_FALSE,
        sizeof(MillPathPosition),
        (void*)offsetof(MillPathPosition, SegmentId)
    );
}

void MillPathLine::Clear()
{
    MillPathPointsBuffer.clear();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GLDELETE_BUFFER(mVbo);
}

void MillPathLine::Render()
{
    SetupVertexAttibs();
    glDrawArrays(GL_LINE_STRIP, 0, mNumVerts);
}

}  // namespace MillSim
