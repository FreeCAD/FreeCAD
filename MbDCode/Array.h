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
		Array(std::vector<T> vec) : std::vector<T>(vec) {}
		Array(int count) : std::vector<T>(count) {}
		Array(int count, const T& value) : std::vector<T>(count, value) {}
		Array(std::vector<T>::iterator begin, std::vector<T>::iterator end) : std::vector<T>(begin, end) {}
		Array(std::initializer_list<T> list) : std::vector<T>{ list } {}
		void copyFrom(std::shared_ptr<Array<T>> x);
		virtual void zeroSelf() = 0;
		virtual double sumOfSquares() = 0;
		double rootMeanSquare();
		virtual int numberOfElements() = 0;
		void swapRows(int i, int ii);
	};
	template<typename T>
	inline void Array<T>::copyFrom(std::shared_ptr<Array<T>> x)
	{
		for (int i = 0; i < x->size(); i++) {
			this->at(i) = x->at(i);
		}
	}
	template<typename T>
	inline double Array<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	template<typename T>
	inline void Array<T>::swapRows(int i, int ii)
	{
		auto temp = this->at(i);
		this->at(i) = this->at(ii);
		this->at(ii) = temp;
	}
	using ListD = std::initializer_list<double>;
	using ListListD = std::initializer_list<std::initializer_list<double>>;
	using ListListPairD = std::initializer_list<std::initializer_list<std::initializer_list<double>>>;
}

