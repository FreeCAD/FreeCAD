#pragma once

#include "ASMTConstraintSet.h"

namespace MbD {
    class ASMTJoint : public ASMTConstraintSet
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        std::string name, markerI, markerJ;

    };
}
