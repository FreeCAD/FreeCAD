/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "EulerArray.h"
#include "FullMatrix.h"

namespace MbD {
	template<typename T>
	class EulerAngles;
	template<typename T>
	class EulerAnglesDot;

	template<typename T>
	class EulerAnglesDDot : public EulerArray<T>
	{
		//aEulerAnglesDot cAddot aAddot alpF alpf 
	public:
		EulerAnglesDDot() : EulerArray<T>(3) {}
		void calc() override;

		EulerAnglesDot<T>* aEulerAnglesDot; //Use raw pointer to point backwards
		FColFMatDsptr cAddot;
		FMatDsptr aAddot;
		FColDsptr alpF, alpf;
		void aEulerAngles(EulerAngles<T>* eulerAngles);
	};
	template<typename T>
	inline void EulerAnglesDDot<T>::calc()
	{
		aEulerAnglesDot->calc();
		auto aEulerAngles = aEulerAnglesDot->aEulerAngles;
		auto rotOrder = aEulerAngles->rotOrder;
		auto cA = aEulerAngles->cA;
		auto cAdot = aEulerAnglesDot->cAdot;
		cAddot = std::make_shared<FullColumn<FMatDsptr>>(3);
		for (size_t i = 0; i < 3; i++)
		{
			auto axis = rotOrder->at(i);
			auto angle = aEulerAngles->at(i)->getValue();
			auto angleDot = aEulerAnglesDot->at(i)->getValue();
			auto angleDDot = this->at(i)->getValue();
			if (axis == 1) {
				cAddot->atiput(i, FullMatrix<double>::rotatexrotDotrotDDot(angle, angleDot, angleDDot));
			}
			else if (axis == 2) {
				cAddot->atiput(i, FullMatrix<double>::rotateyrotDotrotDDot(angle, angleDot, angleDDot));
			}
			else if (axis == 3) {
				cAddot->atiput(i, FullMatrix<double>::rotatezrotDotrotDDot(angle, angleDot, angleDDot));
			}
			else {
				throw std::runtime_error("Euler angle rotation order must be any permutation of 1,2,3 without consecutive repeats.");
			}
		}
		auto phiA = cA->at(0);
		auto theA = cA->at(1);
		auto psiA = cA->at(2);
		auto phiAdot = cAdot->at(0);
		auto theAdot = cAdot->at(1);
		auto psiAdot = cAdot->at(2);
		auto phiAddot = cAddot->at(0);
		auto theAddot = cAddot->at(1);
		auto psiAddot = cAddot->at(2);

		auto term = phiAddot->timesFullMatrix(theA->timesFullMatrix(psiA));
		auto term1 = phiAdot->timesFullMatrix(theAdot->timesFullMatrix(psiA));
		auto term2 = phiAdot->timesFullMatrix(theA->timesFullMatrix(psiAdot));
		auto term3 = phiAdot->timesFullMatrix(theAdot->timesFullMatrix(psiA));
		auto term4 = phiA->timesFullMatrix(theAddot->timesFullMatrix(psiA));
		auto term5 = phiA->timesFullMatrix(theAdot->timesFullMatrix(psiAdot));
		auto term6 = phiAdot->timesFullMatrix(theA->timesFullMatrix(psiAdot));
		auto term7 = phiA->timesFullMatrix(theAdot->timesFullMatrix(psiAdot));
		auto term8 = phiA->timesFullMatrix(theA->timesFullMatrix(psiAddot));

		aAddot = term->plusFullMatrix(term1)->plusFullMatrix(term2)
                ->plusFullMatrix(term3)->plusFullMatrix(term4)
			    ->plusFullMatrix(term5)->plusFullMatrix(term6)
                ->plusFullMatrix(term7)->plusFullMatrix(term8);
	}
	template<typename T>
	inline void EulerAnglesDDot<T>::aEulerAngles(EulerAngles<T>* eulerAngles)
	{
		aEulerAnglesDot->aEulerAngles = eulerAngles;
	}
}

