#pragma once

#include "ASMTItem.h"
#include <vector>
#include <string>
#include "ASMTMarker.h"

namespace MbD {
    class ASMTRefPoint : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        FColDsptr position3D;
        FMatDsptr rotationMatrix;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTMarker>>> markers;
    };
}

