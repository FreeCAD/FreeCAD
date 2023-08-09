/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTAnimationParameters : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

		int nframe = 1000000, icurrent = 0, istart = 0, iend = 1000000, framesPerSecond = 30;
        bool isForward = true;


    };
}

