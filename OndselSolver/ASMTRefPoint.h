/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTRefItem.h"
#include <vector>
#include <string>

namespace MbD {
    class ASMTRefPoint : public ASMTRefItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        std::string fullName(const std::string& partialName) override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        void storeOnLevel(std::ofstream& os, size_t level) override;

    
    };
}

