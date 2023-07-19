#pragma once

#include "ASMTSpatialItem.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
    class ASMTMarker : public ASMTSpatialItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

    };
}

