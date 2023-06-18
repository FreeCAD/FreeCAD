#pragma once

#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EulerParameters.h"
#include "CREATE.h"

namespace MbD {

	template<typename T>
	class EulerParametersDot : public EulerArray<T>
	{
		//qE aAdot aBdot aCdot pAdotpE
	public:
		EulerParametersDot(int count) : EulerArray<T>(count) {}
		EulerParametersDot(int count, const T& value) : EulerArray<T>(count, value) {}
		EulerParametersDot(std::initializer_list<T> list) : EulerArray<T>{ list } {}
		static std::shared_ptr<EulerParametersDot<T>> FromqEOpAndOmegaOpO(std::shared_ptr<EulerParameters<T>> qe, FColDsptr omeOpO);
		void calcAdotBdotCdot();
		void calcpAdotpE();		

		std::shared_ptr<EulerParameters<T>> qE;
		FMatDsptr aAdot, aBdot, aCdot;
		FColFMatDsptr pAdotpE;
		void calc();

	};

	template<typename T>
	inline std::shared_ptr<EulerParametersDot<T>> EulerParametersDot<T>::FromqEOpAndOmegaOpO(std::shared_ptr<EulerParameters<T>> qEOp, FColDsptr omeOpO)
	{
		auto answer = CREATE<EulerParametersDot<T>>::With(4);
		qEOp->calcABC();
		auto aB = qEOp->aB;
		answer->equalFullColumn(aB->transposeTimesFullColumn(omeOpO->times(0.5)));
		answer->qE = qEOp;
		answer->calc();
		return answer;
	}

	template<typename T>
	inline void EulerParametersDot<T>::calcAdotBdotCdot()
	{
		assert(false);
	}

	template<typename T>
	inline void EulerParametersDot<T>::calcpAdotpE()
	{
		assert(false);
	}

	template<typename T>
	inline void EulerParametersDot<T>::calc()
	{
		qE->calc();
		this->calcAdotBdotCdot();
		this->calcpAdotpE();
	}
}

