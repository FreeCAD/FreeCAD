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
		virtual double maxMagnitude() = 0;
		double maxMagnitudeOfVector();
		void equalArrayAt(std::shared_ptr<Array<T>> array, int i);
		//virtual void normalizeSelf();
		//virtual void conditionSelf();
		//virtual void conditionSelfWithTol(double tol);
		virtual void atiput(int i, T value);
		//double length();
		void magnifySelf(T factor);
		void atitimes(int i, double factor);

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
	//template<>
	//inline double Array<double>::maxMagnitude()
	//{
	//	auto max = 0.0;
	//	for (int i = 0; i < this->size(); i++)
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
	//template<>
	//inline void Array<double>::normalizeSelf()
	//{
	//	auto length = this->length();
	//	if (length == 0.0) throw std::runtime_error("Cannot normalize a null vector.");
	//	this->magnifySelf(1.0 / length);
	//}
	//template<>
	//inline void Array<double>::conditionSelf()
	//{
	//	constexpr auto epsilon = std::numeric_limits<double>::epsilon();
	//	auto tol = maxMagnitude() * epsilon;
	//	conditionSelfWithTol(tol);
	//}
	//template<>
	//inline void Array<double>::conditionSelfWithTol(double tol)
	//{
	//	for (int i = 0; i < this->size(); i++)
	//	{
	//		auto element = this->at(i);
	//		if (element < 0.0) element = -element;
	//		if (element < tol) this->atiput(i, 0.0);
	//	}
	//}
	template<typename T>
	inline void Array<T>::atiput(int i, T value)
	{
		this->at(i) = value;
	}
	//template<>
	//inline double Array<double>::length()
	//{
	//	auto ssq = 0.0;
	//	for (int i = 0; i < this->size(); i++)
	//	{
	//		auto elem = this->at(i);
	//		ssq += elem * elem;
	//	}
	//	return std::sqrt(ssq);
	//}
	template<typename T>
	inline void Array<T>::magnifySelf(T factor)
	{
		for (int i = 0; i < this->size(); i++)
		{
			this->atitimes(i, factor);
		}
	}
	template<typename T>
	inline void Array<T>::atitimes(int i, double factor)
	{
		this->at(i) *= factor;
	}
}