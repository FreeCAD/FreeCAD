/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <string>
#include <sstream> 

#include "FullVector.h"

namespace MbD {
	template<typename T>
	class FullColumn;
	using FColDsptr = std::shared_ptr<FullColumn<double>>;
	template<typename T>
	using FColsptr = std::shared_ptr<FullColumn<T>>;
	template<typename T>
	class FullRow;
	template<typename T>
	using FRowsptr = std::shared_ptr<FullRow<T>>;
	class Symbolic;

	template<typename T>
	class FullColumn : public FullVector<T>
	{
	public:
		FullColumn() : FullVector<T>() {}
		FullColumn(std::vector<T> vec) : FullVector<T>(vec) {}
		FullColumn(size_t count) : FullVector<T>(count) {}
		FullColumn(size_t count, const T& value) : FullVector<T>(count, value) {}
		FullColumn(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end) : FullVector<T>(begin, end) {}
		FullColumn(std::initializer_list<T> list) : FullVector<T>{ list } {}
		FColsptr<T> plusFullColumn(FColsptr<T> fullCol);
		FColsptr<T> minusFullColumn(FColsptr<T> fullCol);
		FColsptr<T> times(T a);
		FColsptr<T> negated();
		void atiputFullColumn(size_t i, FColsptr<T> fullCol);
		void atiplusFullColumn(size_t i, FColsptr<T> fullCol);
		void equalSelfPlusFullColumnAt(FColsptr<T> fullCol, size_t i);
		void atiminusFullColumn(size_t i, FColsptr<T> fullCol);
		void equalFullColumnAt(FColsptr<T> fullCol, size_t i);
		FColsptr<T> copy();
		FRowsptr<T> transpose();
		void atiplusFullColumntimes(size_t i, FColsptr<T> fullCol, T factor);
		T transposeTimesFullColumn(const FColsptr<T> fullCol);		
		void equalSelfPlusFullColumntimes(FColsptr<T> fullCol, T factor);
		FColsptr<T> cross(FColsptr<T> fullCol);
		FColsptr<T> simplified();
		double dot(std::shared_ptr<FullVector<T>> vec);
		std::shared_ptr<FullVector<T>> dot(std::shared_ptr<std::vector<std::shared_ptr<FullColumn<T>>>> vecvec);

		std::ostream& printOn(std::ostream& s) const override;
	};
	template<typename T>
	inline FColsptr<T> FullColumn<T>::plusFullColumn(FColsptr<T> fullCol)
	{
		auto n =  this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) + fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FColsptr<T> FullColumn<T>::minusFullColumn(FColsptr<T> fullCol)
	{
		auto n =  this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) - fullCol->at(i);
		}
		return answer;
	}
	template<>
	inline FColDsptr FullColumn<double>::times(double a)
	{
		auto n = this->size();
		auto answer = std::make_shared<FullColumn<double>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i) * a;
		}
		return answer;
	}
	template<typename T>
	inline FColsptr<T> FullColumn<T>::times(T)
	{
		assert(false);
	}
	template<typename T>
	inline FColsptr<T> FullColumn<T>::negated()
	{
		return this->times(-1.0);
	}
	template<typename T>
	inline void FullColumn<T>::atiputFullColumn(size_t i, FColsptr<T> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			this->at(i + ii) = fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullColumn<T>::atiplusFullColumn(size_t i, FColsptr<T> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			this->at(i + ii) += fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullColumn<T>::equalSelfPlusFullColumnAt(FColsptr<T> fullCol, size_t ii)
	{
		//self is subcolumn of fullCol
		for (size_t i = 0; i < this->size(); i++)
		{
			this->at(i) += fullCol->at(ii + i);
		}
	}
	template<typename T>
	inline void FullColumn<T>::atiminusFullColumn(size_t i1, FColsptr<T> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			size_t i = i1 + ii;
			this->at(i) -= fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullColumn<T>::equalFullColumnAt(FColsptr<T> fullCol, size_t i)
	{
		this->equalArrayAt(fullCol, i);
		//for (size_t ii = 0; ii < this->size(); ii++)
		//{
		//	this->at(ii) = fullCol->at(i + ii);
		//}
	}
	template<>
	inline FColDsptr FullColumn<double>::copy()
	{
		auto n =  this->size();
		auto answer = std::make_shared<FullColumn<double>>(n);
		for (size_t i = 0; i < n; i++)
		{
			answer->at(i) = this->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FRowsptr<T> FullColumn<T>::transpose()
	{
		return std::make_shared<FullRow<T>>(*this);
	}
	template<typename T>
	inline void FullColumn<T>::atiplusFullColumntimes(size_t i1, FColsptr<T> fullCol, T factor)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			size_t i = i1 + ii;
			this->at(i) += fullCol->at(ii) * factor;
		}
	}
	template<typename T>
	inline T FullColumn<T>::transposeTimesFullColumn(const FColsptr<T> fullCol)
	{
		return this->dot(fullCol);
	}
	template<typename T>
	inline void FullColumn<T>::equalSelfPlusFullColumntimes(FColsptr<T> fullCol, T factor)
	{
		this->equalSelfPlusFullVectortimes(fullCol, factor);
	}
	template<typename T>
	inline FColsptr<T> FullColumn<T>::cross(FColsptr<T> fullCol)
	{
			auto a0 = this->at(0);
			auto a1 = this->at(1);
			auto a2 = this->at(2);
			auto b0 = fullCol->at(0);
			auto b1 = fullCol->at(1);
			auto b2 = fullCol->at(2);
			auto answer = std::make_shared<FullColumn<T>>(3);
			answer->atiput(0, a1 * b2 - (a2 * b1));
			answer->atiput(1, a2 * b0 - (a0 * b2));
			answer->atiput(2, a0 * b1 - (a1 * b0));
			return answer;
	}
	//template<>
	//inline std::shared_ptr<FullColumn<Symsptr>> FullColumn<Symsptr>::simplified()
	//{
	//	auto n = this->size();
	//	auto answer = std::make_shared<FullColumn<Symsptr>>(n);
	//	for (size_t i = 0; i < n; i++)
	//	{
	//		auto func = this->at(i);
	//		answer->at(i) = func->simplified(func);
	//	}
	//	return answer;
	//}
	template<typename T>
	inline FColsptr<T> FullColumn<T>::simplified()
	{
		assert(false);
		return FColsptr<T>();
	}
	template<typename T>
	inline double FullColumn<T>::dot(std::shared_ptr<FullVector<T>> vec)
	{
		auto n = this->size();
		double answer = 0.0;
		for (size_t i = 0; i < n; i++) {
			answer += this->at(i) * vec->at(i);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullVector<T>> FullColumn<T>::dot(std::shared_ptr<std::vector<std::shared_ptr<FullColumn<T>>>> vecvec)
	{
		auto ncol = this->size();
		auto nelem = vecvec->at(0)->size();
		auto answer = std::make_shared<FullVector<T>>(nelem);
		for (size_t k = 0; k < nelem; k++) {
			auto sum = 0.0;
			for (size_t i = 0; i < ncol; i++)
			{
				sum += this->at(i) * vecvec->at(i)->at(k);
			}
			answer->at(k) = sum;
		}
		return answer;
	}
	template<typename T>
	inline std::ostream& FullColumn<T>::printOn(std::ostream& s) const
	{
		s << "FullCol{";
		s << this->at(0);
		for (size_t i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "}";
		return s;
	}
}

