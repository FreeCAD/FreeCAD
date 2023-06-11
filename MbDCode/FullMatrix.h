#pragma once
#include <memory>

#include "RowTypeMatrix.h"

namespace MbD {
	template <typename T>
	class FullColumn;
	template <typename T>
	class FullRow;

	template <typename T>
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
		std::shared_ptr<FullMatrix<T>> timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> times(double a);
		std::shared_ptr<FullMatrix<T>> plusFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullMatrix<T>> transpose();
		std::shared_ptr<FullMatrix<T>> negated();
		void symLowerWithUpper();
		void atijputFullColumn(int i, int j, std::shared_ptr<FullColumn<T>> fullCol);
		double sumOfSquares() override;
		void zeroSelf() override;
	};
	template <>
	inline void FullMatrix<double>::identity() {
		this->zeroSelf();
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->at(i) = 1.0;
		}
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::column(int j) {
		int n = (int) this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->at(j);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."
		int n = (int) this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (int i = 0; i < n; i++)
		{
			answer->at(i) = this->at(i)->timesFullColumn(fullCol);
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
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::plusFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		int n = (int) this->size();
		auto answer = std::make_shared<FullMatrix<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->plusFullRow(fullMat->at(i));
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
		int n = (int) this->size();
		for (int i = 0; i < n; i++) {
			for (int j = i + 1; j < n; j++) {
				this->at(j)->at(i) = this->at(i)->at(j);
			}
		}
	}
	template<typename T>
	inline void FullMatrix<T>::atijputFullColumn(int i1, int j1, std::shared_ptr<FullColumn<T>> fullCol)
	{
		auto iOffset = i1 - 1;
		for (int ii = 0; ii < fullCol->size(); ii++)
		{
			this->at(iOffset + ii)->at(j1) = fullCol->at(ii);
		}
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
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatFColDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>>;
	using FMatFMatDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>>;
	using FColFMatDsptr = std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>>;
}

