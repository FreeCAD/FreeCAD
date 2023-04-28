#pragma once
#include "EndFramec.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {

    class EndFrameqc : public EndFramec
    {
        //prOeOpE pprOeOpEpE pAOepE ppAOepEpE
    public:
        EndFrameqc();
        FullMatrix<double>* prOeOpE = new FullMatrix<double>(3, 4);
        FullMatrix<FullColumn<double>>* pprOeOpEpE = new FullMatrix<FullColumn<double>>(3, 4);
        FullColumn<FullMatrix<double>>* pAOepE = new FullColumn<FullMatrix<double>>(4);
        FullMatrix<FullMatrix<double>>* ppAOepEpE = new FullMatrix<FullMatrix<double>>(4, 4);
    };
}

