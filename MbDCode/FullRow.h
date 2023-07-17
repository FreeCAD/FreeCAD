#pragma once

#include "FullVector.h"
//#include "FullColumn.h"

namespace MbD {
	template<typename T>
	class FullMatrix;
	template<typename T>
	class FullColumn;

	template<typename T>
	class FullRow : public FullVector<T>
	{
	public:
		FullRow() : FullVector<T>() {}
		FullRow(std::vector<T> vec) : FullVector<T>(vec) {}
		FullRow(int count) : FullVector<T>(count) {}
		FullRow(int count, const T& value) : FullVector<T>(count, value) {}
		FullRow(std::vector<T>::iterator begin, std::vector<T>::iterator end) : FullVector<T>(begin, end) {}
		FullRow(std::initializer_list<T> list) : FullVector<T>{ list } {}
		std::shared_ptr<FullRow<T>> times(double a);
		std::shared_ptr<FullRow<T>> negated();
		std::shared_ptr<FullRow<T>> plusFullRow(std::shared_ptr<FullRow<T>> fullRow);
		std::shared_ptr<FullRow<T>> minusFullRow(std::shared_ptr<FullRow<T>> fullRow);
		T timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		T timesFullColumn(FullColumn<T>* fullCol);
		std::shared_ptr<FullRow<T>> timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullRow<T>> timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		void equalSelfPlusFullRowTimes(std::shared_ptr<FullRow<T>> fullRow, double factor);
		void equalFullRow(std::shared_ptr<FullRow<T>> fullRow);
		std::shared_ptr<FullColumn<T>> transpose();
		std::shared_ptr<FullRow<T>> copy();
		void atiplusFullRow(int j, std::shared_ptr<FullRow<T>> fullRow);
		std::shared_ptr<FullMatrix<T>> transposeTimesFullRow(std::shared_ptr<FullRow<T>> fullRow);
		std::ostream& printOn(std::ostream& s) const override;

	};

	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::times(double a)
	{
		int n = (int) this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) * a;
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::negated()
	{
		return this->times(-1.0);
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::plusFullRow(std::shared_ptr<FullRow<T>> fullRow)
	{
		int n = (int) this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) + fullRow->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::minusFullRow(std::shared_ptr<FullRow<T>> fullRow)
	{
		int n = (int) this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) - fullRow->at(i);
		}
		return answer;
	}
	template<typename T>
	inline T FullRow<T>::timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		return this->timesFullColumn(fullCol.get());
	}
	template<typename T>
	inline T FullRow<T>::timesFullColumn(FullColumn<T>* fullCol)
	{
		auto answer = this->at(0) * fullCol->at(0);
		for (int i = 1; i < this->size(); i++)
		{
			answer += this->at(i) * fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		//"a*bT = a(1,j)b(k,j)"
		int ncol = fullMat->nrow();
		auto answer = std::make_shared<FullRow<T>>(ncol);
		for (int k = 0; k < ncol; k++) {
			answer->at(k) = this->dot(fullMat->at(k));
		}
		return answer;
	}
	template<typename T>
	inline void FullRow<T>::equalSelfPlusFullRowTimes(std::shared_ptr<FullRow<T>> fullRow, double factor)
	{
		this->equalSelfPlusFullVectortimes(fullRow, factor);
	}
	template<typename T>
	inline void FullRow<T>::equalFullRow(std::shared_ptr<FullRow<T>> fullRow)
	{
		this->equalArrayAt(fullRow, 0);
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullRow<T>::transpose()
	{
		return std::make_shared<FullColumn<T>>(*this);
	}
	template<>
	inline std::shared_ptr<FullRow<double>> FullRow<double>::copy()
	{
		auto n = (int)this->size();
		auto answer = std::make_shared<FullRow<double>>(n);
		for (int i = 0; i < n; i++)
		{
			answer->at(i) = this->at(i);
		}
		return answer;
	}
	template<typename T>
	inline void FullRow<T>::atiplusFullRow(int j1, std::shared_ptr<FullRow<T>> fullRow)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			auto j = j1 + jj;
			this->at(j) += fullRow->at(jj);
		}
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullRow<T>::transposeTimesFullRow(std::shared_ptr<FullRow<T>> fullRow)
	{
		//"a*b = a(i)b(j)"
		auto nrow = (int)this->size();
		auto answer = std::make_shared<FullMatrix<double>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->atiput(i, fullRow->times(this->at(i)));
		}
		return answer;
	}
	template<typename T>
	inline std::ostream& FullRow<T>::printOn(std::ostream& s) const
	{
		s << "FullRow{";
		s << this->at(0);
		for (int i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "}";
		return s;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		std::shared_ptr<FullRow<T>> answer = fullMat->at(0)->times(this->at(0));
		for (int j = 1; j < (int) this->size(); j++)
		{
			answer->equalSelfPlusFullRowTimes(fullMat->at(j), this->at(j));
		}
		return answer;
			//return std::shared_ptr<FullRow<T>>();
	}
	using ListFRD = std::initializer_list<std::shared_ptr<FullRow<double>>>;
	using FRowDsptr = std::shared_ptr<FullRow<double>>;
}

