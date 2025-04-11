/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <ostream>
#include <vector>
#include <memory>
#include <cmath>
#include <cassert>
#include "Numeric.h"
#include <limits>

//#include "Symbolic.h"

namespace MbD {
	using ListD = std::initializer_list<double>;
	using ListListD = std::initializer_list<std::initializer_list<double>>;
	using ListListPairD = std::initializer_list<std::initializer_list<std::initializer_list<double>>>;

	template<typename T>
	class Array : public std::vector<T>
	{
	public:
		Array() {}
		Array(std::vector<T> vec) : std::vector<T>(vec) {}
		Array(size_t count) : std::vector<T>(count) {}
		Array(size_t count, const T& value) : std::vector<T>(count, value) {}
		Array(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end) : std::vector<T>(begin, end) {}
		Array(std::initializer_list<T> list) : std::vector<T>{ list } {}
		virtual ~Array() {}
		virtual void initialize();
		static bool equaltol(double x, double xx, double tol);
		void copyFrom(std::shared_ptr<Array<T>> x);
		virtual void zeroSelf();
		virtual double sumOfSquares() = 0;
		double rootMeanSquare();
		virtual size_t numberOfElements();
		void swapElems(size_t i, size_t ii);
		virtual double maxMagnitude() = 0;
		double maxMagnitudeOfVector();
		void equalArrayAt(std::shared_ptr<Array<T>> array, size_t i);
		void atiput(size_t i, T value);
		void magnifySelf(T factor);
		void negateSelf();
		void atitimes(size_t i, double factor);

		virtual std::ostream& printOn(std::ostream& s) const {
			std::string str = typeid(*this).name();
			auto classname = str.substr(11, str.size() - 11);
			s << classname << std::endl;
			return s;
		}
		friend std::ostream& operator<<(std::ostream& s, const Array& array)
		{
			return array.printOn(s);
		}

	};
	template<typename T>
	inline void Array<T>::initialize()
	{
	}
	template<>
	inline bool Array<double>::equaltol(double x, double xx, double tol)
	{
		return std::abs(x - xx) < tol;
	}
	template<typename T>
	inline void Array<T>::copyFrom(std::shared_ptr<Array<T>> x)
	{
		for (size_t i = 0; i < x->size(); i++) {
			this->at(i) = x->at(i);
		}
	}
	template<typename T>
	inline void Array<T>::zeroSelf()
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i) = (T)0;
		}
	}
	template<typename T>
	inline double Array<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	template<typename T>
	inline size_t Array<T>::numberOfElements()
	{
		return this->size();
	}
	template<typename T>
	inline void Array<T>::swapElems(size_t i, size_t ii)
	{
		auto temp = this->at(i);
		this->at(i) = this->at(ii);
		this->at(ii) = temp;
	}
	//template<>
	//inline double Array<double>::maxMagnitude()
	//{
	//	double max = 0.0;
	//	for (size_t i = 0; i < this->size(); i++)
	//	{
	//		auto element = this->at(i);
	//		if (element < 0.0) element = -element;
	//		if (max < element) max = element;
	//	}
	//	return max;
	//}
	template<typename T>
	inline double Array<T>::maxMagnitudeOfVector()
	{
		double answer = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double mag = std::abs(this->at(i));
			if (answer < mag) answer = mag;
		}
		return answer;
	}
	template<typename T>
	inline void Array<T>::equalArrayAt(std::shared_ptr<Array<T>> array, size_t i)
	{
		for (size_t ii = 0; ii < this->size(); ii++)
		{
			this->at(ii) = array->at(i + ii);
		}
	}
	//template<>
	//inline void Array<double>::normalizeSelf()
	//{
	//	double length = this->length();
	//	if (length == 0.0) throw std::runtime_error("Cannot normalize a null vector.");
	//	this->magnifySelf(1.0 / length);
	//}
	//template<>
	//inline void Array<double>::conditionSelf()
	//{
	//	constexpr double epsilon = std::numeric_limits<double>::epsilon();
	//	double tol = maxMagnitude() * epsilon;
	//	conditionSelfWithTol(tol);
	//}
	//template<>
	//inline void Array<double>::conditionSelfWithTol(double tol)
	//{
	//	for (size_t i = 0; i < this->size(); i++)
	//	{
	//		double element = this->at(i);
	//		if (element < 0.0) element = -element;
	//		if (element < tol) this->atiput(i, 0.0);
	//	}
	//}
	template<typename T>
	inline void Array<T>::atiput(size_t i, T value)
	{
		this->at(i) = value;
	}
	//template<>
	//inline double Array<double>::length()
	//{
	//	double ssq = 0.0;
	//	for (size_t i = 0; i < this->size(); i++)
	//	{
	//		double elem = this->at(i);
	//		ssq += elem * elem;
	//	}
	//	return std::sqrt(ssq);
	//}
	template<typename T>
	inline void Array<T>::magnifySelf(T factor)
	{
		for (size_t i = 0; i < this->size(); i++)
		{
			this->atitimes(i, factor);
		}
	}
	template<typename T>
	inline void Array<T>::negateSelf()
	{
		magnifySelf(-1);
	}
	template<typename T>
	inline void Array<T>::atitimes(size_t i, double factor)
	{
		this->at(i) *= factor;
	}
}