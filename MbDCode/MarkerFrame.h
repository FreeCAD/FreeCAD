#pragma once
#include <memory>

#include "CartesianFrame.h"
#include "PartFrame.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EndFrameqc.h"

namespace MbD {
    class PartFrame;
    class EndFrameqc;

    class MarkerFrame : public CartesianFrame
    {
        //partFrame rpmp aApm rOmO aAOm prOmOpE pAOmpE pprOmOpEpE ppAOmpEpE endFrames 
    public:
        MarkerFrame();
        void setPartFrame(PartFrame* partFrm);
        void setrpmp(FullColumn<double>* x);
        void setaApm(FullMatrix<double>* x);
        void addEndFrame(std::shared_ptr<EndFrameqc> x);

        PartFrame* partFrame;
        FullColumn<double>* rpmp = new FullColumn<double>(3);
        FullMatrix<double>* aApm = new FullMatrix<double>(3, 3);
        FullColumn<double>* rOmO = new FullColumn<double>(3);
        FullMatrix<double>* aAOm = new FullMatrix<double>(3, 3);
        FullMatrix<double>* prOmOpE = new FullMatrix<double>(3, 4);
        FullColumn<FullMatrix<double>>* pAOmpE = new FullColumn<FullMatrix<double>>(4);
        std::vector<std::shared_ptr<EndFrameqc>> endFrames;

    };
}

