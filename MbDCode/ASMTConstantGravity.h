#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTConstantGravity : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        FColDsptr g;
    };
}

