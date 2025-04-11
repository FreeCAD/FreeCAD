/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTSpatialItem.h"
#include "ASMTMarker.h"

namespace MbD {

    class ASMTRefItem : public ASMTSpatialItem
    {
        //
    public:
        void addMarker(std::shared_ptr<ASMTMarker> marker);
        void readMarkers(std::vector<std::string>& lines);
        void readMarker(std::vector<std::string>& lines);
        void storeOnLevel(std::ofstream& os, size_t level) override;

        std::shared_ptr<std::vector<std::shared_ptr<ASMTMarker>>> markers = std::make_shared<std::vector<std::shared_ptr<ASMTMarker>>>();


    };
}

