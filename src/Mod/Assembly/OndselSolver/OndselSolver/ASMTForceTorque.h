/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTItemIJ.h"

namespace MbD {
    class ASMTForceTorque : public ASMTItemIJ
    {
        //
    public:
        void updateFromMbD() override;
        void compareResults(AnalysisType type) override;
        void outputResults(AnalysisType type) override;


    };
}

