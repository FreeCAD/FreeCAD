/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DiagonalMatrix.h"
#include "FullMatrix.h"

namespace MbD {

    DiagMatDsptr DiagonalMatrix::times(double factor)
    {
        auto nrow = (int)this->size();
        auto answer = std::make_shared<DiagonalMatrix>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = this->at(i) * factor;
        }
        return answer;
    }
    void DiagonalMatrix::atiputDiagonalMatrix(int i, std::shared_ptr<DiagonalMatrix> diagMat)
    {
        for (int ii = 0; ii < diagMat->size(); ii++)
        {
            this->at(i + ii) = diagMat->at(ii);
        }
    }
    FColsptr<double> DiagonalMatrix::timesFullColumn(FColsptr<double> fullCol)
    {
        //"a*b = a(i,j)b(j) sum j."

        auto nrow = (int)this->size();
        auto answer = std::make_shared<FullColumn<double>>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = this->at(i) * fullCol->at(i);
        }
        return answer;
    }
    FMatDsptr DiagonalMatrix::timesFullMatrix(FMatDsptr fullMat)
    {
        auto nrow = (int)this->size();
        auto answer = std::make_shared<FullMatrixDouble>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = fullMat->at(i)->times(this->at(i));
        }
        return answer;
    }
    double DiagonalMatrix::sumOfSquares()
    {
        double sum = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            double element = this->at(i);
            sum += element * element;
        }
        return sum;
    }
    int DiagonalMatrix::numberOfElements()
    {
        auto n = (int)this->size();
        return n * n;
    }
    void DiagonalMatrix::zeroSelf()
    {
        for (int i = 0; i < this->size(); i++) {
            this->at(i) = 0.0;
        }
    }
    double DiagonalMatrix::maxMagnitude()
    {
        double max = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            double element = this->at(i);
            if (element < 0.0) element = -element;
            if (max < element) max = element;
        }
        return max;
    }
    std::ostream& DiagonalMatrix::printOn(std::ostream& s) const
    {
        s << "DiagMat[";
        s << this->at(0);
        for (int i = 1; i < this->size(); i++)
        {
            s << ", " << this->at(i);
        }
        s << "]";
        return s;
    }
}
