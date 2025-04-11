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
        static std::shared_ptr<ASMTAnimationParameters> With();
        void parseASMT(std::vector<std::string>& lines) override;
        void storeOnLevel(std::ofstream& os, size_t level) override;

		size_t nframe = 1000000, icurrent = 1, istart = 1, iend = 1000000, framesPerSecond = 30;
        bool isForward = true;


    };
}

