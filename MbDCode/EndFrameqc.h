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
        EndFrameqc(const char* str);
        void initialize() override;
        void initializeGlobally() override;
        void initEndFrameqct() override;
        FMatFColDsptr ppAjOepEpE(int j);
        void calcPostDynCorrectorIteration() override;
        FMatDsptr pAjOepET(int j);
        FMatDsptr ppriOeOpEpE(int i);
        int iqX();
        int iqE();
        FRowDsptr priOeOpE(int i);
        FColDsptr qXdot();
        std::shared_ptr<EulerParametersDot<double>> qEdot();
        FColDsptr qXddot();
        FColDsptr qEddot();

        FMatDsptr prOeOpE;
        std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOeOpEpE;
        std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> pAOepE;
        FMatFMatDsptr ppAOepEpE;
        std::shared_ptr<EndFrameqct> endFrameqct;
    };
}

