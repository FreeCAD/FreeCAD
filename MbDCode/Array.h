#pragma once
#include <vector>
#include <memory>
//#include <type_traits>
#include <cmath>
#include <cassert>


namespace MbD {
	template<typename T>
	class Array : public std::vector<T>
	{
	public:
		Array() {}
		Array(std::vector<T> vec) : std::vector<T>(vec) {}
		Array(int count) : std::vector<T>(count) {}
		Array(int count, const T& value) : std::vector<T>(count, value) {}
		Array(std::vector<T>::iterator begin, std::vector<T>::iterator end) : std::vector<T>(begin, end) {}
		Array(std::initializer_list<T> list) : std::vector<T>{ list } {}
		virtual void initialize();
		void copyFrom(std::shared_ptr<Array<T>> x);
		virtual void zeroSelf();
		virtual double sumOfSquares() = 0;
		double rootMeanSquare();
		virtual int numberOfElements();
		void swapElems(int i, int ii);
		//double maxMagnitude();
		double maxMagnitudeOfVector();
		void equalArrayAt(std::shared_ptr<Array<T>> array, int i);

	};
	template<typename T>
	inline void Array<T>::initialize()
	{
	}
	template<typename T>
	inline void Array<T>::copyFrom(std::shared_ptr<Array<T>> x)
	{
		for (int i = 0; i < x->size(); i++) {
			this->at(i) = x->at(i);
		}
	}
	template<typename T>
	inline void Array<T>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i) = (T)0;
		}
	}
	template<typename T>
	inline double Array<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	template<typename T>
	inline int Array<T>::numberOfElements()
	{
		return (int)this->size();
	}
	template<typename T>
	inline void Array<T>::swapElems(int i, int ii)
	{
		auto temp = this->at(i);
		this->at(i) = this->at(ii);
		this->at(ii) = temp;
	}
	//template<typename T>
	//inline double Array<T>::maxMagnitude()
	//{
	//	if (std::is_arithmetic<T>::value) {
	//		return this->maxMagnitudeOfVector();
	//	}
	//	else {
	//		auto answer = 0.0;
	//		for (int i = 0; i < this->size(); i++)
	//		{
	//			auto mag = this->at(i)->maxMagnitude();
	//			if (answer < mag) answer = mag;
	//		}
	//		return answer;
	//	}
	//}
	template<typename T>
	inline double Array<T>::maxMagnitudeOfVector()
	{
		auto answer = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			auto mag = std::abs(this->at(i));
			if (answer < mag) answer = mag;
		}
		return answer;
	}
	template<typename T>
	inline void Array<T>::equalArrayAt(std::shared_ptr<Array<T>> array, int i)
	{
		for (int ii = 0; ii < this->size(); ii++)
		{
			this->at(ii) = array->at(i + ii);
		}
	}
		using ListD = std::initializer_list<double>;
		using ListListD = std::initializer_list<std::initializer_list<double>>;
		using ListListPairD = std::initializer_list<std::initializer_list<std::initializer_list<double>>>;
}