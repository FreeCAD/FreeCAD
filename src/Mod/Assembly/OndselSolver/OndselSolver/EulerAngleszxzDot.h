/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "EulerArray.h"
#include "EulerAngleszxz.h"

namespace MbD {

	template<typename T>
	class EulerAngleszxzDot : public EulerArray<T>
	{
		//phiThePsi phiAdot theAdot psiAdot aAdot 
	public:
		EulerAngleszxzDot() : EulerArray<T>(3) {}
		void initialize() override;
		void calc() override;

		std::shared_ptr<EulerAngleszxz<T>> phiThePsi;
		FMatDsptr phiAdot, theAdot, psiAdot, aAdot;
	};
	template<typename T>
	inline void EulerAngleszxzDot<T>::initialize()
	{
		phiAdot = std::make_shared<FullMatrix<double>>(3, 3);
		phiAdot->zeroSelf();
		theAdot = std::make_shared<FullMatrix<double>>(3, 3);
		theAdot->zeroSelf();
		psiAdot = std::make_shared<FullMatrix<double>>(3, 3);
		psiAdot->zeroSelf();
	}
	template<typename T>
	inline void EulerAngleszxzDot<T>::calc()
	{
		auto phi = phiThePsi->at(0);
		auto sphi = std::sin(phi);
		auto cphi = std::cos(phi);
		auto phidot = this->at(0);
		auto minussphiTimesphidot = -(sphi * phidot);
		auto cphiTimesphidot = cphi * phidot;
		auto the = phiThePsi->at(1);
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto thedot = this->at(1);
		auto minusstheTimesthedot = -(sthe * thedot);
		auto ctheTimesthedot = cthe * thedot;
		auto psi = phiThePsi->at(2);
		auto spsi = std::sin(psi);
		auto cpsi = std::cos(psi);
		auto psidot = this->at(2);
		auto minusspsiTimespsidot = -(spsi * psidot);
		auto cpsiTimespsidot = cpsi * psidot;
		phiAdot->at(0)->at(0) = minussphiTimesphidot;
		phiAdot->at(0)->at(1) = -cphiTimesphidot;
		phiAdot->at(1)->at(0) = cphiTimesphidot;
		phiAdot->at(1)->at(1) = minussphiTimesphidot;
		theAdot->at(1)->at(1) = minusstheTimesthedot;
		theAdot->at(1)->at(2) = -ctheTimesthedot;
		theAdot->at(2)->at(1) = ctheTimesthedot;
		theAdot->at(2)->at(2) = minusstheTimesthedot;
		psiAdot->at(0)->at(0) = minusspsiTimespsidot;
		psiAdot->at(0)->at(1) = -cpsiTimespsidot;
		psiAdot->at(1)->at(0) = cpsiTimespsidot;
		psiAdot->at(1)->at(1) = minusspsiTimespsidot;
		auto phiA = phiThePsi->phiA;
		auto theA = phiThePsi->theA;
		auto psiA = phiThePsi->psiA;
		auto term1 = phiAdot->timesFullMatrix(theA->timesFullMatrix(psiA));
		auto term2 = phiA->timesFullMatrix(theAdot->timesFullMatrix(psiA));
		auto term3 = phiA->timesFullMatrix(theA->timesFullMatrix(psiAdot));
		aAdot = (term1->plusFullMatrix(term2))->plusFullMatrix(term3);
	}
}

