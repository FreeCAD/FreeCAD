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
        void initialize() override;
        void parseMBDyn(std::string line);
        void readVelocity(std::shared_ptr<std::vector<std::string>>& args);
        void readOmega(std::shared_ptr<std::vector<std::string>>& args);

        std::string strucString, name, type;
        FColDsptr rOfO, vOfO, omeOfO;
        FMatDsptr aAOf;
    };
}
