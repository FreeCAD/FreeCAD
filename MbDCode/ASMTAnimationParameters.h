#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTAnimationParameters : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

		int nframe, icurrent, istart, iend, framesPerSecond;
        bool isForward;

    };
}

