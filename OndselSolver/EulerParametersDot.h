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
#include "EulerParameters.h"
//#include "CREATE.h" //Cannot use CREATE.h in subclasses of std::vector. Why?

namespace MbD {

	template<typename T>
	class EulerParametersDot : public EulerArray<T>
	{
		//qE aAdot aBdot aCdot pAdotpE
	public:
		EulerParametersDot(size_t count) : EulerArray<T>(count) {}
		EulerParametersDot(size_t count, const T& value) : EulerArray<T>(count, value) {}
		EulerParametersDot(std::initializer_list<T> list) : EulerArray<T>{ list } {}
		static std::shared_ptr<EulerParametersDot<T>> FromqEOpAndOmegaOpO(std::shared_ptr<EulerParameters<T>> qe, FColDsptr omeOpO);
		void initialize() override;
		void calcAdotBdotCdot();
		void calcpAdotpE();
		FMatDsptr aB();
		FColDsptr omeOpO();

		std::shared_ptr<EulerParameters<T>> qE;
		FMatDsptr aAdot, aBdot, aCdot;
		FColFMatDsptr pAdotpE;
		void calc() override;

	};

	template<typename T>
	inline std::shared_ptr<EulerParametersDot<T>> EulerParametersDot<T>::FromqEOpAndOmegaOpO(std::shared_ptr<EulerParameters<T>> qEOp, FColDsptr omeOpO)
	{
		//auto answer = CREATE<EulerParametersDot<T>>::With(4);	//Cannot use CREATE.h in subclasses of std::vector. Why?
		auto answer = std::make_shared<EulerParametersDot<T>>(4);
		answer->initialize();	
		qEOp->calcABC();
		auto aB = qEOp->aB;
		answer->equalFullColumn(aB->transposeTimesFullColumn(omeOpO->times(0.5)));
		answer->qE = qEOp;
		answer->calc();
		return answer;
	}

	template<typename T>
	inline void EulerParametersDot<T>::initialize()
	{
		aAdot = std::make_shared<FullMatrix<double>>(3, 3);
		aBdot = std::make_shared<FullMatrix<double>>(3, 4);
		aCdot = std::make_shared<FullMatrix<double>>(3, 4);
		pAdotpE = std::make_shared<FullColumn<FMatDsptr>>(4);
		for (size_t i = 0; i < 4; i++)
		{
			pAdotpE->at(i) = std::make_shared<FullMatrix<double>>(3, 3);
		}
	}

	template<typename T>
	inline void EulerParametersDot<T>::calcAdotBdotCdot()
	{
		//"aAdot, aBdot and aCdot are all calculated together and only here."
		auto aE0dot = this->at(0);
		auto aE1dot = this->at(1);
		auto aE2dot = this->at(2);
		auto aE3dot = this->at(3);
		auto mE0dot = -aE0dot;
		auto mE1dot = -aE1dot;
		auto mE2dot = -aE2dot;
		aBdot->at(0)->at(0) = aE3dot;
		aBdot->at(0)->at(1) = mE2dot;
		aBdot->at(0)->at(2) = aE1dot;
		aBdot->at(0)->at(3) = mE0dot;
		aBdot->at(1)->at(0) = aE2dot;
		aBdot->at(1)->at(1) = aE3dot;
		aBdot->at(1)->at(2) = mE0dot;
		aBdot->at(1)->at(3) = mE1dot;
		aBdot->at(2)->at(0) = mE1dot;
		aBdot->at(2)->at(1) = aE0dot;
		aBdot->at(2)->at(2) = aE3dot;
		aBdot->at(2)->at(3) = mE2dot;
		aCdot->at(0)->at(0) = aE3dot;
		aCdot->at(0)->at(1) = aE2dot;
		aCdot->at(0)->at(2) = mE1dot;
		aCdot->at(0)->at(3) = mE0dot;
		aCdot->at(1)->at(0) = mE2dot;
		aCdot->at(1)->at(1) = aE3dot;
		aCdot->at(1)->at(2) = aE0dot;
		aCdot->at(1)->at(3) = mE1dot;
		aCdot->at(2)->at(0) = aE1dot;
		aCdot->at(2)->at(1) = mE0dot;
		aCdot->at(2)->at(2) = aE3dot;
		aCdot->at(2)->at(3) = mE2dot;
		aAdot = this->aB()->timesTransposeFullMatrix(aCdot)->times(2.0);
	}

	template<typename T>
	inline void EulerParametersDot<T>::calcpAdotpE()
	{
		//"Mimic calcpApE"
		//"All aE's are actually aEdot's."
		double a2E0 = 2.0 * (this->at(0));
		double a2E1 = 2.0 * (this->at(1));
		double a2E2 = 2.0 * (this->at(2));
		double a2E3 = 2.0 * (this->at(3));
		double m2E0 = -a2E0;
		double m2E1 = -a2E1;
		double m2E2 = -a2E2;
		double m2E3 = -a2E3;
		FMatDsptr pApEk;
		pApEk = pAdotpE->at(0);
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
		pApEk = pAdotpE->at(1);
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
		pApEk = pAdotpE->at(2);
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
		pApEk = pAdotpE->at(3);
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
	inline FMatDsptr EulerParametersDot<T>::aB()
	{
		return qE->aB;
	}

	template<typename T>
	inline FColDsptr EulerParametersDot<T>::omeOpO()
	{
		auto aaa = this->aB();
//		auto bbb = aaa->timesFullColumn((MbD::FColsptr<double>)this);
		auto bbb = aaa->timesFullColumn(this);
		auto ccc = bbb->times(2.0);
		return ccc;
		//return this->aB->timesFullColumn(this)->times(2.0);
	}

	template<typename T>
	inline void EulerParametersDot<T>::calc()
	{
		qE->calc();
		this->calcAdotBdotCdot();
		this->calcpAdotpE();
	}
}

