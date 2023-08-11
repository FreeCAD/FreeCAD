/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTSpatialContainer.h"
#include "ASMTRefPoint.h"
#include "ASMTRefCurve.h"
#include "ASMTRefSurface.h"
#include "ASMTPrincipalMassMarker.h"
#include "PosVelAccData.h"

namespace MbD {
    class ASMTPart : public ASMTSpatialContainer
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readFeatureOrder(std::vector<std::string>& lines);
        void readPrincipalMassMarker(std::vector<std::string>& lines);
        void readPartSeries(std::vector<std::string>& lines);
        FColDsptr vOcmO() override;
        FColDsptr omeOpO() override;

        //std::shared_ptr<std::vector<std::shared_ptr<ASMTFeature>>> featureOrder;
        std::shared_ptr<std::vector<std::shared_ptr<PosVelAccData>>> partSeries;

    };
}

