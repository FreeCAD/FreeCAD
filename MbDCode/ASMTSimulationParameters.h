#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTSimulationParameters : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        double tstart, tend, hmin, hmax, hout, errorTol;

    };
}

