#pragma once
#include "RowTypeMatrix.h"
#include "SparseRow.h"
#include "DiagonalMatrix.h"

namespace MbD {
	template <typename T>
	class SparseMatrix : public RowTypeMatrix<std::shared_ptr<SparseRow<T>>>
	{
	public:
		SparseMatrix(size_t m) : RowTypeMatrix<std::shared_ptr<SparseRow<T>>>(m)
		{
		}
		SparseMatrix(size_t m, size_t n) {
			for (size_t i = 0; i < m; i++)
			{
				auto row = std::make_shared<SparseRow<T>>(n);
				this->push_back(row);
			}
		}
		SparseMatrix(std::initializer_list<std::initializer_list<std::initializer_list<double>>> list2D) {
			for (auto& rowList : list2D)
			{
				auto row = std::make_shared<SparseRow<T>>(rowList);
				this->push_back(row);
			}
		}
		void atijminusDiagonalMatrix(size_t i, size_t j, std::shared_ptr<DiagonalMatrix<double>> diagMat);
		double sumOfSquares() override;
		void atijplusNumber(size_t i, size_t j, double value);
		void zeroSelf() override;
	};
	using SpMatDsptr = std::shared_ptr<SparseMatrix<double>>;

	template<>
	inline void SparseMatrix<double>::atijminusDiagonalMatrix(size_t i1, size_t j1, std::shared_ptr<DiagonalMatrix<double>> diagMat)
	{
		auto n = diagMat->nrow();
		for (size_t ii = 0; ii < n; ii++)
		{
			this->at(i1 + ii)->atiminusNumber(j1 + ii, diagMat->at(ii));
		}
	}
	template<typename T>
	inline double SparseMatrix<T>::sumOfSquares()
	{
		double sum = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			sum += this->at(i)->sumOfSquares();
		}
		return sum;
	}
	template<>
	inline void SparseMatrix<double>::atijplusNumber(size_t i, size_t j, double value)
	{
		this->at(i)->atiplusNumber(j, value);
	}
	template<>
	inline void SparseMatrix<double>::zeroSelf()
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
}