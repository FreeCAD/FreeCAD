#pragma once

#include "ASMTMotion.h"

namespace MbD {
    class ASMTRotationalMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readMotionJoint(std::vector<std::string>& lines);
        void readRotationZ(std::vector<std::string>& lines);

        std::string motionJoint, rotationZ;
    };
}

