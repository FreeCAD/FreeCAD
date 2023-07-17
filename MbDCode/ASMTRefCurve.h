#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTRefCurve : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;


    };
}

