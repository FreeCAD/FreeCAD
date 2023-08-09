/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTMotion.h"
#include "FullMotion.h"

namespace MbD {
    class ASMTGeneralMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readrIJI(std::vector<std::string>& lines);
        void readangIJJ(std::vector<std::string>& lines);
        void readRotationOrder(std::vector<std::string>& lines);
        std::shared_ptr<Joint> mbdClassNew() override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

        std::shared_ptr<std::vector<std::string>> rIJI, angIJJ;
        std::string rotationOrder;

    };
}

