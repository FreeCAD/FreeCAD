#pragma once

#include "EndFramec.h"
#include "Symbolic.h"

namespace MbD {
    class EndFrameqct;

    class EndFrameqc : public EndFramec
    {
        //prOeOpE pprOeOpEpE pAOepE ppAOepEpE
    public:
        EndFrameqc();
        EndFrameqc(const char* str);
        void initialize();
        void initializeGlobally() override;
        void initEndFrameqct() override;
        FMatFColDsptr ppAjOepEpE(size_t j);
        void calcPostDynCorrectorIteration() override;
        FMatDsptr pAjOepET(size_t j);
        FMatDsptr ppriOeOpEpE(size_t i);
        size_t iqX();
        size_t iqE();
        FRowDsptr priOeOpE(size_t i);

        FMatDsptr prOeOpE;
        std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOeOpEpE;
        std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> pAOepE;
        FMatFMatDsptr ppAOepEpE;
        std::shared_ptr<EndFrameqct> endFrameqct;
    };
}

