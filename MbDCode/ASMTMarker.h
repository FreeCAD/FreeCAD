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
        FColDsptr rpmp();
        FMatDsptr aApm();
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

    };
}

