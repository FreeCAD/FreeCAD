#pragma once

#include <memory>

#include "RowTypeMatrix.h"

namespace MbD {
	template<typename T>
	class FullColumn;
	template<typename T>
	class FullRow;

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

