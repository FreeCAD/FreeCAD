#pragma once

#include "ASMTMotion.h"

namespace MbD {
    class ASMTGeneralMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;


    };
}

