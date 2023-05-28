#pragma once
#include <string>
#include <sstream> 

#include "Vector.h"

namespace MbD {

	template <typename T>
	class FullColumn : public Vector<T>
	{
	public:
		FullColumn(size_t count) : Vector<T>(count) {}
		FullColumn(size_t count, const T& value) : Vector<T>(count, value) {}
		FullColumn(std::initializer_list<T> list) : Vector<T>{ list } {}
		std::shared_ptr<FullColumn<T>> plusFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		std::shared_ptr<FullColumn<T>> minusFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		std::shared_ptr<FullColumn<T>> times(double a);
		std::shared_ptr<FullColumn<T>> negated();
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
}

