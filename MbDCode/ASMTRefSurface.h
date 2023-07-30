#pragma once

#include "ASMTRefItem.h"

namespace MbD {
    class ASMTRefSurface : public ASMTRefItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;


    };
}

