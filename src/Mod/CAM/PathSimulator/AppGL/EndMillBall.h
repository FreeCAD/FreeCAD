#ifndef __end_mill_ball_h__
#define __end_mill_ball_h__

#include "EndMill.h"
namespace MillSim
{
    class EndMillBall :
        public EndMill
    {
    public:
        EndMillBall(int toolid, float radius, int nslices, int nSections, float flatRadius);
        virtual ~EndMillBall();

    };
}
#endif
