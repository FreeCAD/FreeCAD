#pragma once
#include <vector>
#include <memory>
//#include <type_traits>
#include <cmath>
#include <cassert>


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
		virtual void zeroSelf() = 0;
		virtual double sumOfSquares() = 0;
		double rootMeanSquare();
		virtual size_t numberOfElements() = 0;
	};
	template<typename T>
	inline void Array<T>::copyFrom(std::shared_ptr<Array<T>> x)
	{
		for (size_t i = 0; i < x->size(); i++) {
			this->at(i) = x->at(i);
		}
	}
	//template <>
	//inline void Array<double>::zeroSelf() {
	//	for (size_t i = 0; i < this->size(); i++) {
	//		this->at(i) = 0.0;;
	//	}
	//}
	template<typename T>
	inline double Array<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	using ListD = std::initializer_list<double>;
	using ListListD = std::initializer_list<std::initializer_list<double>>;
	using ListListPairD = std::initializer_list<std::initializer_list<std::initializer_list<double>>>;
}

