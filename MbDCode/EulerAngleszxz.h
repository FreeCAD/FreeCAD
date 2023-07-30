#pragma once

#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {

	template<typename T>
	class EulerAngleszxz : public EulerArray<T>
	{
		//phiA theA psiA aA 
		//Used by EndFrameqct
	public:
		EulerAngleszxz() : EulerArray<T>(3) {}
		void initialize() override;
		void calc() override;

		FMatDsptr phiA, theA, psiA, aA;
	};
	template<typename T>
	inline void EulerAngleszxz<T>::initialize()
	{
		phiA = std::make_shared<FullMatrix<double>>(3, 3);
		phiA->identity();
		theA = std::make_shared<FullMatrix<double>>(3, 3);
		theA->identity();
		psiA = std::make_shared<FullMatrix<double>>(3, 3);
		psiA->identity();
	}
	template<typename T>
	inline void EulerAngleszxz<T>::calc()
	{
		double zero = 0.0;
		double phi = this->at(0);
		double sphi = sin(phi);
		double cphi = cos(phi);
		double the = this->at(1);
		double sthe = sin(the);
		double cthe = cos(the);
		double psi = this->at(2);
		double spsi = sin(psi);
		double cpsi = cos(psi);
		FRowDsptr phiAi;
		phiAi = phiA->at(0);
		phiAi->at(0) = cphi;
		phiAi->at(1) = -sphi;
		phiAi = phiA->at(1);
		phiAi->at(0) = sphi;
		phiAi->at(1) = cphi;
		FRowDsptr theAi;
		theAi = theA->at(1);
		theAi->at(1) = cthe;
		theAi->at(2) = -sthe;
		theAi = theA->at(2);
		theAi->at(1) = sthe;
		theAi->at(2) = cthe;
		FRowDsptr psiAi;
		psiAi = psiA->at(0);
		psiAi->at(0) = cpsi;
		psiAi->at(1) = -spsi;
		psiAi = psiA->at(1);
		psiAi->at(0) = spsi;
		psiAi->at(1) = cpsi;
		aA = phiA->timesFullMatrix(theA->timesFullMatrix(psiA));
	}
}

