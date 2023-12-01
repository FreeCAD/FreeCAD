/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynNode.h"

namespace MbD {
    class MBDynStructural : public MBDynNode
    {
    public:
        MBDynStructural();
        void parseMBDyn(std::string line) override;
        void readVelocity(std::vector<std::string>& args);
        void readOmega(std::vector<std::string>& args);
        void createASMT() override;

        std::string strucString, type;
        FColDsptr rOfO, vOfO, omeOfO;
        FMatDsptr aAOf;
    };
}
