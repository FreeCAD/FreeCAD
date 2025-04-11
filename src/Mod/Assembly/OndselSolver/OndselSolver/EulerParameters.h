/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD
{

template<typename T>
class EulerParameters: public EulerArray<T>
{
    // Quarternion = {q0, q1, q2, q3}
    // EulerParameters = {qE1, qE2, qE3, qE4} is preferred because Smalltalk uses one-based
    // indexing.
    //  q0 = qE4
    // Note: It is tempting to use quarternions in C++ because of zero-based indexing.
    // Note: But that will make it harder to compare computation results with Smalltalk
    // aA aB aC pApE
public:
    EulerParameters()
        : EulerArray<T>(4)
    {
        this->initialize();
    }
    EulerParameters(size_t count)
        : EulerArray<T>(count)
    {
        this->initialize();
    }
    EulerParameters(size_t count, const T& value)
        : EulerArray<T>(count, value)
    {
        this->initialize();
    }
    EulerParameters(std::initializer_list<T> list)
        : EulerArray<T> {list}
    {
        this->initialize();
    }
    EulerParameters(FColDsptr axis, double theta)
        : EulerArray<T>(4)
    {
        auto halfTheta = theta / 2.0;
        auto sinHalfTheta = std::sin(halfTheta);
        auto cosHalfTheta = std::cos(halfTheta);
        axis->normalizeSelf();
        this->atiputFullColumn(0, axis->times(sinHalfTheta));
        this->atiput(3, cosHalfTheta);
        this->conditionSelf();
        this->initialize();
        this->calc();
    }

    static std::shared_ptr<FullMatrix<FColsptr<T>>> ppApEpEtimesColumn(FColDsptr col);
    static FMatDsptr pCpEtimesColumn(FColDsptr col);
    static FMatDsptr pCTpEtimesColumn(FColDsptr col);
    static std::shared_ptr<FullMatrix<FMatsptr<T>>> ppApEpEtimesMatrix(FMatDsptr mat);


    void initialize() override;
    void calc() override;
    void calcABC();
    void calcpApE();
    void conditionSelf() override;
    std::shared_ptr<EulerParameters<T>> times(T a);
    std::shared_ptr<EulerParameters<T>>
    plusFullColumn(std::shared_ptr<EulerParameters<T>> eulerPars);
    std::shared_ptr<EulerParameters<T>> copy();

    FMatDsptr aA;
    FMatDsptr aB;
    FMatDsptr aC;
    FColFMatDsptr pApE;
};

template<>
inline FMatFColDsptr EulerParameters<double>::ppApEpEtimesColumn(FColDsptr col)
{
    double a2c0 = 2 * col->at(0);
    double a2c1 = 2 * col->at(1);
    double a2c2 = 2 * col->at(2);
    double m2c0 = 0 - a2c0;
    double m2c1 = 0 - a2c1;
    double m2c2 = 0 - a2c2;
    auto col00 = std::make_shared<FullColumn<double>>(ListD {a2c0, m2c1, m2c2});
    auto col01 = std::make_shared<FullColumn<double>>(ListD {a2c1, a2c0, 0});
    auto col02 = std::make_shared<FullColumn<double>>(ListD {a2c2, 0, a2c0});
    auto col03 = std::make_shared<FullColumn<double>>(ListD {0, m2c2, a2c1});
    auto col11 = std::make_shared<FullColumn<double>>(ListD {m2c0, a2c1, m2c2});
    auto col12 = std::make_shared<FullColumn<double>>(ListD {0, a2c2, a2c1});
    auto col13 = std::make_shared<FullColumn<double>>(ListD {a2c2, 0, m2c0});
    auto col22 = std::make_shared<FullColumn<double>>(ListD {m2c0, m2c1, a2c2});
    auto col23 = std::make_shared<FullColumn<double>>(ListD {m2c1, a2c0, 0});
    auto col33 = std::make_shared<FullColumn<double>>(ListD {a2c0, a2c1, a2c2});
    auto answer = std::make_shared<FullMatrix<FColDsptr>>(4, 4);
    auto& row0 = answer->at(0);
    row0->at(0) = col00;
    row0->at(1) = col01;
    row0->at(2) = col02;
    row0->at(3) = col03;
    auto& row1 = answer->at(1);
    row1->at(0) = col01;
    row1->at(1) = col11;
    row1->at(2) = col12;
    row1->at(3) = col13;
    auto& row2 = answer->at(2);
    row2->at(0) = col02;
    row2->at(1) = col12;
    row2->at(2) = col22;
    row2->at(3) = col23;
    auto& row3 = answer->at(3);
    row3->at(0) = col03;
    row3->at(1) = col13;
    row3->at(2) = col23;
    row3->at(3) = col33;
    return answer;
}

template<>
inline FMatDsptr EulerParameters<double>::pCpEtimesColumn(FColDsptr col)
{
    //"col size = 4."
    auto c0 = col->at(0);
    auto c1 = col->at(1);
    auto c2 = col->at(2);
    auto mc0 = -c0;
    auto mc1 = -c1;
    auto mc2 = -c2;
    auto mc3 = -col->at(3);
    auto answer = std::make_shared<FullMatrix<double>>(3, 4);
    auto& row0 = answer->at(0);
    auto& row1 = answer->at(1);
    auto& row2 = answer->at(2);
    row0->atiput(0, mc3);
    row0->atiput(1, mc2);
    row0->atiput(2, c1);
    row0->atiput(3, c0);
    row1->atiput(0, c2);
    row1->atiput(1, mc3);
    row1->atiput(2, mc0);
    row1->atiput(3, c1);
    row2->atiput(0, mc1);
    row2->atiput(1, c0);
    row2->atiput(2, mc3);
    row2->atiput(3, c2);
    return answer;
}

template<typename T>
inline FMatDsptr EulerParameters<T>::pCTpEtimesColumn(FColDsptr col)
{
    //"col size = 3."
    auto c0 = col->at(0);
    auto c1 = col->at(1);
    auto c2 = col->at(2);
    auto mc0 = -c0;
    auto mc1 = -c1;
    auto mc2 = -c2;
    auto answer = std::make_shared<FullMatrix<double>>(4, 4);
    auto& row0 = answer->at(0);
    auto& row1 = answer->at(1);
    auto& row2 = answer->at(2);
    auto& row3 = answer->at(3);
    row0->atiput(0, 0.0);
    row0->atiput(1, c2);
    row0->atiput(2, mc1);
    row0->atiput(3, c0);
    row1->atiput(0, mc2);
    row1->atiput(1, 0.0);
    row1->atiput(2, c0);
    row1->atiput(3, c1);
    row2->atiput(0, c1);
    row2->atiput(1, mc0);
    row2->atiput(2, 0.0);
    row2->atiput(3, c2);
    row3->atiput(0, mc0);
    row3->atiput(1, mc1);
    row3->atiput(2, mc2);
    row3->atiput(3, 0.0);
    return answer;
}

template<>
inline FMatFMatDsptr EulerParameters<double>::ppApEpEtimesMatrix(FMatDsptr mat)
{
    FRowDsptr a2m0 = mat->at(0)->times(2.0);
    FRowDsptr a2m1 = mat->at(1)->times(2.0);
    FRowDsptr a2m2 = mat->at(2)->times(2.0);
    FRowDsptr m2m0 = a2m0->negated();
    FRowDsptr m2m1 = a2m1->negated();
    FRowDsptr m2m2 = a2m2->negated();
    FRowDsptr zero = std::make_shared<FullRow<double>>(3, 0.0);
    auto mat00 = std::make_shared<FullMatrix<double>>(ListFRD {a2m0, m2m1, m2m2});
    auto mat01 = std::make_shared<FullMatrix<double>>(ListFRD {a2m1, a2m0, zero});
    auto mat02 = std::make_shared<FullMatrix<double>>(ListFRD {a2m2, zero, a2m0});
    auto mat03 = std::make_shared<FullMatrix<double>>(ListFRD {zero, m2m2, a2m1});
    auto mat11 = std::make_shared<FullMatrix<double>>(ListFRD {m2m0, a2m1, m2m2});
    auto mat12 = std::make_shared<FullMatrix<double>>(ListFRD {zero, a2m2, a2m1});
    auto mat13 = std::make_shared<FullMatrix<double>>(ListFRD {a2m2, zero, m2m0});
    auto mat22 = std::make_shared<FullMatrix<double>>(ListFRD {m2m0, m2m1, a2m2});
    auto mat23 = std::make_shared<FullMatrix<double>>(ListFRD {m2m1, a2m0, zero});
    auto mat33 = std::make_shared<FullMatrix<double>>(ListFRD {a2m0, a2m1, a2m2});
    auto answer = std::make_shared<FullMatrix<FMatDsptr>>(4, 4);
    auto& row0 = answer->at(0);
    row0->at(0) = mat00;
    row0->at(1) = mat01;
    row0->at(2) = mat02;
    row0->at(3) = mat03;
    auto& row1 = answer->at(1);
    row1->at(0) = mat01;
    row1->at(1) = mat11;
    row1->at(2) = mat12;
    row1->at(3) = mat13;
    auto& row2 = answer->at(2);
    row2->at(0) = mat02;
    row2->at(1) = mat12;
    row2->at(2) = mat22;
    row2->at(3) = mat23;
    auto& row3 = answer->at(3);
    row3->at(0) = mat03;
    row3->at(1) = mat13;
    row3->at(2) = mat23;
    row3->at(3) = mat33;
    return answer;
}

template<>
inline void EulerParameters<double>::initialize()
{
    aA = FullMatrix<double>::identitysptr(3);
    aB = std::make_shared<FullMatrix<double>>(3, 4);
    aC = std::make_shared<FullMatrix<double>>(3, 4);
    pApE = std::make_shared<FullColumn<FMatDsptr>>(4);
    for (size_t i = 0; i < 4; i++) {
        pApE->at(i) = std::make_shared<FullMatrix<double>>(3, 3);
    }
}
template<typename T>
inline void EulerParameters<T>::calc()
{
    this->calcABC();
    this->calcpApE();
}
template<>
inline void EulerParameters<double>::calcABC()
{
    double aE0 = this->at(0);
    double aE1 = this->at(1);
    double aE2 = this->at(2);
    double aE3 = this->at(3);
    double mE0 = -aE0;
    double mE1 = -aE1;
    double mE2 = -aE2;
    FRowDsptr aBi;
    aBi = aB->at(0);
    aBi->at(0) = aE3;
    aBi->at(1) = mE2;
    aBi->at(2) = aE1;
    aBi->at(3) = mE0;
    aBi = aB->at(1);
    aBi->at(0) = aE2;
    aBi->at(1) = aE3;
    aBi->at(2) = mE0;
    aBi->at(3) = mE1;
    aBi = aB->at(2);
    aBi->at(0) = mE1;
    aBi->at(1) = aE0;
    aBi->at(2) = aE3;
    aBi->at(3) = mE2;
    FRowDsptr aCi;
    aCi = aC->at(0);
    aCi->at(0) = aE3;
    aCi->at(1) = aE2;
    aCi->at(2) = mE1;
    aCi->at(3) = mE0;
    aCi = aC->at(1);
    aCi->at(0) = mE2;
    aCi->at(1) = aE3;
    aCi->at(2) = aE0;
    aCi->at(3) = mE1;
    aCi = aC->at(2);
    aCi->at(0) = aE1;
    aCi->at(1) = mE0;
    aCi->at(2) = aE3;
    aCi->at(3) = mE2;

    aA = aB->timesTransposeFullMatrix(aC);
}
template<>
inline void EulerParameters<double>::calcpApE()
{
    double a2E0 = 2.0 * (this->at(0));
    double a2E1 = 2.0 * (this->at(1));
    double a2E2 = 2.0 * (this->at(2));
    double a2E3 = 2.0 * (this->at(3));
    double m2E0 = -a2E0;
    double m2E1 = -a2E1;
    double m2E2 = -a2E2;
    double m2E3 = -a2E3;
    FMatDsptr pApEk;
    pApEk = pApE->at(0);
    FRowDsptr pAipEk;
    pAipEk = pApEk->at(0);
    pAipEk->at(0) = a2E0;
    pAipEk->at(1) = a2E1;
    pAipEk->at(2) = a2E2;
    pAipEk = pApEk->at(1);
    pAipEk->at(0) = a2E1;
    pAipEk->at(1) = m2E0;
    pAipEk->at(2) = m2E3;
    pAipEk = pApEk->at(2);
    pAipEk->at(0) = a2E2;
    pAipEk->at(1) = a2E3;
    pAipEk->at(2) = m2E0;
    //
    pApEk = pApE->at(1);
    pAipEk = pApEk->at(0);
    pAipEk->at(0) = m2E1;
    pAipEk->at(1) = a2E0;
    pAipEk->at(2) = a2E3;
    pAipEk = pApEk->at(1);
    pAipEk->at(0) = a2E0;
    pAipEk->at(1) = a2E1;
    pAipEk->at(2) = a2E2;
    pAipEk = pApEk->at(2);
    pAipEk->at(0) = m2E3;
    pAipEk->at(1) = a2E2;
    pAipEk->at(2) = m2E1;
    //
    pApEk = pApE->at(2);
    pAipEk = pApEk->at(0);
    pAipEk->at(0) = m2E2;
    pAipEk->at(1) = m2E3;
    pAipEk->at(2) = a2E0;
    pAipEk = pApEk->at(1);
    pAipEk->at(0) = a2E3;
    pAipEk->at(1) = m2E2;
    pAipEk->at(2) = a2E1;
    pAipEk = pApEk->at(2);
    pAipEk->at(0) = a2E0;
    pAipEk->at(1) = a2E1;
    pAipEk->at(2) = a2E2;
    //
    pApEk = pApE->at(3);
    pAipEk = pApEk->at(0);
    pAipEk->at(0) = a2E3;
    pAipEk->at(1) = m2E2;
    pAipEk->at(2) = a2E1;
    pAipEk = pApEk->at(1);
    pAipEk->at(0) = a2E2;
    pAipEk->at(1) = a2E3;
    pAipEk->at(2) = m2E0;
    pAipEk = pApEk->at(2);
    pAipEk->at(0) = m2E1;
    pAipEk->at(1) = a2E0;
    pAipEk->at(2) = a2E3;
}
template<typename T>
inline void EulerParameters<T>::conditionSelf()
{
    EulerArray<T>::conditionSelf();
    this->normalizeSelf();
}
template<>
inline std::shared_ptr<EulerParameters<double>> EulerParameters<double>::times(double a)
{
    auto n = this->size();
    auto answer = std::make_shared<EulerParameters<double>>(n);
    for (size_t i = 0; i < n; i++) {
        answer->at(i) = this->at(i) * a;
    }
    return answer;
}
template<typename T>
inline std::shared_ptr<EulerParameters<T>> EulerParameters<T>::times(T)
{
    assert(false);
}
template<typename T>
inline std::shared_ptr<EulerParameters<T>>
EulerParameters<T>::plusFullColumn(std::shared_ptr<EulerParameters<T>> eulerPars)
{
    auto n = this->size();
    auto answer = std::make_shared<EulerParameters<T>>(n);
    for (size_t i = 0; i < n; i++) {
        answer->at(i) = this->at(i) + eulerPars->at(i);
    }
    return answer;
}
template<typename T>
inline std::shared_ptr<EulerParameters<T>> EulerParameters<T>::copy()
{
    auto n = this->size();
    auto answer = std::make_shared<EulerParameters<double>>(n);
    for (size_t i = 0; i < n; i++) {
        answer->at(i) = this->at(i);
    }
    return answer;
}
}  // namespace MbD
