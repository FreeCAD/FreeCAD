#pragma once
#include <vector>
#include <memory>
#include <type_traits>

namespace MbD {
	template <typename T>
	class Array : public std::vector<T>
	{
	public:
		Array() {}
		Array(size_t count) : std::vector<T>(count) {}
		Array(size_t count, const T& value) : std::vector<T>(count, value) {}
		Array(std::initializer_list<T> list) : std::vector<T>{ list } {}
		void copyFrom(std::shared_ptr<Array<T>> x);
		void zeroSelf();
		double sumOfSquares();
		double sumOfSquaresOfVector();
	};
	template<typename T>
	inline void Array<T>::copyFrom(std::shared_ptr<Array<T>> x)
	{
		for (size_t i = 0; i < x->size(); i++) {
			this->at(i) = x->at(i);
		}
	}
	template <>
	inline void Array<double>::zeroSelf() {
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;;
		}
	}
	template<typename T>
	inline double Array<T>::sumOfSquares()
	{
		if (std::is_arithmetic<T>) {
			return this->sumOfSquaresOfVector();
		}
		else {
			double sum = 0.0;
			for (size_t i = 0; i < this->size(); i++)
			{
				sum += this->at(i)->sumOfSquares();
			}
			return sum;
		}
	}
	template<typename T>
	inline double Array<T>::sumOfSquaresOfVector()
	{
		double sum = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			sum += element * element;
		}
		return sum;
	}
	using ListD = std::initializer_list<double>;
	using ListListD = std::initializer_list<std::initializer_list<double>>;
	using ListListPairD = std::initializer_list<std::initializer_list<std::initializer_list<double>>>;
}

