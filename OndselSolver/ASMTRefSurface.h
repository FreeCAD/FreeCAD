/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTRefItem.h"

namespace MbD {
    class ASMTRefSurface : public ASMTRefItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void storeOnLevel(std::ofstream& os, size_t level) override;


    };
}

