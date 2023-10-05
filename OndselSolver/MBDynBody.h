/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynElement.h"

namespace MbD {
    class MBDynBody : public MBDynElement
    {
    public:
        void initialize() override;
        void parseMBDyn(std::string line);
        void readInertiaMatrix(std::shared_ptr<std::vector<std::string>>& args);

        std::string bodyString, name, node;
        double mass;
        FColDsptr rOfO;
        FMatDsptr aAOf;

    };
}
