/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Function.h"

namespace MbD {
	class Symbolic;
	//using Symsptr = Symsptr;

	class FunctionX : public Function
	{
		//
	public:
		FunctionX() = default;
		FunctionX(Symsptr arg);
		void arguments(Symsptr args) override;
		virtual Symsptr copyWith(Symsptr arg);
		Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
		Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
		Symsptr differentiateWRT(Symsptr var) override;
		virtual Symsptr differentiateWRTx();
		void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
		double getValue() override;
		double getValue(double arg) override;
		bool isConstant() override;

		Symsptr xx;



	};
}

