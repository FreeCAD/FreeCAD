/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <map>
#include <cmath>
#include <sstream> 
#include <iomanip>

namespace MbD {
	template<typename T>
	class SparseVector : public std::map<size_t, T>
	{
	public:
		size_t n;
		SparseVector() {}
		SparseVector(size_t n) : std::map<size_t, T>(), n(n) {}
		SparseVector(std::initializer_list<std::pair<const size_t, T>> list) : std::map<size_t, T>{ list } {}
		SparseVector(std::initializer_list<std::initializer_list<T>> list) {
			for (auto& pair : list) {
				size_t i = 0;
				size_t index;
				T value;
				for (auto& element : pair) {
					if (i == 0) index = std::round(element);
					if (i == 1) value = element;
					i++;
				}
				this->insert(std::pair<const size_t, double>(index, value));
			}
		}
		virtual ~SparseVector() {}
		double rootMeanSquare();
		size_t numberOfElements();
		double sumOfSquares();
		void atiput(size_t i, T value);
		void atiplusNumber(size_t i, double value);
		void atiminusNumber(size_t i, double value);
		void zeroSelf();
		double maxMagnitude();
		void magnifySelf(T factor);

		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const SparseVector& spVec)
		{
			return spVec.printOn(s);
		}
	};
	template<typename T>
	inline double SparseVector<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	template<typename T>
	inline size_t SparseVector<T>::numberOfElements()
	{
		return n;
	}
	template<typename T>
	inline double SparseVector<T>::sumOfSquares()
	{
		double sum = 0.0;
		for (auto const& keyValue : *this)
		{
			sum += keyValue.second * keyValue.second;
		}
		return sum;
	}
	template<typename T>
	inline void SparseVector<T>::atiput(size_t i, T value)
	{
		(*this)[i] = value;
	}
	template<>
	inline void SparseVector<double>::atiplusNumber(size_t i, double value)
	{
		(*this)[i] += value;
	}
	template<typename T>
	inline void SparseVector<T>::atiminusNumber(size_t i, double value)
	{
		(*this)[i] -= value;
	}
	template<>
	inline void SparseVector<double>::zeroSelf()
	{
		this->clear();
	}
	template<typename T>
	inline double SparseVector<T>::maxMagnitude()
	{
		double max = 0.0;
		for (const auto& keyValue : *this) {
			auto val = keyValue.second;
			if (val < 0.0) val = -val;
			if (max < val) max = val;
		}
		return max;
	}
	template<typename T>
	inline void SparseVector<T>::magnifySelf(T factor)
	{
		for (const auto& keyValue : *this) {
			auto key = keyValue.first;
			auto val = keyValue.second;
			val *= factor;
			this->at(key) = val;
		}
	}
	template<typename T>
	inline std::ostream& SparseVector<T>::printOn(std::ostream& s) const
	{

		std::stringstream ss;
		ss << std::setprecision(std::numeric_limits<double>::max_digits10);
		ss << "{" << std::endl;
		auto index = 0;
		for (const auto& keyValue : *this) {
			if (index > 0) ss << ", " << std::endl;
			ss << keyValue.first;
			ss << "->";
			ss << keyValue.second;
			index++;
		}
		ss << std::endl << "}";
		s << ss.str();
		return s;
	}
}

