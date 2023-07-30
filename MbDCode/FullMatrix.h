#pragma once

#include <corecrt_math_defines.h>
#include <memory>

#include "RowTypeMatrix.h"
//#include "CREATE.h" //Use forward declaration. //Cannot use CREATE.h in subclasses of std::vector. Why?

namespace MbD {
	template<typename T>
	class FullColumn;
	template<typename T>
	class FullRow;
	template<typename T>
	class EulerParameters;

	template<typename T>
	class FullMatrix : public RowTypeMatrix<std::shared_ptr<FullRow<T>>>
	{
	public:
		FullMatrix() {}
		FullMatrix(int m) : RowTypeMatrix<std::shared_ptr<FullRow<T>>>(m)
		{
		}
		FullMatrix(int m, int n) {
			for (int i = 0; i < m; i++) {
				auto row = std::make_shared<FullRow<T>>(n);
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<std::shared_ptr<FullRow<T>>> listOfRows) {
			for (auto& row : listOfRows)
			{
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<std::initializer_list<T>> list2D) {
			for (auto& rowList : list2D)
			{
				auto row = std::make_shared<FullRow<T>>(rowList);
				this->push_back(row);
			}
		}
		void identity();
		std::shared_ptr<FullColumn<T>> column(int j);
		std::shared_ptr<FullColumn<T>> timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		std::shared_ptr<FullColumn<T>> timesFullColumn(FullColumn<T>* fullCol);
		std::shared_ptr<FullMatrix<T>> timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> times(double a);
		std::shared_ptr<FullMatrix<T>> transposeTimesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> plusFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> minusFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> transpose();
		std::shared_ptr<FullMatrix<T>> negated();
		void symLowerWithUpper();
		void atiput(int i, std::shared_ptr<FullRow<T>> fullRow);
		void atijput(int i, int j, T value);
		void atijputFullColumn(int i, int j, std::shared_ptr<FullColumn<T>> fullCol);
		void atijplusFullRow(int i, int j, std::shared_ptr<FullRow<T>> fullRow);
		void atijplusNumber(int i, int j, double value);
		void atijminusNumber(int i, int j, double value);
		double sumOfSquares() override;
		void zeroSelf() override;
		std::shared_ptr<FullMatrix<T>> copy();
		FullMatrix<T> operator+(const FullMatrix<T> fullMat);
		std::shared_ptr<FullColumn<T>> transposeTimesFullColumn(const std::shared_ptr<FullColumn<T>> fullCol);
		void magnifySelf(T factor);
		std::shared_ptr<EulerParameters<T>> asEulerParameters();
		T trace();
		double maxMagnitude() override;
		std::shared_ptr<FullColumn<T>> bryantAngles();

		std::ostream& printOn(std::ostream& s) const override;
	};
	template<>
	inline void FullMatrix<double>::identity() {
		this->zeroSelf();
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->at(i) = 1.0;
		}
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::column(int j) {
		int n = (int)this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->at(j);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		int m = this->nrow();
		auto answer = std::make_shared<FullMatrix<T>>(m);
		for (int i = 0; i < m; i++) {
			answer->at(i) = this->at(i)->timesFullMatrix(fullMat);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		int nrow = this->nrow();
		auto answer = std::make_shared<FullMatrix<T>>(nrow);
		for (int i = 0; i < nrow; i++) {
			answer->at(i) = this->at(i)->timesTransposeFullMatrix(fullMat);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::times(double a)
	{
		int m = this->nrow();
		auto answer = std::make_shared<FullMatrix<T>>(m);
		for (int i = 0; i < m; i++) {
			answer->at(i) = this->at(i)->times(a);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::transposeTimesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		return this->transpose()->timesFullMatrix(fullMat);
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::plusFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		int n = (int)this->size();
		auto answer = std::make_shared<FullMatrix<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->plusFullRow(fullMat->at(i));
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::minusFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		int n = (int)this->size();
		auto answer = std::make_shared<FullMatrix<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->minusFullRow(fullMat->at(i));
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::transpose()
	{
		int nrow = this->nrow();
		auto ncol = this->ncol();
		auto answer = std::make_shared<FullMatrix<T>>(ncol, nrow);
		for (int i = 0; i < nrow; i++) {
			auto& row = this->at(i);
			for (int j = 0; j < ncol; j++) {
				answer->at(j)->at(i) = row->at(j);
			}
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::negated()
	{
		return this->times(-1.0);
	}
	template<typename T>
	inline void FullMatrix<T>::symLowerWithUpper()
	{
		int n = (int)this->size();
		for (int i = 0; i < n; i++) {
			for (int j = i + 1; j < n; j++) {
				this->at(j)->at(i) = this->at(i)->at(j);
			}
		}
	}
	template<typename T>
	inline void FullMatrix<T>::atiput(int i, std::shared_ptr<FullRow<T>> fullRow)
	{
		this->at(i) = fullRow;
	}
	template<typename T>
	inline void FullMatrix<T>::atijput(int i, int j, T value)
	{
		this->at(i)->atiput(j, value);
	}
	template<typename T>
	inline void FullMatrix<T>::atijputFullColumn(int i1, int j1, std::shared_ptr<FullColumn<T>> fullCol)
	{
		for (int ii = 0; ii < fullCol->size(); ii++)
		{
			this->at(i1 + ii)->at(j1) = fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullMatrix<T>::atijplusFullRow(int i, int j, std::shared_ptr<FullRow<T>> fullRow)
	{
		this->at(i)->atiplusFullRow(j, fullRow);
	}
	template<typename T>
	inline void FullMatrix<T>::atijplusNumber(int i, int j, double value)
	{
		auto rowi = this->at(i);
		rowi->at(j) += value;
	}
	template<typename T>
	inline void FullMatrix<T>::atijminusNumber(int i, int j, double value)
	{
		auto rowi = this->at(i);
		rowi->at(j) -= value;
	}
	template<>
	inline double FullMatrix<double>::sumOfSquares()
	{
		double sum = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			sum += this->at(i)->sumOfSquares();
		}
		return sum;
	}
	template<typename T>
	inline double FullMatrix<T>::sumOfSquares()
	{
		assert(false);
		return 0.0;
	}
	template<>
	inline void FullMatrix<double>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
	template<typename T>
	inline void FullMatrix<T>::zeroSelf()
	{
		assert(false);
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::copy()
	{
		auto m = (int)this->size();
		auto answer = std::make_shared<FullMatrix<T>>(m);
		for (int i = 0; i < m; i++)
		{
			answer->at(i) = this->at(i)->copy();
		}
		return answer;
	}
	template<typename T>
	inline FullMatrix<T> FullMatrix<T>::operator+(const FullMatrix<T> fullMat)
	{
		int n = (int)this->size();
		auto answer = FullMatrix<T>(n);
		for (int i = 0; i < n; i++) {
			answer.at(i) = this->at(i)->plusFullRow(fullMat.at(i));
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::transposeTimesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		auto sptr = std::make_shared<FullMatrix<T>>(*this);
		return fullCol->transpose()->timesFullMatrix(sptr)->transpose();
	}
	template<typename T>
	inline void FullMatrix<T>::magnifySelf(T factor)
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->magnifySelf(factor);
		}
	}
	template<typename T>
	inline std::ostream& FullMatrix<T>::printOn(std::ostream& s) const
	{
		s << "FullMat[" << std::endl;
		for (int i = 0; i < this->size(); i++)
		{
			s << *(this->at(i)) << std::endl;
		}
		s << "]";
		return s;
	}
	template<typename T>
	inline std::shared_ptr<EulerParameters<T>> FullMatrix<T>::asEulerParameters()
	{
		//"Given [A], compute Euler parameter."

		auto traceA = this->trace();
		T dum = 0.0;
		T dumSq = 0.0;
		//auto qE = CREATE<EulerParameters<double>>::With(4); //Cannot use CREATE.h in subclasses of std::vector. Why?
		auto qE = std::make_shared<EulerParameters<T>>(4);
		qE->initialize();
		auto OneMinusTraceDivFour = (1.0 - traceA) / 4.0;
		for (int i = 0; i < 3; i++)
		{
			dumSq = this->at(i)->at(i) / 2.0 + OneMinusTraceDivFour;
			dum = (dumSq > 0.0) ? std::sqrt(dumSq) : 0.0;
			qE->atiput(i, dum);
		}
		dumSq = (1.0 + traceA) / 4.0;
		dum = (dumSq > 0.0) ? std::sqrt(dumSq) : 0.0;
		qE->atiput(3, dum);
		T max = 0.0;
		int maxE = -1;
		for (int i = 0; i < 4; i++)
		{
			auto num = qE->at(i);
			if (max < num) {
				max = num;
				maxE = i;
			}
		}

		if (maxE == 0) {
			auto FourE = 4.0 * qE->at(0);
			qE->atiput(1, (this->at(0)->at(1) + this->at(1)->at(0)) / FourE);
			qE->atiput(2, (this->at(0)->at(2) + this->at(2)->at(0)) / FourE);
			qE->atiput(3, (this->at(2)->at(1) - this->at(1)->at(2)) / FourE);
		}
		else if (maxE == 1) {
			auto FourE = 4.0 * qE->at(1);
			qE->atiput(0, (this->at(0)->at(1) + this->at(1)->at(0)) / FourE);
			qE->atiput(2, (this->at(1)->at(2) + this->at(2)->at(1)) / FourE);
			qE->atiput(3, (this->at(0)->at(2) - this->at(2)->at(0)) / FourE);
		}
		else if (maxE == 2) {
			auto FourE = 4.0 * qE->at(2);
			qE->atiput(0, (this->at(0)->at(2) + this->at(2)->at(0)) / FourE);
			qE->atiput(1, (this->at(1)->at(2) + this->at(2)->at(1)) / FourE);
			qE->atiput(3, (this->at(1)->at(0) - this->at(0)->at(1)) / FourE);
		}
		else if (maxE == 3) {
			auto FourE = 4.0 * qE->at(3);
			qE->atiput(0, (this->at(2)->at(1) - this->at(1)->at(2)) / FourE);
			qE->atiput(1, (this->at(0)->at(2) - this->at(2)->at(0)) / FourE);
			qE->atiput(2, (this->at(1)->at(0) - this->at(0)->at(1)) / FourE);
		}
		qE->conditionSelf();
		qE->calc();
		return qE;
	}
	template<typename T>
	inline T FullMatrix<T>::trace()
	{
		T trace = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			trace += this->at(i)->at(i);
		}
		return trace;
	}
	template<typename T>
	inline double FullMatrix<T>::maxMagnitude()
	{
		auto max = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			auto element = this->at(i)->maxMagnitude();
			if (max < element) max = element;
		}
		return max;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::bryantAngles()
	{
		auto answer = std::make_shared<FullColumn<T>>(3);
		auto sthe1y = this->at(0)->at(2);
		T the0x, the1y, the2z, cthe0x, sthe0x, y, x;
		if (std::abs(sthe1y) > 0.9999) {
			if (sthe1y > 0.0) {
				the0x = std::atan2(this->at(1)->at(0), this->at(1)->at(1));
				the1y = M_PI / 2.0;
				the2z = 0.0;
			}
			else {
				the0x = std::atan2(this->at(2)->at(1), this->at(2)->at(0));
				the1y = M_PI / -2.0;
				the2z = 0.0;
			}
		}
		else {
			the0x = std::atan2(-this->at(1)->at(2), this->at(2)->at(2));
			cthe0x = std::cos(the0x);
			sthe0x = std::sin(the0x);
			y = sthe1y;
			if (std::abs(cthe0x) > std::abs(sthe0x)) {
				x = this->at(2)->at(2) / cthe0x;
			}
			else {
				x = this->at(1)->at(2) / -sthe0x;
			}
			the1y = std::atan2(y, x);
			the2z = std::atan2(-this->at(0)->at(1), this->at(0)->at(0));
		}
		answer->atiput(0, the0x);
		answer->atiput(1, the1y);
		answer->atiput(2, the2z);
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		return this->timesFullColumn(fullCol.get());
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::timesFullColumn(FullColumn<T>* fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."
		auto nrow = this->nrow();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i)->timesFullColumn(fullCol);
		}
		return answer;
	}
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatFColDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>>;
	using FMatFMatDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>>;
	using FColFMatDsptr = std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>>;
}

