/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTMotion.h"

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
        void storeOnLevel(std::ofstream& os, int level) override;
        void storeOnTimeSeries(std::ofstream& os) override;

        std::shared_ptr<FullColumn<std::string>> rIJI = std::make_shared<FullColumn<std::string>>(3);
        std::shared_ptr<FullColumn<std::string>> angIJJ = std::make_shared<FullColumn<std::string>>(3);
        std::shared_ptr<std::vector<int>> rotationOrder = std::make_shared<std::vector<int>>(std::initializer_list<int>{ 1, 2, 3 });

    };
}

