#pragma once

#include "ASMTSpatialItem.h"
#include <vector>
#include <string>
#include "ASMTMarker.h"

namespace MbD {
    class ASMTRefPoint : public ASMTSpatialItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readMarkers(std::vector<std::string>& lines);
        void readMarker(std::vector<std::string>& lines);

        std::shared_ptr<std::vector<std::shared_ptr<ASMTMarker>>> markers;
    };
}

