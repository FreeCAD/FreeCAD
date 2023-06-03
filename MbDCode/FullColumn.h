#pragma once
#include <string>
#include <sstream> 

#include "FullVector.h"

namespace MbD {

	template <typename T>
	class FullColumn : public FullVector<T>
	{
	public:
		FullColumn(size_t count) : FullVector<T>(count) {}
		FullColumn(size_t count, const T& value) : FullVector<T>(count, value) {}
		FullColumn(std::initializer_list<T> list) : FullVector<T>{ list } {}
		std::shared_ptr<FullColumn<T>> plusFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		std::shared_ptr<FullColumn<T>> minusFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		std::shared_ptr<FullColumn<T>> times(double a);
		std::shared_ptr<FullColumn<T>> negated();
		void atiputFullColumn(size_t i, std::shared_ptr<FullColumn<T>> fullCol);
		void equalSelfPlusFullColumnAt(std::shared_ptr<FullColumn<T>> fullCol, size_t i);
		void atiminusFullColumn(size_t i, std::shared_ptr<FullColumn<T>> fullCol);
		void equalFullColumnAt(std::shared_ptr<FullColumn<T>> fullCol, size_t i);
		std::shared_ptr<FullColumn<T>> copy();
		

		std::string toString()
		{
			std::stringstream ss;

			ss << "FullColumn { ";
			for (size_t i = 0; i < this->size() - 1; i++) {
				ss << this->at(i) << ", ";
			}
			ss << this->back() << " }";
			return ss.str();
		}
	};
	using FColDsptr = std::shared_ptr<FullColumn<double>>;
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullColumn<T>::plusFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		size_t n = this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) + fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullColumn<T>::minusFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		size_t n = this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) - fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullColumn<T>::times(double a)
	{
		size_t n = this->size();
		auto answer = std::make_shared<FullColumn>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) * a;
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullColumn<T>::negated()
	{
		return this->times(-1.0);
	}
	template<typename T>
	inline void FullColumn<T>::atiputFullColumn(size_t i, std::shared_ptr<FullColumn<T>> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			this->at(i + ii) = fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullColumn<T>::equalSelfPlusFullColumnAt(std::shared_ptr<FullColumn<T>> fullCol, size_t ii)
	{
		//self is subcolumn of fullCol
		for (size_t i = 0; i < this->size(); i++)
		{
			this->at(i) += fullCol->at(ii + i);
		}
	}
	template<typename T>
	inline void FullColumn<T>::atiminusFullColumn(size_t i1, std::shared_ptr<FullColumn<T>> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			size_t i = i1 + ii;
			this->at(i) -= fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullColumn<T>::equalFullColumnAt(std::shared_ptr<FullColumn<T>> fullCol, size_t ii)
	{
		for (size_t i = 0; i < this->size(); i++)
		{
			this->at(i) = fullCol->at(ii + i);
		}
	}
	template<>
	inline std::shared_ptr<FullColumn<double>> FullColumn<double>::copy()
	{
		auto n = this->size();
		auto answer = std::make_shared<FullColumn<double>>(n);
		for (size_t i = 0; i < n; i++)
		{
			answer->at(i) = this->at(i);
		}
		return answer;
	}
}

