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
        static std::shared_ptr<ASMTGeneralMotion> With();
        void parseASMT(std::vector<std::string>& lines) override;
        void readrIJI(std::vector<std::string>& lines);
        void readangIJJ(std::vector<std::string>& lines);
        void readRotationOrder(std::vector<std::string>& lines);
        std::shared_ptr<ItemIJ> mbdClassNew() override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        void storeOnLevel(std::ofstream& os, size_t level) override;
        void storeOnTimeSeries(std::ofstream& os) override;

        std::shared_ptr<FullColumn<std::string>> rIJI = std::make_shared<FullColumn<std::string>>(3);
        std::shared_ptr<FullColumn<std::string>> angIJJ = std::make_shared<FullColumn<std::string>>(3);
        std::shared_ptr<std::vector<size_t>> rotationOrder = std::make_shared<std::vector<size_t>>(std::initializer_list<size_t>{ 1, 2, 3 });

    };
}

