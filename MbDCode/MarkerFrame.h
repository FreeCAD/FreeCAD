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
        void setrpmp(FullColDptr x);
        void setaApm(FullMatDptr x);
        void addEndFrame(std::shared_ptr<EndFrameqc> x);

        PartFrame* partFrame;
        FullColDptr rpmp = std::make_shared<FullColumn<double>>(3);
        FullMatDptr aApm = std::make_shared<FullMatrix<double>>(3, 3);
        FullColDptr rOmO = std::make_shared<FullColumn<double>>(3);
        FullMatDptr aAOm = std::make_shared<FullMatrix<double>>(3, 3);
        FullMatDptr prOmOpE = std::make_shared<FullMatrix<double>>(3, 4);
        FullColumn<FullMatrix<double>>* pAOmpE = new FullColumn<FullMatrix<double>>(4);
        std::vector<std::shared_ptr<EndFrameqc>> endFrames;

    };
}

