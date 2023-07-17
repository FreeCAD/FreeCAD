#pragma once

#include "ASMTMotion.h"

namespace MbD {
    class ASMTTranslationalMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        std::string name, motionJoint, translationZ;

    };
}

