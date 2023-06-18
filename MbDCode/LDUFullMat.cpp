#include "LDUFullMat.h"

using namespace MbD;

FColDsptr MbD::LDUFullMat::basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	this->decomposesaveOriginal(fullMat, saveOriginal);
	FColDsptr answer = this->forAndBackSubsaveOriginal(fullCol, saveOriginal);
	return answer;
}

FColDsptr MbD::LDUFullMat::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
	return FColDsptr();
}

void MbD::LDUFullMat::preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

void MbD::LDUFullMat::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

void MbD::LDUFullMat::forwardEliminateWithPivot(int p)
{
	//"Save factors in lower triangle for LU decomposition."

		//| rowp app rowi aip factor |
	auto rowp = matrixA->at(p);
	auto app = rowp->at(p);
	for (int i = p + 1; i < m; i++)
	{
		auto rowi = matrixA->at(i);
		auto aip = rowi->at(p);
		auto factor = aip / app;
		rowi->at(p) = factor;
		if (factor != 0) {
			for (int j = p + 1; j < n; j++)
			{
				rowi->atiminusNumber(j, factor * rowp->at(j));
			}
		}
	}
}

void MbD::LDUFullMat::postSolve()
{
	assert(false);
}

void MbD::LDUFullMat::preSolvesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	if (saveOriginal) {
		matrixA = fullMat->copy();
	}
	else {
		matrixA = fullMat;
	}
	if (m != matrixA->nrow() || n != matrixA->ncol()) {
		m = matrixA->nrow();
		n = matrixA->ncol();
		pivotValues = std::make_shared<FullColumn<double>>(m);
		rowOrder = std::make_shared<FullColumn<int>>(m);
		colOrder = std::make_shared<FullRow<int>>(n);
	}
	if (m == n) {
		for (int i = 0; i < m; i++)
		{
			rowOrder->at(i) = i;
			colOrder->at(i) = i;
		}
	}
	else {
		for (int i = 0; i < m; i++)
		{
			rowOrder->at(i) = i;
		}
		for (int j = 0; j < n; j++)
		{
			colOrder->at(j) = j;
		}
	}
	this->findScalingsForRowRange(0, m);
}

void MbD::LDUFullMat::decomposesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	this->preSolvesaveOriginal(fullMat, saveOriginal);
	for (int p = 0; p < m; p++)
	{
		this->doPivoting(p);
		this->forwardEliminateWithPivot(p);
	}
}

void MbD::LDUFullMat::decomposesaveOriginal(SpMatDsptr spMat, bool saveOriginal)
{
	assert(false);
}

FMatDsptr MbD::LDUFullMat::inversesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	//"ForAndBackSub be optimized for the identity matrix."

	this->decomposesaveOriginal(fullMat, saveOriginal);
	rightHandSideB = std::make_shared<FullColumn<double>>(m);
	auto matrixAinverse = std::make_shared <FullMatrix<double>>(m, n);
	for (int j = 0; j < n; j++)
	{
		rightHandSideB->zeroSelf();
		rightHandSideB->at(j) = 1.0;
		this->forAndBackSubsaveOriginal(rightHandSideB, saveOriginal);
		matrixAinverse->atijputFullColumn(0, j, answerX);
	}
	return matrixAinverse;
}

double MbD::LDUFullMat::getmatrixArowimaxMagnitude(int i)
{
	return matrixA->at(i)->maxMagnitude();
}

void MbD::LDUFullMat::forwardSubstituteIntoL()
{
	//"L is lower triangular with nonzero and ones in diagonal."
	auto vectorc = std::make_shared<FullColumn<double>>(n);
	for (int i = 0; i < n; i++)
	{
		auto rowi = matrixA->at(i);
		double sum = 0.0;
		for (int j = 0; j < i - 1; j++)
		{
			sum += rowi->at(j) * vectorc->at(j);
		}
		vectorc->at(i) = rightHandSideB->at(i) - sum;
	}
	rightHandSideB = vectorc;
}

void MbD::LDUFullMat::backSubstituteIntoDU()
{
	//"DU is upper triangular with nonzero and arbitrary diagonals."

		//| rowi sum |
	answerX = std::make_shared<FullColumn<double>>(n);
	answerX->at(n - 1) = rightHandSideB->at(m - 1) / matrixA->at(m - 1)->at(n - 1);
	for (int i = n-2; i >= 0; i--)
	{
		auto rowi = matrixA->at(i);
		double sum = answerX->at(n) * rowi->at(n);
		for (int j = i + 1; j < n - 1; j++)
		{
			sum += answerX->at(j) * rowi->at(j);
		}
		answerX->at(i) = (rightHandSideB->at(i) - sum) / rowi->at(i);
	}
}
