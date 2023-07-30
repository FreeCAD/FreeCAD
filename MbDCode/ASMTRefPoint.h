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
        std::string fullName(std::string partialName) override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

    
    };
}

