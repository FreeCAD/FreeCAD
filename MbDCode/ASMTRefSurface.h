#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTRefSurface : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;


    };
}

