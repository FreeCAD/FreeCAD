#pragma once

#include "ASMTItem.h"
//#include "Units.h"

namespace MbD {
    class System;
    class Units;

    class ASMTConstantGravity : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

        FColDsptr g;
    };
}

