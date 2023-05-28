#pragma once
#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {

	template <typename T>
	class EulerParameters : public EulerArray<T>
	{
		//aA aB aC pApE
	public:
		EulerParameters() : EulerArray<T>(4) {}
		EulerParameters(size_t count) : EulerArray<T>(count) {}
		EulerParameters(size_t count, const T& value) : EulerArray<T>(count, value) {}
		EulerParameters(std::initializer_list<T> list) : EulerArray<T>{ list } {}
		double sumOfSquares();

		static std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<T>>>> ppApEpEtimesColumn(FColDsptr col);
		static std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<T>>>> ppApEpEtimesMatrix(FMatDsptr mat);

		void initialize();
		void calcABC();
		void calcpApE();

		FMatDsptr aA;
		FMatDsptr aB;
		FMatDsptr aC;
		FColFMatDsptr pApE;
	};

	template<typename T>
	inline double EulerParameters<T>::sumOfSquares()
	{
		return 0.0;
	}

	template<>
	inline std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> EulerParameters<double>::ppApEpEtimesColumn(FColDsptr col)
	{
		double a2c1 = 2 * col->at(0);
		double a2c2 = 2 * col->at(1);
		double a2c3 = 2 * col->at(2);
		double m2c1 = 0 - a2c1;
		double m2c2 = 0 - a2c2;
		double m2c3 = 0 - a2c3;
		auto col11 = std::make_shared<FullColumn<double>>(ListD{ a2c1, m2c2, m2c3 });
		auto col12 = std::make_shared<FullColumn<double>>(ListD{ a2c2, a2c1, 0 });
		auto col13 = std::make_shared<FullColumn<double>>(ListD{ a2c3, 0, a2c1 });
		auto col14 = std::make_shared<FullColumn<double>>(ListD{ 0, m2c3, a2c2 });
		auto col22 = std::make_shared<FullColumn<double>>(ListD{ m2c1, a2c2, m2c3 });
		auto col23 = std::make_shared<FullColumn<double>>(ListD{ 0, a2c3, a2c2 });
		auto col24 = std::make_shared<FullColumn<double>>(ListD{ a2c3, 0, m2c1 });
		auto col33 = std::make_shared<FullColumn<double>>(ListD{ m2c1, m2c2, a2c3 });
		auto col34 = std::make_shared<FullColumn<double>>(ListD{ m2c2, a2c1, 0 });
		auto col44 = std::make_shared<FullColumn<double>>(ListD{ a2c1, a2c2, a2c3 });
		auto answer = std::make_shared<FullMatrix<std::shared_ptr<FullColumn<double>>>>(4, 4);
		auto& row1 = answer->at(0);
		row1->at(0) = col11;
		row1->at(1) = col12;
		row1->at(2) = col13;
		row1->at(3) = col14;
		auto& row2 = answer->at(1);
		row2->at(0) = col12;
		row2->at(1) = col22;
		row2->at(2) = col23;
		row2->at(3) = col24;
		auto& row3 = answer->at(2);
		row3->at(0) = col13;
		row3->at(1) = col23;
		row3->at(2) = col33;
		row3->at(3) = col34;
		auto& row4 = answer->at(3);
		row4->at(0) = col14;
		row4->at(1) = col24;
		row4->at(2) = col34;
		row4->at(3) = col44;
		return answer;
	}

	template<>
	inline std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>> EulerParameters<double>::ppApEpEtimesMatrix(FMatDsptr mat)
	{
		FRowDsptr a2m1 = mat->at(0)->times(2.0);
		FRowDsptr a2m2 = mat->at(1)->times(2.0);
		FRowDsptr a2m3 = mat->at(2)->times(2.0);
		FRowDsptr m2m1 = a2m1->negated();
		FRowDsptr m2m2 = a2m2->negated();
		FRowDsptr m2m3 = a2m3->negated();
		FRowDsptr zero = std::make_shared<FullRow<double>>(3, 0.0);
		auto mat11 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m1, m2m2, m2m3 });
		auto mat12 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m2, a2m1, zero });
		auto mat13 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m3, zero, a2m1 });
		auto mat14 = std::make_shared<FullMatrix<double>>(ListFRD{ zero, m2m3, a2m2 });
		auto mat22 = std::make_shared<FullMatrix<double>>(ListFRD{ m2m1, a2m2, m2m3 });
		auto mat23 = std::make_shared<FullMatrix<double>>(ListFRD{ zero, a2m3, a2m2 });
		auto mat24 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m3, zero, m2m1 });
		auto mat33 = std::make_shared<FullMatrix<double>>(ListFRD{ m2m1, m2m2, a2m3 });
		auto mat34 = std::make_shared<FullMatrix<double>>(ListFRD{ m2m2, a2m1, zero });
		auto mat44 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m1, a2m2, a2m3 });
		auto answer = std::make_shared<FullMatrix<std::shared_ptr<FullMatrix<double>>>>(4, 4);
		auto& row1 = answer->at(0);
		row1->at(0) = mat11;
		row1->at(1) = mat12;
		row1->at(2) = mat13;
		row1->at(3) = mat14;
		auto& row2 = answer->at(1);
		row2->at(0) = mat12;
		row2->at(1) = mat22;
		row2->at(2) = mat23;
		row2->at(3) = mat24;
		auto& row3 = answer->at(2);
		row3->at(0) = mat13;
		row3->at(1) = mat23;
		row3->at(2) = mat33;
		row3->at(3) = mat34;
		auto& row4 = answer->at(3);
		row4->at(0) = mat14;
		row4->at(1) = mat24;
		row4->at(2) = mat34;
		row4->at(3) = mat44;
		return answer;
	}

	template<>
	inline void EulerParameters<double>::initialize()
	{
		aA = std::make_shared<FullMatrix<double>>(3, 3);
		aB = std::make_shared<FullMatrix<double>>(3, 4);
		aC = std::make_shared<FullMatrix<double>>(3, 4);
		pApE = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
		for (size_t i = 0; i < 4; i++)
		{
			pApE->at(i) = std::make_shared<FullMatrix<double>>(3, 3);
		}
	}
	template<>
	inline void EulerParameters<double>::calcABC()
	{
		double aE0 = this->at(0);
		double aE1 = this->at(1);
		double aE2 = this->at(2);
		double aE3 = this->at(3);
		double mE0 = -aE0;
		double mE1 = -aE1;
		double mE2 = -aE2;
		FRowDsptr aBi;
		aBi = aB->at(0);
		aBi->at(0) = aE3;
		aBi->at(1) = mE2;
		aBi->at(2) = aE1;
		aBi->at(3) = mE0;
		aBi = aB->at(1);
		aBi->at(0) = aE2;
		aBi->at(1) = aE3;
		aBi->at(2) = mE0;
		aBi->at(3) = mE1;
		aBi = aB->at(2);
		aBi->at(0) = mE1;
		aBi->at(1) = aE0;
		aBi->at(2) = aE3;
		aBi->at(3) = mE2;
		FRowDsptr aCi;
		aCi = aC->at(0);
		aCi->at(0) = aE3;
		aCi->at(1) = aE2;
		aCi->at(2) = mE1;
		aCi->at(3) = mE0;
		aCi = aC->at(1);
		aCi->at(0) = mE2;
		aCi->at(1) = aE3;
		aCi->at(2) = aE0;
		aCi->at(3) = mE1;
		aCi = aC->at(2);
		aCi->at(0) = aE1;
		aCi->at(1) = mE0;
		aCi->at(2) = aE3;
		aCi->at(3) = mE2;

		aA = aB->timesTransposeFullMatrix(aC);
	}
	template<>
	inline void EulerParameters<double>::calcpApE()
	{
		double a2E0 = 2.0*(this->at(0));
		double a2E1  = 2.0*(this->at(1));
		double a2E2  = 2.0*(this->at(2));
		double a2E3  = 2.0*(this->at(3));
		double m2E0 = -a2E0;
		double m2E1 = -a2E1;
		double m2E2 = -a2E2;
		double m2E3 = -a2E3;
		FMatDsptr pApEk;
		pApEk = pApE->at(0);
		FRowDsptr pAipEk;
		pAipEk = pApEk->at(0);
		pAipEk->at(0) = a2E0;
		pAipEk->at(1) = a2E1;
		pAipEk->at(2) = a2E2;
		pAipEk = pApEk->at(1);
		pAipEk->at(0) = a2E1;
		pAipEk->at(1) = m2E0;
		pAipEk->at(2) = m2E3;
		pAipEk = pApEk->at(2);
		pAipEk->at(0) = a2E2;
		pAipEk->at(1) = a2E3;
		pAipEk->at(2) = m2E0;
		//
		pApEk = pApE->at(1);
		pAipEk = pApEk->at(0);
		pAipEk->at(0) = m2E1;
		pAipEk->at(1) = a2E0;
		pAipEk->at(2) = a2E3;
		pAipEk = pApEk->at(1);
		pAipEk->at(0) = a2E0;
		pAipEk->at(1) = a2E1;
		pAipEk->at(2) = a2E2;
		pAipEk = pApEk->at(2);
		pAipEk->at(0) = m2E3;
		pAipEk->at(1) = a2E2;
		pAipEk->at(2) = m2E1;
		//
		pApEk = pApE->at(2);
		pAipEk = pApEk->at(0);
		pAipEk->at(0) = m2E2;
		pAipEk->at(1) = m2E3;
		pAipEk->at(2) = a2E0;
		pAipEk = pApEk->at(1);
		pAipEk->at(0) = a2E3;
		pAipEk->at(1) = m2E2;
		pAipEk->at(2) = a2E1;
		pAipEk = pApEk->at(2);
		pAipEk->at(0) = a2E0;
		pAipEk->at(1) = a2E1;
		pAipEk->at(2) = a2E2;
		//
		pApEk = pApE->at(3);
		pAipEk = pApEk->at(0);
		pAipEk->at(0) = a2E3;
		pAipEk->at(1) = m2E2;
		pAipEk->at(2) = a2E1;
		pAipEk = pApEk->at(1);
		pAipEk->at(0) = a2E2;
		pAipEk->at(1) = a2E3;
		pAipEk->at(2) = m2E0;
		pAipEk = pApEk->at(2);
		pAipEk->at(0) = m2E1;
		pAipEk->at(1) = a2E0;
		pAipEk->at(2) = a2E3;
	}
}

