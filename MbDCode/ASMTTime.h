#pragma once

#include "ExpressionX.h"
#include "System.h"
#include "Units.h"

namespace MbD {
    class ASMTTime : public ExpressionX
    {
        //
    public:
        void deleteMbD();
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits);

    };
}

