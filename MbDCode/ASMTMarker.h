#pragma once

#include "ASMTItem.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
    class ASMTMarker : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

        std::string name;
        FColDsptr position3D;
        FMatDsptr rotationMatrix;

    };
}

