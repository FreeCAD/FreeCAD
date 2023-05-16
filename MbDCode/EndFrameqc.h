#pragma once

#include "EndFramec.h"

namespace MbD {
    class Symbolic;

    class EndFrameqc : public EndFramec
    {
        //prOeOpE pprOeOpEpE pAOepE ppAOepEpE
    public:
        EndFrameqc();
        EndFrameqc(const char* str);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        void EndFrameqctFrom(EndFrmcptr& frm) override;
        void setrmemBlks(std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> xyzBlks);
        void setphiThePsiBlks(std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> xyzRotBlks);

        FMatDsptr prOeOpE;
        std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOeOpEpE;
        std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> pAOepE;
        FMatFMatDsptr ppAOepEpE;
        std::shared_ptr<EndFramec> endFrameqct;
    };
}

