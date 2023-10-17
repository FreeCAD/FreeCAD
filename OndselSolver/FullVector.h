/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include <ostream>
#include <limits>

#include "Array.h"

namespace MbD {
	template<typename T>
	class FullVector : public Array<T>
	{
	public:
		FullVector() : Array<T>() {}
		FullVector(std::vector<T> vec) : Array<T>(vec) {}
		FullVector(int count) : Array<T>(count) {}
		FullVector(int count, const T& value) : Array<T>(count, value) {}
		FullVector(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end) : Array<T>(begin, end) {}
		FullVector(std::initializer_list<T> list) : Array<T>{ list } {}
		double dot(std::shared_ptr<FullVector<T>> vec);
		void atiplusNumber(int i, T value);
		void atiminusNumber(int i, T value);
		double sumOfSquares() override;
		int numberOfElements() override;
		void zeroSelf() override;
		void atiplusFullVector(int i, std::shared_ptr<FullVector<T>> fullVec);
		void atiplusFullVectortimes(int i, std::shared_ptr<FullVector<T>> fullVec, T factor);
		void equalSelfPlusFullVectortimes(std::shared_ptr<FullVector<T>> fullVec, T factor);
		double maxMagnitude() override;
		void normalizeSelf();
		double length();
		virtual void conditionSelf();
		void conditionSelfWithTol(double tol);

		std::ostream& printOn(std::ostream& s) const override;

	};
	template<typename T>
	inline double FullVector<T>::dot(std::shared_ptr<FullVector<T>> vec)
	{
		int n = (int)this->size();
		double answer = 0.0;
		for (int i = 0; i < n; i++) {
			answer += this->at(i) * vec->at(i);
		}
		return answer;
	}
	template<typename T>
	inline void FullVector<T>::atiplusNumber(int i, T value)
	{
		this->at(i) += value;
	}
	template<typename T>
	inline void FullVector<T>::atiminusNumber(int i, T value)
	{
		this->at(i) -= value;
	}
	template<>
	inline double FullVector<double>::sumOfSquares()
	{
		double sum = 0.0;
		for (int i = 0; i < this->size(); i++)
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
	inline int FullVector<T>::numberOfElements()
	{
		return (int)this->size();
	}
	template<>
	inline void FullVector<double>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;
		}
	}
	template<typename T>
	inline void FullVector<T>::zeroSelf()
	{
		assert(false);
	}
	template<typename T>
	inline void FullVector<T>::atiplusFullVector(int i1, std::shared_ptr<FullVector<T>> fullVec)
	{
		for (int ii = 0; ii < fullVec->size(); ii++)
		{
			auto i = i1 + ii;
			this->at(i) += fullVec->at(ii);
		}
	}
	template<typename T>
	inline void FullVector<T>::atiplusFullVectortimes(int i1, std::shared_ptr<FullVector<T>> fullVec, T factor)
	{
		for (int ii = 0; ii < fullVec->size(); ii++)
		{
			auto i = i1 + ii;
			this->at(i) += fullVec->at(ii) * factor;
		}
	}
	template<typename T>
	inline void FullVector<T>::equalSelfPlusFullVectortimes(std::shared_ptr<FullVector<T>> fullVec, T factor)
	{
		for (int i = 0; i < this->size(); i++)
		{
			this->atiplusNumber(i, fullVec->at(i) * factor);
		}
	}
	template<>
	inline double FullVector<double>::maxMagnitude()
	{
		double max = 0.0;
		for (int i = 0; i < this->size(); i++)
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
		for (int i = 0; i < this->size(); i++)
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
		for (int i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			if (element < 0.0) element = -element;
			if (element < tol) this->atiput(i, 0.0);
		}
	}
	template<typename T>
	inline void FullVector<T>::conditionSelfWithTol(double tol)
	{
		assert(false);
		return;
	}
	template<typename T>
	inline std::ostream& FullVector<T>::printOn(std::ostream& s) const
	{
		s << "FullVec{";
		s << this->at(0);
		for (int i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "}";
		return s;
	}
}
