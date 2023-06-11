#pragma once

#include "Array.h"
#include "FullColumn.h"

namespace MbD {
	template <typename T>
	class DiagonalMatrix : public Array<T>
	{
		//
	public:
		DiagonalMatrix(int count) : Array<T>(count) {}
		DiagonalMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void atiputDiagonalMatrix(int i, std::shared_ptr < DiagonalMatrix<T>> diagMat);
		std::shared_ptr<FullColumn<T>> timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
		int nrow() {
			return (int) this->size();
		}
		int ncol() {
			return (int) this->size();
		}
		double sumOfSquares() override;
		int numberOfElements() override;
		void zeroSelf() override;
	};
	template<typename T>
	inline void DiagonalMatrix<T>::atiputDiagonalMatrix(int i, std::shared_ptr<DiagonalMatrix<T>> diagMat)
	{
		for (int ii = 0; ii < diagMat->size(); ii++)
		{
			this->at(i + ii) = diagMat->at(ii);
		}
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> DiagonalMatrix<T>::timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."

		auto nrow = (int) this->size();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * fullCol->at(i);
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
		auto n = (int) this->size();
		return n * n;
	}
	template<>
	inline void DiagonalMatrix<double>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;;
		}
	}
}

