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
		Symsptr differentiateWRT(Symsptr var) override;
		virtual Symsptr differentiateWRTx();
		void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

		Symsptr xx;



	};
}

