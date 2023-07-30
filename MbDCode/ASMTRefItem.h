#pragma once

#include "ASMTSpatialItem.h"
#include "ASMTMarker.h"

namespace MbD {

    class ASMTRefItem : public ASMTSpatialItem
    {
        //
    public:
        void readMarkers(std::vector<std::string>& lines);
        void readMarker(std::vector<std::string>& lines);

        std::shared_ptr<std::vector<std::shared_ptr<ASMTMarker>>> markers;


    };
}

