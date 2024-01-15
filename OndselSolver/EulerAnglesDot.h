/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <algorithm>

#include "EulerArray.h"
#include "FullMatrix.h"

namespace MbD {
	template<typename T>
	class EulerAngles;
	template<typename T>
	class EulerAnglesDDot;

	template<typename T>
	class EulerAnglesDot : public EulerArray<T>
	{
		//aEulerAngles cAdot aAdot omeF omef 
	public:
		EulerAnglesDot() : EulerArray<T>(3) {}
		std::shared_ptr<EulerAnglesDDot<T>> differentiateWRT(T var);
		void calc() override;

		EulerAngles<T>* aEulerAngles = nullptr; //Use raw pointer to point backwards
		FColFMatDsptr cAdot;
		FMatDsptr aAdot;
		FColDsptr omeF, omef;
	};
	template<typename T>
	inline std::shared_ptr<EulerAnglesDDot<T>> EulerAnglesDot<T>::differentiateWRT(T var)
	{
		auto derivatives = std::make_shared<EulerAnglesDDot<T>>();
		std::transform(this->begin(), this->end(), derivatives->begin(),
			[var](T term) { return term->differentiateWRT(var); }
		);
		derivatives->aEulerAnglesDot = this;
		return derivatives;
	}
	template<typename T>
	inline void EulerAnglesDot<T>::calc()
	{
		aEulerAngles->calc();
		auto& rotOrder = aEulerAngles->rotOrder;
		auto& cA = aEulerAngles->cA;
		cAdot = std::make_shared<FullColumn<FMatDsptr>>(3);
		for (size_t i = 0; i < 3; i++)
		{
			auto axis = rotOrder->at(i);
			auto angle = aEulerAngles->at(i)->getValue();
			auto angleDot = this->at(i)->getValue();
			if (axis == 1) {
				cAdot->atiput(i, FullMatrix<double>::rotatexrotDot(angle, angleDot));
			}
			else if (axis == 2) {
				cAdot->atiput(i, FullMatrix<double>::rotateyrotDot(angle, angleDot));
			}
			else if (axis == 3) {
				cAdot->atiput(i, FullMatrix<double>::rotatezrotDot(angle, angleDot));
			}
			else {
				throw std::runtime_error("Euler angle rotation order must be any permutation of 1,2,3 without consecutive repeats.");
			}
		}
		auto phidot = this->at(0)->getValue();
		auto thedot = this->at(1)->getValue();
		auto psidot = this->at(2)->getValue();
		auto& phiA = cA->at(0);
		auto& theA = cA->at(1);
		auto& psiA = cA->at(2);
		auto& phiAdot = cAdot->at(0);
		auto& theAdot = cAdot->at(1);
		auto& psiAdot = cAdot->at(2);

		aAdot = phiAdot->timesFullMatrix(theA->timesFullMatrix(psiA))
                ->plusFullMatrix(phiA->timesFullMatrix(theAdot->timesFullMatrix(psiA)))
                ->plusFullMatrix(phiA->timesFullMatrix(theA->timesFullMatrix(psiAdot)));
		omeF = (phiA->column(0)->times(phidot)
                ->plusFullColumn(phiA->timesFullMatrix(theA)->column(1)->times(thedot))
                ->plusFullColumn(aEulerAngles->aA->column(2)->times(psidot)));
		omef = aEulerAngles->aA->transposeTimesFullColumn(omeF);
	}
}

