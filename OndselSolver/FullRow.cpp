/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "FullRow.h"

using namespace MbD;

template<typename T>
FMatsptr<T> FullRow<T>::transposeTimesFullRow(FRowsptr<T> fullRow)
{
    //"a*b = a(i)b(j)"
    auto nrow = (int)this->size();
    auto answer = std::make_shared<MbD::FullMatrix<double>>(nrow);
    for (int i = 0; i < nrow; i++)
    {
        answer->atiput(i, fullRow->times(this->at(i)));
    }
    return answer;
}

template<typename T>
inline FRowsptr<T> FullRow<T>::timesTransposeFullMatrix(FMatsptr<T> fullMat)
{
    //"a*bT = a(1,j)b(k,j)"
    int ncol = fullMat->nrow();
    auto answer = std::make_shared<FullRow<T>>(ncol);
    for (int k = 0; k < ncol; k++) {
        answer->at(k) = this->dot(fullMat->at(k));
    }
    return answer;
}

template<typename T>
inline FRowsptr<T> FullRow<T>::timesFullMatrix(FMatsptr<T> fullMat)
{
    FRowsptr<T> answer = fullMat->at(0)->times(this->at(0));
    for (int j = 1; j < (int) this->size(); j++)
    {
        answer->equalSelfPlusFullRowTimes(fullMat->at(j), this->at(j));
    }
    return answer;
    //return FRowsptr<T>();
}
