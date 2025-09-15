#ifndef __millpathline_h__
#define __millpathline_h__
#include <vector>

namespace CAMSimulator
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

}  // namespace CAMSimulator

#endif  // !__millpathline_h__
