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
        void readMass(std::vector<std::string>& args);
        void readInertiaMatrix(std::vector<std::string>& args);
        void createASMT() override;

        std::string bodyString, nodeName;
        double mass;
        FColDsptr rPcmP;
        FMatDsptr aJmat;

    };
}
