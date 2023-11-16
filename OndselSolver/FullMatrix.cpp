/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "FullMatrix.h"
#include "FullColumn.h"
#include "FullRow.h"
#include "DiagonalMatrix.h"
#include "EulerParameters.h"

namespace MbD {
    FColsptr<double> FullMatrixDouble::timesFullColumn(FColsptr<double> fullCol)
    {
        return this->timesFullColumn(fullCol.get());
//        auto nrow = this->nrow();
//        auto answer = std::make_shared<FullColumn<double>>(nrow);
//        for (int i = 0; i < nrow; i++)
//        {
//            answer->at(i) = this->at(i)->timesFullColumn(fullCol);
//        }
//        return answer;
    }
    FColsptr<double> FullMatrixDouble::timesFullColumn(FullColumn<double>* fullCol) // local
    {
        //"a*b = a(i,j)b(j) sum j."
        auto nrow = this->nrow();
        auto answer = std::make_shared<FullColumn<double>>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            answer->at(i) = this->at(i)->timesFullColumn(fullCol);
        }
        return answer;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatex(double the)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, 1.0);
        row0->atiput(1, 0.0);
        row0->atiput(2, 0.0);
        auto row1 = rotMat->at(1);
        row1->atiput(0, 0.0);
        row1->atiput(1, cthe);
        row1->atiput(2, -sthe);
        auto row2 = rotMat->at(2);
        row2->atiput(0, 0.0);
        row2->atiput(1, sthe);
        row2->atiput(2, cthe);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatey(double the)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, cthe);
        row0->atiput(1, 0.0);
        row0->atiput(2, sthe);
        auto row1 = rotMat->at(1);
        row1->atiput(0, 0.0);
        row1->atiput(1, 1.0);
        row1->atiput(2, 0.0);
        auto row2 = rotMat->at(2);
        row2->atiput(0, -sthe);
        row2->atiput(1, 0.0);
        row2->atiput(2, cthe);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatez(double the)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, cthe);
        row0->atiput(1, -sthe);
        row0->atiput(2, 0.0);
        auto row1 = rotMat->at(1);
        row1->atiput(0, sthe);
        row1->atiput(1, cthe);
        row1->atiput(2, 0.0);
        auto row2 = rotMat->at(2);
        row2->atiput(0, 0.0);
        row2->atiput(1, 0.0);
        row2->atiput(2, 1.0);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatexrotDot(double the, double thedot)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto sthedot = cthe * thedot;
        auto cthedot = -sthe * thedot;
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, 0.0);
        row0->atiput(1, 0.0);
        row0->atiput(2, 0.0);
        auto row1 = rotMat->at(1);
        row1->atiput(0, 0.0);
        row1->atiput(1, cthedot);
        row1->atiput(2, -sthedot);
        auto row2 = rotMat->at(2);
        row2->atiput(0, 0.0);
        row2->atiput(1, sthedot);
        row2->atiput(2, cthedot);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotateyrotDot(double the, double thedot)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto sthedot = cthe * thedot;
        auto cthedot = -sthe * thedot;
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, cthedot);
        row0->atiput(1, 0.0);
        row0->atiput(2, sthedot);
        auto row1 = rotMat->at(1);
        row1->atiput(0, 0.0);
        row1->atiput(1, 0.0);
        row1->atiput(2, 0.0);
        auto row2 = rotMat->at(2);
        row2->atiput(0, -sthedot);
        row2->atiput(1, 0.0);
        row2->atiput(2, cthedot);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatezrotDot(double the, double thedot)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto sthedot = cthe * thedot;
        auto cthedot = -sthe * thedot;
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, cthedot);
        row0->atiput(1, -sthedot);
        row0->atiput(2, 0.0);
        auto row1 = rotMat->at(1);
        row1->atiput(0, sthedot);
        row1->atiput(1, cthedot);
        row1->atiput(2, 0.0);
        auto row2 = rotMat->at(2);
        row2->atiput(0, 0.0);
        row2->atiput(1, 0.0);
        row2->atiput(2, 0.0);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatexrotDotrotDDot(double the, double thedot, double theddot)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto sthedot = cthe * thedot;
        auto cthedot = -sthe * thedot;
        auto stheddot = cthedot * thedot + (cthe * theddot);
        auto ctheddot = -(sthedot * thedot) - (sthe * theddot);
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, 0.0);
        row0->atiput(1, 0.0);
        row0->atiput(2, 0.0);
        auto row1 = rotMat->at(1);
        row1->atiput(0, 0.0);
        row1->atiput(1, ctheddot);
        row1->atiput(2, -stheddot);
        auto row2 = rotMat->at(2);
        row2->atiput(0, 0.0);
        row2->atiput(1, stheddot);
        row2->atiput(2, ctheddot);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotateyrotDotrotDDot(double the, double thedot, double theddot)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto sthedot = cthe * thedot;
        auto cthedot = -sthe * thedot;
        auto stheddot = cthedot * thedot + (cthe * theddot);
        auto ctheddot = -(sthedot * thedot) - (sthe * theddot);
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, ctheddot);
        row0->atiput(1, 0.0);
        row0->atiput(2, stheddot);
        auto row1 = rotMat->at(1);
        row1->atiput(0, 0.0);
        row1->atiput(1, 0.0);
        row1->atiput(2, 0.0);
        auto row2 = rotMat->at(2);
        row2->atiput(0, -stheddot);
        row2->atiput(1, 0.0);
        row2->atiput(2, ctheddot);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::rotatezrotDotrotDDot(double the, double thedot, double theddot)
    {
        auto sthe = std::sin(the);
        auto cthe = std::cos(the);
        auto sthedot = cthe * thedot;
        auto cthedot = -sthe * thedot;
        auto stheddot = cthedot * thedot + (cthe * theddot);
        auto ctheddot = -(sthedot * thedot) - (sthe * theddot);
        auto rotMat = std::make_shared<FullMatrixDouble>(3, 3);
        auto row0 = rotMat->at(0);
        row0->atiput(0, ctheddot);
        row0->atiput(1, -stheddot);
        row0->atiput(2, 0.0);
        auto row1 = rotMat->at(1);
        row1->atiput(0, stheddot);
        row1->atiput(1, ctheddot);
        row1->atiput(2, 0.0);
        auto row2 = rotMat->at(2);
        row2->atiput(0, 0.0);
        row2->atiput(1, 0.0);
        row2->atiput(2, 0.0);
        return rotMat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::identitysptr(int n)
    {
        auto mat = std::make_shared<FullMatrixDouble>(n, n);
        mat->identity();
        return mat;
    }
    std::shared_ptr<MbD::FullMatrixFullMatrixDouble> FullMatrixFullMatrixDouble::identitysptr(int n)
    {
        auto mat = std::make_shared<FullMatrixFullMatrixDouble>(n, n);
        mat->identity();
        return mat;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::tildeMatrix(FColDsptr col)
    {
        //"tildeMatrix is skew symmetric matrix related to angular velocity and cross product."
        if (col->size() != 3) throw std::runtime_error("Column is not of dimension 3");
        auto tilde = std::make_shared<FullMatrixDouble>(3, 3);
        auto c0 = col->at(0);
        auto c1 = col->at(1);
        auto c2 = col->at(2);
        tilde->atijput(0, 0, 0.0);
        tilde->atijput(1, 1, 0.0);
        tilde->atijput(2, 2, 0.0);
        tilde->atijput(1, 2, -c0);
        tilde->atijput(0, 2, c1);
        tilde->atijput(0, 1, -c2);
        tilde->atijput(1, 0, c2);
        tilde->atijput(2, 0, -c1);
        tilde->atijput(2, 1, c0);
        return tilde;
    }
    void FullMatrixDouble::zeroSelf()
    {
        for (int i = 0; i < this->size(); i++) {
            this->at(i)->zeroSelf();
        }
    }
    void FullMatrixFullMatrixDouble::zeroSelf()
    {
        for (int i = 0; i < this->size(); i++) {
            this->at(i)->zeroSelf();
        }
    }
    void FullMatrixFullColumnDouble::zeroSelf()
    {
        for (int i = 0; i < this->size(); i++) {
            this->at(i)->zeroSelf();
        }
    }
    void FullMatrixDouble::identity() {
        this->zeroSelf();
        for (int i = 0; i < this->size(); i++) {
            this->at(i)->at(i) = 1.0;
        }
    }
    void FullMatrixFullMatrixDouble::identity() {
        assert(false);
//    this->zeroSelf();
//    for (int i = 0; i < this->size(); i++) {
//        this->at(i)->at(i) = 1.0;
//    }
    }
    // TODO: should there be a FullMatrixFullColumnDouble version of this?
    FColsptr<double> FullMatrixDouble::column(int j) {
        int n = (int)this->size();
        auto answer = std::make_shared<FullColumn<double>>(n);
        for (int i = 0; i < n; i++) {
            answer->at(i) = this->at(i)->at(j);
        }
        return answer;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::timesFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        int m = this->nrow();
        auto answer = std::make_shared<FullMatrixDouble>(m);
        for (int i = 0; i < m; i++) {
            answer->at(i) = this->at(i)->timesFullMatrix(fullMat);
        }
        return answer;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::timesTransposeFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        int nrow = this->nrow();
        auto answer = std::make_shared<FullMatrixDouble>(nrow);
        for (int i = 0; i < nrow; i++) {
            answer->at(i) = this->at(i)->timesTransposeFullMatrix(fullMat);
        }
        return answer;
    }
//    std::shared_ptr<FullMatrixFullMatrixDouble> FullMatrixFullMatrixDouble::timesTransposeFullMatrix(std::shared_ptr<FullMatrixFullMatrixDouble> fullMat)
//    {
//        int nrow = this->nrow();
//        auto answer = std::make_shared<FullMatrixFullMatrixDouble>(nrow);
//        for (int i = 0; i < nrow; i++) {
//            answer->at(i) = this->at(i)->timesTransposeFullMatrixForFMFMDsptr(fullMat);
//        }
//        return answer;
//    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::times(double a)
    {
        int m = this->nrow();
        auto answer = std::make_shared<FullMatrixDouble>(m);
        for (int i = 0; i < m; i++) {
            // auto x = this->at(i);
            answer->at(i) = this->at(i)->times(a);
        }
        return answer;
    }
    std::shared_ptr<FullMatrixFullMatrixDouble> FullMatrixFullMatrixDouble::times(double a)
    {
        // TODO: correct action?
        assert(false);
        return std::make_shared<FullMatrixFullMatrixDouble>();
    }
    std::shared_ptr<FullMatrixFullColumnDouble> FullMatrixFullColumnDouble::times(double a)
    {
        // TODO: correct action?
        assert(false);
        return std::make_shared<FullMatrixFullColumnDouble>();
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::transposeTimesFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        return this->transpose()->timesFullMatrix(fullMat);
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::plusFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        int n = (int)this->size();
        auto answer = std::make_shared<FullMatrixDouble>(n);
        for (int i = 0; i < n; i++) {
            answer->at(i) = this->at(i)->plusFullRow(fullMat->at(i));
        }
        return answer;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::minusFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat)
    {
        int n = (int)this->size();
        auto answer = std::make_shared<FullMatrixDouble>(n);
        for (int i = 0; i < n; i++) {
            answer->at(i) = this->at(i)->minusFullRow(fullMat->at(i));
        }
        return answer;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::transpose() {
        int nrow = this->nrow();
        auto ncol = this->ncol();
        auto answer = std::make_shared<FullMatrixDouble>(ncol, nrow);
        for (int i = 0; i < nrow; i++) {
            auto& row = this->at(i);
            for (int j = 0; j < ncol; j++) {
                answer->at(j)->at(i) = row->at(j);
            }
        }
        return answer;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::negated()
    {
        return this->times(-1.0);
    }
    void FullMatrixDouble::symLowerWithUpper()
    {
        int n = (int)this->size();
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                this->at(j)->at(i) = this->at(i)->at(j);
            }
        }
    }
    void FullMatrixFullColumnDouble::symLowerWithUpper()
    {
        int n = (int)this->size();
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                this->at(j)->at(i) = this->at(i)->at(j);
            }
        }
    }
    void FullMatrixDouble::atiput(int i, FRowsptr<double> fullRow)
    {
        this->at(i) = fullRow;
    }
    void FullMatrixDouble::atijput(int i, int j, double value)
    {
        this->at(i)->atiput(j, value);
    }
    void FullMatrixDouble::atijputFullColumn(int i1, int j1, FColsptr<double> fullCol)
    {
        for (int ii = 0; ii < fullCol->size(); ii++)
        {
            this->at(i1 + ii)->at(j1) = fullCol->at(ii);
        }
    }
    void FullMatrixDouble::atijplusFullRow(int i, int j, FRowsptr<double> fullRow)
    {
        this->at(i)->atiplusFullRow(j, fullRow);
    }
    void FullMatrixDouble::atijplusNumber(int i, int j, double value)
    {
        auto rowi = this->at(i);
        rowi->at(j) += value;
    }
    void FullMatrixDouble::atijminusNumber(int i, int j, double value)
    {
        auto rowi = this->at(i);
        rowi->at(j) -= value;
    }
    double FullMatrixDouble::sumOfSquares()
    {
        double sum = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            sum += this->at(i)->sumOfSquares();
        }
        return sum;
    }
    double FullMatrixFullMatrixDouble::sumOfSquares()
    {
        double sum = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            sum += this->at(i)->sumOfSquares();
        }
        return sum;
    }
    double FullMatrixFullColumnDouble::sumOfSquares()
    {
        double sum = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            sum += this->at(i)->sumOfSquares();
        }
        return sum;
    }
    std::shared_ptr<FullMatrixDouble> FullMatrixDouble::copy()
    {
        auto m = (int)this->size();
        auto answer = std::make_shared<FullMatrixDouble>(m);
        for (int i = 0; i < m; i++)
        {
            answer->at(i) = this->at(i)->copy();
        }
        return answer;
    }
    FullMatrixDouble FullMatrixDouble::operator+(const FullMatrixDouble fullMat)
    {
        int n = (int)this->size();
        FullMatrixDouble answer(n);
        for (int i = 0; i < n; i++) {
            answer.at(i) = this->at(i)->plusFullRow(fullMat.at(i));
        }
        return answer;
    }
    FColsptr<double> FullMatrixDouble::transposeTimesFullColumn(FColsptr<double> fullCol)
    {
        auto sptr = std::make_shared<FullMatrixDouble>(*this);
        return fullCol->transpose()->timesFullMatrix(sptr)->transpose();
    }
    void FullMatrixDouble::magnifySelf(double factor)
    {
        for (int i = 0; i < this->size(); i++) {
            this->at(i)->magnifySelf(factor);
        }
    }
    std::ostream& FullMatrixDouble::printOn(std::ostream& s) const
    {
        s << "FullMat[" << std::endl;
        for (int i = 0; i < this->size(); i++)
        {
            s << *(this->at(i)) << std::endl;
        }
        s << "]";
        return s;
    }
    std::shared_ptr<EulerParameters<double>> FullMatrixDouble::asEulerParameters()
    {
        //"Given [A], compute Euler parameter."

        auto traceA = this->trace();
        double dum = 0.0;
        double dumSq = 0.0;
        //auto qE = CREATE<EulerParameters<double>>::With(4); //Cannot use CREATE.h in subclasses of std::vector. Why?
        auto qE = std::make_shared<EulerParameters<double>>(4);
        qE->initialize();
        auto OneMinusTraceDivFour = (1.0 - traceA) / 4.0;
        for (int i = 0; i < 3; i++)
        {
            dumSq = this->at(i)->at(i) / 2.0 + OneMinusTraceDivFour;
            dum = (dumSq > 0.0) ? std::sqrt(dumSq) : 0.0;
            qE->atiput(i, dum);
        }
        dumSq = (1.0 + traceA) / 4.0;
        dum = (dumSq > 0.0) ? std::sqrt(dumSq) : 0.0;
        qE->atiput(3, dum);
        double max = 0.0;
        int maxE = -1;
        for (int i = 0; i < 4; i++)
        {
            auto num = qE->at(i);
            if (max < num) {
                max = num;
                maxE = i;
            }
        }

        if (maxE == 0) {
            auto FourE = 4.0 * qE->at(0);
            qE->atiput(1, (this->at(0)->at(1) + this->at(1)->at(0)) / FourE);
            qE->atiput(2, (this->at(0)->at(2) + this->at(2)->at(0)) / FourE);
            qE->atiput(3, (this->at(2)->at(1) - this->at(1)->at(2)) / FourE);
        }
        else if (maxE == 1) {
            auto FourE = 4.0 * qE->at(1);
            qE->atiput(0, (this->at(0)->at(1) + this->at(1)->at(0)) / FourE);
            qE->atiput(2, (this->at(1)->at(2) + this->at(2)->at(1)) / FourE);
            qE->atiput(3, (this->at(0)->at(2) - this->at(2)->at(0)) / FourE);
        }
        else if (maxE == 2) {
            auto FourE = 4.0 * qE->at(2);
            qE->atiput(0, (this->at(0)->at(2) + this->at(2)->at(0)) / FourE);
            qE->atiput(1, (this->at(1)->at(2) + this->at(2)->at(1)) / FourE);
            qE->atiput(3, (this->at(1)->at(0) - this->at(0)->at(1)) / FourE);
        }
        else if (maxE == 3) {
            auto FourE = 4.0 * qE->at(3);
            qE->atiput(0, (this->at(2)->at(1) - this->at(1)->at(2)) / FourE);
            qE->atiput(1, (this->at(0)->at(2) - this->at(2)->at(0)) / FourE);
            qE->atiput(2, (this->at(1)->at(0) - this->at(0)->at(1)) / FourE);
        }
        qE->conditionSelf();
        qE->calc();
        return qE;
    }
    double FullMatrixDouble::trace()
    {
        double trace = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            trace += this->at(i)->at(i);
        }
        return trace;
    }
    double FullMatrixDouble::maxMagnitude()
    {
        double max = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            double element = this->at(i)->maxMagnitude();
            if (max < element) max = element;
        }
        return max;
    }
    double FullMatrixFullMatrixDouble::maxMagnitude()
    {
        double max = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            double element = this->at(i)->maxMagnitude();
            if (max < element) max = element;
        }
        return max;
    }
    double FullMatrixFullColumnDouble::maxMagnitude()
    {
        double max = 0.0;
        for (int i = 0; i < this->size(); i++)
        {
            double element = this->at(i)->maxMagnitude();
            if (max < element) max = element;
        }
        return max;
    }
    FColsptr<double> FullMatrixDouble::bryantAngles()
    {
        auto answer = std::make_shared<FullColumn<double>>(3);
        auto sthe1y = this->at(0)->at(2);
        double the0x, the1y, the2z, cthe0x, sthe0x, y, x;
        if (std::abs(sthe1y) > 0.9999) {
            if (sthe1y > 0.0) {
                the0x = std::atan2(this->at(1)->at(0), this->at(1)->at(1));
                the1y = M_PI / 2.0;
                the2z = 0.0;
            }
            else {
                the0x = std::atan2(this->at(2)->at(1), this->at(2)->at(0));
                the1y = M_PI / -2.0;
                the2z = 0.0;
            }
        }
        else {
            the0x = std::atan2(-this->at(1)->at(2), this->at(2)->at(2));
            cthe0x = std::cos(the0x);
            sthe0x = std::sin(the0x);
            y = sthe1y;
            if (std::abs(cthe0x) > std::abs(sthe0x)) {
                x = this->at(2)->at(2) / cthe0x;
            }
            else {
                x = this->at(1)->at(2) / -sthe0x;
            }
            the1y = std::atan2(y, x);
            the2z = std::atan2(-this->at(0)->at(1), this->at(0)->at(0));
        }
        answer->atiput(0, the0x);
        answer->atiput(1, the1y);
        answer->atiput(2, the2z);
        return answer;
    }
    bool FullMatrixDouble::isDiagonal()
    {
        auto m = this->nrow();
        auto n = this->ncol();
        if (m != n) return false;
        for (int i = 0; i < m; i++)
        {
            auto rowi = this->at(i);
            for (int j = 0; j < n; j++)
            {
                if (i != j && rowi->at(j) != 0) return false;
            }
        }
        return true;
    }
    bool FullMatrixDouble::isDiagonalToWithin(double ratio)
    {
        double maxMag = this->maxMagnitude();
        auto tol = ratio * maxMag;
        auto nrow = this->nrow();
        if (nrow == this->ncol()) {
            for (int i = 0; i < 3; i++)
            {
                for (int j = i + 1; j < 3; j++)
                {
                    if (std::abs(this->at(i)->at(j)) > tol) return false;
                    if (std::abs(this->at(j)->at(i)) > tol) return false;
                }
            }
            return true;
        }
        else {
            return false;
        }
    }
    std::shared_ptr<DiagonalMatrix> FullMatrixDouble::asDiagonalMatrix()
    {
        int nrow = this->nrow();
        auto diagMat = std::make_shared<DiagonalMatrix>(nrow);
        for (int i = 0; i < nrow; i++)
        {
            diagMat->atiput(i, this->at(i)->at(i));
        }
        return diagMat;
    }
    void FullMatrixDouble::conditionSelfWithTol(double tol)
    {
        for (auto row : *this) {
            row->conditionSelfWithTol(tol);
        }
    }
}
