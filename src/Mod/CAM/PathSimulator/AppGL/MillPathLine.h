// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <vector>

namespace MillSim
{

struct MillPathPosition
{
    float X, Y, Z;
    int SegmentId;
};

class MillPathLine
{
public:
    void GenerateModel();
    void SetupVertexAttibs();
    void Clear();
    void Render();

public:
    std::vector<MillPathPosition> MillPathPointsBuffer;

protected:
    unsigned int mVbo = 0;
    int mNumVerts;
};

}  // namespace MillSim
