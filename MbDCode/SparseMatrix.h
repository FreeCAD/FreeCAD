#pragma once
#include <sstream> 

#include "RowTypeMatrix.h"
#include "SparseRow.h"
#include "DiagonalMatrix.h"

namespace MbD {
	template<typename T>
	class SparseMatrix : public RowTypeMatrix<std::shared_ptr<SparseRow<T>>>
	{
	public:
		SparseMatrix(int m) : RowTypeMatrix<std::shared_ptr<SparseRow<T>>>(m)
		{
		}
		SparseMatrix(int m, int n) {
			for (int i = 0; i < m; i++)
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
		void atiput(int i, std::shared_ptr<SparseRow<T>> spRow);
		void atijplusDiagonalMatrix(int i, int j, std::shared_ptr<DiagonalMatrix<double>> diagMat);
		void atijminusDiagonalMatrix(int i, int j, std::shared_ptr<DiagonalMatrix<double>> diagMat);
		double sumOfSquares() override;
		void zeroSelf() override;
		void atijplusFullRow(int i, int j, std::shared_ptr<FullRow<T>> fullRow);
		void atijplusFullColumn(int i, int j, std::shared_ptr<FullColumn<T>> fullCol);
		void atijplusFullMatrix(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat);
		void atijminusFullMatrix(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat);
		void atijplusTransposeFullMatrix(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat);
		void atijplusFullMatrixtimes(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat, T factor);
		void atijplusNumber(int i, int j, double value);
		void atijminusNumber(int i, int j, double value);
		void atijput(int i, int j, T value);

		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const SparseMatrix& spMat)
		{
			return spMat.printOn(s);
		}
		std::shared_ptr<FullColumn<T>> timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol);

	};
	using SpMatDsptr = std::shared_ptr<SparseMatrix<double>>;

	template<typename T>
	inline void SparseMatrix<T>::atiput(int i, std::shared_ptr<SparseRow<T>> spRow)
	{
		this->at(i) = spRow;
	}

	template<typename T>
	inline void SparseMatrix<T>::atijplusDiagonalMatrix(int i, int j, std::shared_ptr<DiagonalMatrix<double>> diagMat)
	{
		auto n = diagMat->nrow();
		for (int ii = 0; ii < n; ii++)
		{
			this->atijplusNumber(i + ii, j + ii, diagMat->at(ii));
		}
	}

	template<>
	inline void SparseMatrix<double>::atijminusDiagonalMatrix(int i1, int j1, std::shared_ptr<DiagonalMatrix<double>> diagMat)
	{
		auto n = diagMat->nrow();
		for (int ii = 0; ii < n; ii++)
		{
			this->atijminusNumber(i1 + ii, j1 + ii, diagMat->at(ii));
		}
	}
	template<typename T>
	inline double SparseMatrix<T>::sumOfSquares()
	{
		double sum = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			sum += this->at(i)->sumOfSquares();
		}
		return sum;
	}
	template<>
	inline void SparseMatrix<double>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullRow(int i, int j, std::shared_ptr<FullRow<T>> fullRow)
	{
		this->at(i)->atiplusFullRow(j, fullRow);
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullColumn(int i, int j, std::shared_ptr<FullColumn<T>> fullCol)
	{
		for (int ii = 0; ii < fullCol->size(); ii++)
		{
			this->atijplusNumber(i + ii, j, fullCol->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullMatrix(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat)
	{
		for (int ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->at(i + ii)->atiplusFullRow(j, fullMat->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijminusFullMatrix(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat)
	{
		for (int ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->at(i + ii)->atiminusFullRow(j, fullMat->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusTransposeFullMatrix(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat)
	{
		for (int ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->atijplusFullColumn(i, j + ii, fullMat->at(ii)->transpose());
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullMatrixtimes(int i, int j, std::shared_ptr<FullMatrix<T>> fullMat, T factor)
	{
		for (int ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->at(i + ii)->atiplusFullRowtimes(j, fullMat->at(ii), factor);
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusNumber(int i, int j, double value)
	{
		this->at(i)->atiplusNumber(j, value);
	}
	template<typename T>
	inline void SparseMatrix<T>::atijminusNumber(int i, int j, double value)
	{
		this->at(i)->atiminusNumber(j, value);
	}
	template<typename T>
	inline void SparseMatrix<T>::atijput(int i, int j, T value)
	{
		this->at(i)->atiput(j, value);
	}
	template<typename T>
	inline std::ostream& SparseMatrix<T>::printOn(std::ostream& s) const
	{
		s << "SpMat[" << std::endl;
		for (int i = 0; i < this->size(); i++)
		{
			s << *(this->at(i)) << std::endl;
		}
		s << "]" << std::endl;
		return s;
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> SparseMatrix<T>::timesFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."
		auto nrow = this->nrow();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i)->timesFullColumn(fullCol);
		}
		return answer;
	}
}