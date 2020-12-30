#ifndef SKETCHER_POINT_POSITION_H
#define SKETCHER_POINT_POSITION_H


namespace Sketcher
{
    /// define if you want to use the end or start point. These end up in the save file, so changing the values breaks stuff
    enum PointPos  { none=0, edge=0, start=1, end=2, mid=3, size=4 };
}

#endif //SKETCHER_POINT_POSITION_H