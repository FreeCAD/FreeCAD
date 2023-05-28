#pragma once
#include "Vector.h"
#include "FullColumn.h"

namespace MbD {
	template <typename T>
	class FullMatrix;

	template <typename T>
	class FullRow : public Vector<T>
	{
	public:
		FullRow() {}
		FullRow(size_t count) : Vector<T>(count) {}
		FullRow(size_t count, const T& value) : Vector<T>(count, value) {}
		FullRow(std::initializer_list<T> list) : Vector<T>{ list } {}
		std::shared_ptr<FullRow<T>> times(double a);
		std::shared_ptr<FullRow<T>> negated();
		std::shared_ptr<FullRow<T>> plusFullRow(std::shared_ptr<FullRow<T>> fullRow);
		std::shared_ptr<FullRow<T>> minusFullRow(std::shared_ptr<FullRow<T>> fullRow);
		T timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		std::shared_ptr<FullRow<T>> timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		std::shared_ptr<FullRow<T>> timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat);
		void equalSelfPlusFullRowTimes(std::shared_ptr<FullRow<T>> fullRow, double factor);
	};

	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::times(double a)
	{
		size_t n = this->size();
		auto answer = std::make_shared<FullRow>(n);
		for (size_t i = 0; i < n; i++) {
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
		size_t n = this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) + fullRow->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::minusFullRow(std::shared_ptr<FullRow<T>> fullRow)
	{
		size_t n = this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) - fullRow->at(i);
		}
		return answer;
	}
	template<typename T>
	inline T FullRow<T>::timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		auto answer = this->at(0) * fullCol->at(0);
		for (size_t i = 1; i < this->size(); i++)
		{
			answer += this->at(i) * fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::timesTransposeFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		return std::shared_ptr<FullRow<T>>();
	}
	template<typename T>
	inline void FullRow<T>::equalSelfPlusFullRowTimes(std::shared_ptr<FullRow<T>> fullRow, double factor)
	{
		for (size_t i = 0; i < this->size(); i++)
		{
			this->at(i) += fullRow->at(i) * factor;
		}
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::timesFullMatrix(std::shared_ptr<FullMatrix<T>> fullMat)
	{
		std::shared_ptr<FullRow<T>> answer = fullMat->at(0)->times(this->at(0));
		for (size_t j = 1; j < this->size(); j++)
		{
			answer->equalSelfPlusFullRowTimes(fullMat->at(j), this->at(j));
		}
		return answer;
			//return std::shared_ptr<FullRow<T>>();
	}
	using ListFRD = std::initializer_list<std::shared_ptr<FullRow<double>>>;
	using FRowDsptr = std::shared_ptr<FullRow<double>>;
}

