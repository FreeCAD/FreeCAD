#pragma once

#include "ASMTConstraintSet.h"
#include "ForceTorqueData.h"

namespace MbD {
    class ASMTMotion : public ASMTConstraintSet
    {
        //
    public:
        void readMotionSeries(std::vector<std::string>& lines);

        std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueData>>> motionSeries;

    };
}
