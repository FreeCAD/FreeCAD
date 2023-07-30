#pragma once

#include "ASMTRefItem.h"

namespace MbD {
    class ASMTRefCurve : public ASMTRefItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;


    };
}

