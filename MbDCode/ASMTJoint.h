#pragma once

#include "ASMTConstraintSet.h"
#include "ForceTorqueData.h"

namespace MbD {
    class ASMTJoint : public ASMTConstraintSet
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readJointSeries(std::vector<std::string>& lines);

        std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueData>>> jointSeries;

    };
}
