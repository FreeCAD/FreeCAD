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

    template<>
    inline DiagMatDsptr DiagonalMatrix<double>::times(double factor)
    {
        auto nrow = (int)this->size();
        auto answer = std::make_shared<DiagonalMatrix<double>>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = this->at(i) * factor;
        }
        return answer;
    }
    template<typename T>
    inline void DiagonalMatrix<T>::atiputDiagonalMatrix(int i, std::shared_ptr<DiagonalMatrix<T>> diagMat)
    {
        for (int ii = 0; ii < diagMat->size(); ii++)
        {
            this->at(i + ii) = diagMat->at(ii);
        }
    }
    template<typename T>
    inline DiagMatsptr<T> DiagonalMatrix<T>::times(T factor)
    {
        assert(false);
    }
    template<typename T>
    inline FColsptr<T> DiagonalMatrix<T>::timesFullColumn(FColsptr<T> fullCol)
    {
        //"a*b = a(i,j)b(j) sum j."

        auto nrow = (int)this->size();
        auto answer = std::make_shared<FullColumn<T>>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = this->at(i) * fullCol->at(i);
        }
        return answer;
    }
    template<typename T>
    inline FMatDsptr DiagonalMatrix<T>::timesFullMatrix(FMatDsptr fullMat)
    {
        auto nrow = (int)this->size();
        auto answer = std::make_shared<FullMatrixDouble>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = fullMat->at(i)->times(this->at(i));
        }
        return answer;
    }
    template<>
    inline double DiagonalMatrix<double>::sumOfSquares()
    {
        double sum = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            double element = this->at(i);
            sum += element * element;
        }
        return sum;
    }
    template<typename T>
    inline int DiagonalMatrix<T>::numberOfElements()
    {
        auto n = (int)this->size();
        return n * n;
    }
    template<>
    inline void DiagonalMatrix<double>::zeroSelf()
    {
        for (int i = 0; i < this->size(); i++) {
            this->at(i) = 0.0;
        }
    }
    template<>
    inline double DiagonalMatrix<double>::maxMagnitude()
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
    template<typename T>
    inline double DiagonalMatrix<T>::maxMagnitude()
    {
        assert(false);
        return 0.0;
    }
    template<typename T>
    inline std::ostream& DiagonalMatrix<T>::printOn(std::ostream& s) const
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
