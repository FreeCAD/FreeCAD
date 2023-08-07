#pragma once

#include "Array.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
	template<typename T>
	class DiagonalMatrix;
	template<typename T>
	using DiagMatsptr = std::shared_ptr<DiagonalMatrix<T>>;
	using DiagMatDsptr = std::shared_ptr<DiagonalMatrix<double>>;

	template<typename T>
	class DiagonalMatrix : public Array<T>
	{
		//
	public:
		DiagonalMatrix(int count) : Array<T>(count) {}
		DiagonalMatrix(int count, const T& value) : Array<T>(count, value) {}
		DiagonalMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void atiputDiagonalMatrix(int i, std::shared_ptr < DiagonalMatrix<T>> diagMat);
		DiagMatsptr<T> times(T factor);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FMatsptr<T> timesFullMatrix(FMatsptr<T> fullMat);
		int nrow() {
			return (int)this->size();
		}
		int ncol() {
			return (int)this->size();
		}
		double sumOfSquares() override;
		int numberOfElements() override;
		void zeroSelf() override;
		double maxMagnitude() override;

		std::ostream& printOn(std::ostream& s) const override;

	};
	template<typename T>
	inline void DiagonalMatrix<T>::atiputDiagonalMatrix(int i, DiagMatsptr<T> diagMat)
	{
		for (int ii = 0; ii < diagMat->size(); ii++)
		{
			this->at(i + ii) = diagMat->at(ii);
		}
	}
	template<>
	inline DiagMatDsptr DiagonalMatrix<double>::times(double factor)
	{
		auto nrow = (int)this->size();
		auto answer = std::make_shared<DiagonalMatrix<double>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * factor;
		}
		return answer;
	}
	template<typename T>
	inline DiagMatsptr<T> DiagonalMatrix<T>::times(T factor)
	{
		assert(false);
	}
	template<typename T>
	inline FColsptr<T> DiagonalMatrix<T>::timesFullColumn(FColsptr<T> fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."

		auto nrow = (int)this->size();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> DiagonalMatrix<T>::timesFullMatrix(FMatsptr<T> fullMat)
	{
		auto nrow = (int)this->size();
		auto answer = std::make_shared<FullMatrix<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = fullMat->at(i)->times(this->at(i));
		}
		return answer;
	}
	template<>
	inline double DiagonalMatrix<double>::sumOfSquares()
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
	inline int DiagonalMatrix<T>::numberOfElements()
	{
		auto n = (int)this->size();
		return n * n;
	}
	template<>
	inline void DiagonalMatrix<double>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;
		}
	}
	template<>
	inline double DiagonalMatrix<double>::maxMagnitude()
	{
		auto max = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			auto element = this->at(i);
			if (element < 0.0) element = -element;
			if (max < element) max = element;
		}
		return max;
	}
	template<typename T>
	inline double DiagonalMatrix<T>::maxMagnitude()
	{
		assert(false);
		return 0.0;
	}
	template<typename T>
	inline std::ostream& DiagonalMatrix<T>::printOn(std::ostream& s) const
	{
		s << "DiagMat[";
		s << this->at(0);
		for (int i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "]";
		return s;
	}
}

