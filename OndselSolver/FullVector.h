/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include <ostream>

#include "Array.h"

namespace MbD {
	template<typename T>
	class FullVector : public Array<T>
	{
	public:
		FullVector() : Array<T>() {}
		FullVector(std::vector<T> vec) : Array<T>(vec) {}
		FullVector(size_t count) : Array<T>(count) {}
		FullVector(size_t count, const T& value) : Array<T>(count, value) {}
		FullVector(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end) : Array<T>(begin, end) {}
		FullVector(std::initializer_list<T> list) : Array<T>{ list } {}
		double dot(std::shared_ptr<FullVector<T>> vec);
		void atiplusNumber(size_t i, T value);
		void atiminusNumber(size_t i, T value);
		double sumOfSquares() override;
		size_t numberOfElements() override;
		void zeroSelf() override;
		void atiplusFullVector(size_t i, std::shared_ptr<FullVector<T>> fullVec);
		void atiplusFullVectortimes(size_t i, std::shared_ptr<FullVector<T>> fullVec, T factor);
		void equalSelfPlusFullVectortimes(std::shared_ptr<FullVector<T>> fullVec, T factor);
		double maxMagnitude() override;
		void normalizeSelf();
		double length();
		virtual void conditionSelf();
		virtual void conditionSelfWithTol(double tol);
		std::shared_ptr<FullVector<T>> clonesptr();
		bool isIncreasing();
		bool isIncreasingIfExceptionsAreLessThan(double tol);
		bool isDecreasingIfExceptionsAreLessThan(double tol);
		

		std::ostream& printOn(std::ostream& s) const override;

	};
	template<typename T>
	inline double FullVector<T>::dot(std::shared_ptr<FullVector<T>> vec)
	{
		auto n = this->size();
		double answer = 0.0;
		for (size_t i = 0; i < n; i++) {
			answer += this->at(i) * vec->at(i);
		}
		return answer;
	}
	template<typename T>
	inline void FullVector<T>::atiplusNumber(size_t i, T value)
	{
		this->at(i) += value;
	}
	template<typename T>
	inline void FullVector<T>::atiminusNumber(size_t i, T value)
	{
		this->at(i) -= value;
	}
	template<>
	inline double FullVector<double>::sumOfSquares()
	{
		double sum = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			sum += element * element;
		}
		return sum;
	}
	template<typename T>
	inline double FullVector<T>::sumOfSquares()
	{
		assert(false);
		return 0.0;
	}
	template<typename T>
	inline size_t FullVector<T>::numberOfElements()
	{
		return this->size();
	}
	template<>
	inline void FullVector<double>::zeroSelf()
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;
		}
	}
	template<typename T>
	inline void FullVector<T>::zeroSelf()
	{
		assert(false);
	}
	template<typename T>
	inline void FullVector<T>::atiplusFullVector(size_t i1, std::shared_ptr<FullVector<T>> fullVec)
	{
		for (size_t ii = 0; ii < fullVec->size(); ii++)
		{
			auto i = i1 + ii;
			this->at(i) += fullVec->at(ii);
		}
	}
	template<typename T>
	inline void FullVector<T>::atiplusFullVectortimes(size_t i1, std::shared_ptr<FullVector<T>> fullVec, T factor)
	{
		for (size_t ii = 0; ii < fullVec->size(); ii++)
		{
			auto i = i1 + ii;
			this->at(i) += fullVec->at(ii) * factor;
		}
	}
	template<typename T>
	inline void FullVector<T>::equalSelfPlusFullVectortimes(std::shared_ptr<FullVector<T>> fullVec, T factor)
	{
		for (size_t i = 0; i < this->size(); i++)
		{
			this->atiplusNumber(i, fullVec->at(i) * factor);
		}
	}
	template<>
	inline double FullVector<double>::maxMagnitude()
	{
		double max = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			if (element < 0.0) element = -element;
			if (max < element) max = element;
		}
		return max;
	}
	template<typename T>
	inline double FullVector<T>::maxMagnitude()
	{
		assert(false);
		return 0.0;
	}
	template<>
	inline void FullVector<double>::normalizeSelf()
	{
		double length = this->length();
		if (length == 0.0) throw std::runtime_error("Cannot normalize a null vector.");
		this->magnifySelf(1.0 / length);
	}
	template<typename T>
	inline double FullVector<T>::length()
	{
		double ssq = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double elem = this->at(i);
			ssq += elem * elem;
		}
		return std::sqrt(ssq);
	}
	template<typename T>
	inline void FullVector<T>::conditionSelf()
	{
		constexpr double epsilon = std::numeric_limits<double>::epsilon();
		double tol = this->maxMagnitude() * epsilon;
		this->conditionSelfWithTol(tol);
	}
	template<>
	inline void FullVector<double>::conditionSelfWithTol(double tol)
	{
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			if (element < 0.0) element = -element;
			if (element < tol) this->atiput(i, 0.0);
		}
	}
	template<typename T>
	inline void FullVector<T>::conditionSelfWithTol(double tol)
	{
		assert(false && tol != tol);   // clang++ flips out with warnings if you don't use 'tol'
                                       // but suppressing that warning breaks Visual Studio.
		return;                        // Visual Studio demands the unused return
	}
	template<typename T>
	inline std::shared_ptr<FullVector<T>> FullVector<T>::clonesptr()
	{
		//Return shallow copy of *this wrapped in shared_ptr
		assert(false);
		return std::make_shared<FullVector<T>>(*this);
	}
	template<typename T>
	inline bool FullVector<T>::isIncreasing()
	{
		return isIncreasingIfExceptionsAreLessThan(0.0);
	}
	template<typename T>
	inline bool FullVector<T>::isIncreasingIfExceptionsAreLessThan(double tol)
	{
		//"Test if elements are increasing."
		//"Ok if spoilers are less than tol."
		auto next = this->at(0);
		for (size_t i = 1; i < this->size(); i++)
		{
			auto previous = next;
			next = this->at(i);
			if (previous > next && (previous - tol > next)) return false;
		}
		return true;
	}
	template<typename T>
	inline bool FullVector<T>::isDecreasingIfExceptionsAreLessThan(double tol)
	{
		//"Test if elements are increasing."
		//"Ok if spoilers are less than tol."
		auto next = this->at(0);
		for (size_t i = 1; i < this->size(); i++)
		{
			auto previous = next;
			next = this->at(i);
			if (previous < next && (previous + tol < next)) return false;
		}
		return true;
	}
	template<typename T>
	inline std::ostream& FullVector<T>::printOn(std::ostream& s) const
	{
		s << "FullVec{";
		s << this->at(0);
		for (size_t i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "}";
		return s;
	}
}
