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
		void setRotOrder(int i, int j, int k);

		std::shared_ptr<FullColumn<int>> rotOrder;
		FColFMatDsptr cA;
		FMatDsptr aA;
	};
    template class EulerAngles<std::shared_ptr<MbD::Symbolic>>;
    template class EulerAngles<double>;
    std::shared_ptr<EulerAnglesDot<std::shared_ptr<MbD::Symbolic>>> differentiateWRT(EulerAngles<std::shared_ptr<MbD::Symbolic>>& ref, std::shared_ptr<MbD::Symbolic> var);
    template<typename T>

    void EulerAngles<T>::setRotOrder(int i, int j, int k)
    {
        rotOrder = std::make_shared<FullColumn<int>>(3);
        rotOrder->at(0) = i;
        rotOrder->at(1) = j;
        rotOrder->at(2) = k;
    }
}

