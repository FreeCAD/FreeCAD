#include "MillPathLine.h"
#include "OpenGlWrapper.h"
#include "GlUtils.h"
#include "Shader.h"

namespace MillSim
{


MillPathLine::MillPathLine()
{
    mVao = mVbo = 0;
}

void MillPathLine::GenerateModel()
{
    mNumVerts = MillPathPointsBuffer.size();
    void* vbuffer = MillPathPointsBuffer.data();

    // vertex array
    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);

    // vertex buffer
    glGenBuffers(1, &mVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glBufferData(GL_ARRAY_BUFFER, mNumVerts * sizeof(MillPathPosition), vbuffer, GL_STATIC_DRAW);

    // vertex attribs
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MillPathPosition),
                          (void*)offsetof(MillPathPosition, X));
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1,
                           1,
                           GL_INT,
                           sizeof(MillPathPosition),
                           (void*)offsetof(MillPathPosition, SegmentId));

    // unbind and free
    glBindVertexArray(0);
    MillPathPointsBuffer.clear();
}

void MillPathLine::Clear()
{
    MillPathPointsBuffer.clear();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    GLDELETE_BUFFER(mVbo);
    GLDELETE_VERTEXARRAY(mVao);
}

void MillPathLine::Render()
{
    glBindVertexArray(mVao);
    glDrawArrays(GL_LINE_STRIP, 0, mNumVerts);
}

}  // namespace MillSim
