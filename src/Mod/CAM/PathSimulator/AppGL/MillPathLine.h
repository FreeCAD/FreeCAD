// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#ifndef __millpathline_h__
#define __millpathline_h__
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
    MillPathLine();
    void GenerateModel();
    void Clear();
    void Render();

public:
    std::vector<MillPathPosition> MillPathPointsBuffer;

protected:
    unsigned int mVbo;
    unsigned int mVao;
    int mNumVerts;
};

}  // namespace MillSim

#endif  // !__millpathline_h__
