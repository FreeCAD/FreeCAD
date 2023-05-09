#pragma once
#include "EndFramec.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
    class EndFramec;

    class EndFrameqc : public EndFramec
    {
        //prOeOpE pprOeOpEpE pAOepE ppAOepEpE
    public:
        EndFrameqc();
        EndFrameqc(const char* str);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        void EndFrameqctFrom(std::shared_ptr<EndFramec>& frm) override;

        FMatDuptr prOeOpE;
        std::unique_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOeOpEpE;
        std::unique_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> pAOepE;
        FMatFMatDuptr ppAOepEpE;
    };
}

