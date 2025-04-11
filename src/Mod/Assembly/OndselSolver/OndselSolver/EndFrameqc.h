/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "EndFramec.h"
#include "Symbolic.h"
#include "EulerParametersDot.h"
#include "EulerParametersDDot.h"

namespace MbD {
    class EndFrameqct;

    class EndFrameqc : public EndFramec
    {
        //prOeOpE pprOeOpEpE pAOepE ppAOepEpE
    public:
        EndFrameqc();
        EndFrameqc(const std::string& str);
        void initialize() override;
        void initializeGlobally() override;
        void initEndFrameqct() override;
        void initEndFrameqct2() override;
        FMatFColDsptr ppAjOepEpE(size_t j);
        void calcPostDynCorrectorIteration() override;
        FMatDsptr pAjOepET(size_t j);
        FMatDsptr ppriOeOpEpE(size_t i);
        size_t iqX();
        size_t iqE();
        FRowDsptr priOeOpE(size_t i);
        FColDsptr qXdot();
        std::shared_ptr<EulerParametersDot<double>> qEdot();
        FColDsptr qXddot();
        FColDsptr qEddot();
        FColDsptr rpep() override;
        FColFMatDsptr pAOppE() override;
        FMatDsptr aBOp() override;
        bool isEndFrameqc() override;

        FMatDsptr prOeOpE;
        FMatFColDsptr pprOeOpEpE;
        FColFMatDsptr pAOepE;
        FMatFMatDsptr ppAOepEpE;
        std::shared_ptr<EndFrameqct> endFrameqct;
    };
}

