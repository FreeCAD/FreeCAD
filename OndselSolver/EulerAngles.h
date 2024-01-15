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
#include "EulerAnglesDot.h"
#include "Symbolic.h"

namespace MbD {
	//template<typename T>
	//class EulerAnglesDot;

	template<typename T>
	class EulerAngles : public EulerArray<T>
	{
		//rotOrder cA aA 
		//Used for user input.
	public:
		EulerAngles() : EulerArray<T>(3) {}
		EulerAngles(std::initializer_list<T> list) : EulerArray<T>{ list } {}
		void initialize() override;
		void calc() override;
		std::shared_ptr<EulerAnglesDot<T>> differentiateWRT(T var);
		void setRotOrder(size_t i, size_t j, size_t k);

		std::shared_ptr<std::vector<size_t>> rotOrder;
		FColFMatDsptr cA;
		FMatDsptr aA;

	};
	template<typename T>
	inline void EulerAngles<T>::initialize()
	{
		assert(false);
	}
	template<>
	inline void EulerAngles<Symsptr>::calc()
	{
		cA = std::make_shared<FullColumn<FMatDsptr>>(3);
		for (size_t i = 0; i < 3; i++)
		{
			auto axis = rotOrder->at(i);
			auto angle = this->at(i)->getValue();
			if (axis == 1) {
				cA->atiput(i, FullMatrix<double>::rotatex(angle));
			}
			else if (axis == 2) {
				cA->atiput(i, FullMatrix<double>::rotatey(angle));
			}
			else if (axis == 3) {
				cA->atiput(i, FullMatrix<double>::rotatez(angle));
			}
			else {
				throw std::runtime_error("Euler angle rotation order must be any permutation of 1,2,3 without consecutive repeats.");
			}
		}
		aA = cA->at(0)->timesFullMatrix(cA->at(1)->timesFullMatrix(cA->at(2)));
	}
	template<>
	inline void EulerAngles<double>::calc()
	{
		cA = std::make_shared<FullColumn<FMatDsptr>>(3);
		for (size_t i = 0; i < 3; i++)
		{
			auto axis = rotOrder->at(i);
			auto angle = this->at(i);
			if (axis == 1) {
				cA->atiput(i, FullMatrix<double>::rotatex(angle));
			}
			else if (axis == 2) {
				cA->atiput(i, FullMatrix<double>::rotatey(angle));
			}
			else if (axis == 3) {
				cA->atiput(i, FullMatrix<double>::rotatez(angle));
			}
			else {
				throw std::runtime_error("Euler angle rotation order must be any permutation of 1,2,3 without consecutive repeats.");
			}
		}
		aA = cA->at(0)->timesFullMatrix(cA->at(1)->timesFullMatrix(cA->at(2)));
	}
	template<typename T>
	inline void EulerAngles<T>::calc()
	{
		assert(false);
	}
	template<typename T>
	inline std::shared_ptr<EulerAnglesDot<T>> EulerAngles<T>::differentiateWRT(T var)
	{
		auto derivatives = std::make_shared<EulerAnglesDot<T>>();
		std::transform(this->begin(), this->end(), derivatives->begin(),
			[var](T term) { return term->differentiateWRT(var); }
		);
		derivatives->aEulerAngles = this;
		return derivatives;
	}
	template<typename T>
	inline void EulerAngles<T>::setRotOrder(size_t i, size_t j, size_t k)
	{
		rotOrder = std::make_shared<std::vector<size_t>>(3);
		rotOrder->at(0) = i;
		rotOrder->at(1) = j;
		rotOrder->at(2) = k;
	}
}

