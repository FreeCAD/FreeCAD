/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FullColumn.ref.h"
#include "FullMatrix.ref.h"
#include "EulerParameters.ref.h"
#include "EulerArray.h"
// #include "FullColumn.h"
// #include "FullMatrix.h"

namespace MbD {

	template<typename T>
	class EulerParameters : public EulerArray<T>
	{
		//Quarternion = {q0, q1, q2, q3}
		//EulerParameters = {qE1, qE2, qE3, qE4} is preferred because Smalltalk uses one-based indexing.
		// q0 = qE4
		//Note: It is tempting to use quarternions in C++ because of zero-based indexing.
		//Note: But that will make it harder to compare computation results with Smalltalk
		//aA aB aC pApE
	public:
		EulerParameters() : EulerArray<T>(4) {}
		EulerParameters(int count) : EulerArray<T>(count) {}
		EulerParameters(int count, const T& value) : EulerArray<T>(count, value) {}
		EulerParameters(std::initializer_list<T> list) : EulerArray<T>{ list } {}
		EulerParameters(FColDsptr axis, double theta) : EulerArray<T>(4) {
			auto halfTheta = theta / 2.0;
			auto sinHalfTheta = std::sin(halfTheta);
			auto cosHalfTheta = std::cos(halfTheta);
			axis->normalizeSelf();
			this->atiputFullColumn(0, axis->times(sinHalfTheta));
			this->atiput(3, cosHalfTheta);
			this->conditionSelf();
			this->initialize();
			this->calc();
		}

		static std::shared_ptr<FullMatrixFullColumnDouble> ppApEpEtimesColumn(FColDsptr col);
		static FMatDsptr pCpEtimesColumn(FColDsptr col);
		static FMatDsptr pCTpEtimesColumn(FColDsptr col);
		static std::shared_ptr<FullMatrixFullMatrixDouble> ppApEpEtimesMatrix(FMatDsptr mat);


		void initialize() override;
		void calc() override;
		void calcABC();
		void calcpApE();
		void conditionSelf() override;

		FMatDsptr aA;
		FMatDsptr aB;
		FMatDsptr aC;
		FColFMatDsptr pApE;
	};
}
