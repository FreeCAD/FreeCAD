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
    template<typename T>
    std::shared_ptr<FullMatrixDouble> FullRow<T>::transposeTimesFullRow(FRowsptr<T> fullRow)
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

// TODO: can't get the following to work, but CLion says the routine that calls it in FullMatrixFullMatrixDouble is also
//       never called.
//    template<>
//    FRowsptr<std::shared_ptr<FullMatrixDouble>> FullRow<std::shared_ptr<FullMatrixDouble>>::timesTransposeFullMatrixForFMFMDsptr(
//            std::shared_ptr<FullMatrixFullMatrixDouble> fullMat)
//    {
//        //"a*bT = a(1,j)b(k,j)"
//        int ncol = fullMat->nrow();
//        auto answer = std::make_shared<FullRow<std::shared_ptr<FullMatrixDouble>>>(ncol);
//        for (int k = 0; k < ncol; k++) {
//            answer->at(k) = this->dot(fullMat->at(k));
//        }
//        return answer;
//    }

    template<>
    FRowsptr<double> FullRow<double>::timesFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        FRowsptr<double> answer = fullMat->at(0)->times(this->at(0));
        for (int j = 1; j < (int) this->size(); j++)
        {
            answer->equalSelfPlusFullRowTimes(fullMat->at(j), this->at(j));
        }
        return answer;
    }
}
