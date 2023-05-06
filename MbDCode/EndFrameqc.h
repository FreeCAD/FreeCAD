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
        FullMatDptr prOeOpE = std::make_shared<FullMatrix<double>>(3, 4);
        //std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOeOpEpE = std::make_shared<FullMatrix<std::shared_ptr<FullColumn<double>>>>(3, 4);
        std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> pAOepE = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
        //std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>> ppAOepEpE = std::make_shared<FullMatrix<std::shared_ptr<FullMatrix<double>>>>(4, 4);
        //FullMatrix<FullColumn<double>>* pprOeOpEpE1 = new FullMatrix<FullColumn<double>>(3, 4);
        //FullColumn<FullMatrix<double>>* pAOepE1 = new FullColumn<FullMatrix<double>>(4);
        //FullMatrix<FullMatrix<double>>* ppAOepEpE1 = new FullMatrix<FullMatrix<double>>(4, 4);
    };
}

