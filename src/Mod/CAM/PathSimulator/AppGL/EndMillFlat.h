#ifndef __end_mill_flat_h__
#define __end_mill_flat_h__

#include "EndMill.h"

#define FLAT_MILL_PROFILE_VERTS 3

namespace MillSim
{
    class EndMillFlat :
        public EndMill
    {
    public:
        EndMillFlat(int toolid, float radius, int nslices);

    private:
        float _profVerts[PROFILE_BUFFER_SIZE(FLAT_MILL_PROFILE_VERTS)];
    };
}
#endif
