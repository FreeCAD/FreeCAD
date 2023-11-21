/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "FullRow.h"
#include "FullMatrix.h"

namespace MbD {
    template<>
    std::shared_ptr<FullMatrixDouble> FullRow<double>::transposeTimesFullRow(FRowsptr<double> fullRow)
    {
        //"a*b = a(i)b(j)"
        auto nrow = (int)this->size();
        auto answer = std::make_shared<FullMatrixDouble>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->atiput(i, fullRow->times(this->at(i)));
        }
        return answer;
    }
    //template<typename T>
    //inline FMatDsptr FullRow<T>::transposeTimesFullRow(FRowDsptr fullRow)
    //{
    //    //"a*b = a(i)b(j)"
    //    auto nrow = (int)this->size();
    //    auto answer = std::make_shared<FullMatrixDouble>(nrow);
    //    for (int i = 0; i < nrow; i++)
    //    {
    //        answer->atiput(i, fullRow->times(this->at(i)));
    //    }
    //    return answer;
    //}

    template<>
    FRowsptr<double> FullRow<double>::timesTransposeFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        //"a*bT = a(1,j)b(k,j)"
        int ncol = fullMat->nrow();
        auto answer = std::make_shared<FullRow<double>>(ncol);
        for (int k = 0; k < ncol; k++) {
            answer->at(k) = this->dot(fullMat->at(k));
        }
        return answer;
    }

    template<>
    FRowsptr<double> FullRow<double>::timesFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        FRowsptr<double> answer = fullMat->at(0)->times(this->at(0));
        for (size_t j = 1; j < this->size(); j++)
        {
            answer->equalSelfPlusFullRowTimes(fullMat->at(j), this->at(j));
        }
        return answer;
    }
}
