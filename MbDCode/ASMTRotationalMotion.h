#pragma once

#include "ASMTMotion.h"

namespace MbD {
    class ASMTRotationalMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        std::string name, motionJoint, rotationZ;
    };
}

