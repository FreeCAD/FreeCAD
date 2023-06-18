#include "LDUSpMat.h"
#include "FullColumn.h"

using namespace MbD;

FColDsptr MbD::LDUSpMat::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->decomposesaveOriginal(spMat, saveOriginal);
	FColDsptr answer = this->forAndBackSubsaveOriginal(fullCol, saveOriginal);
	return answer;
}

void MbD::LDUSpMat::decomposesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	assert(false);
}

void MbD::LDUSpMat::decomposesaveOriginal(SpMatDsptr spMat, bool saveOriginal)
{
	assert(false);
}

FColDsptr MbD::LDUSpMat::forAndBackSubsaveOriginal(FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
	return FColDsptr();
}

double MbD::LDUSpMat::getmatrixArowimaxMagnitude(int i)
{
	return matrixA->at(i)->maxMagnitude();
}

void MbD::LDUSpMat::forwardSubstituteIntoL()
{
	//"L is lower triangular with nonzero and ones in diagonal."
	auto vectorc = std::make_shared<FullColumn<double>>(n);
	vectorc->at(0) = rightHandSideB->at(0);
	for (int i = 1; i < n; i++)
	{
		auto rowi = matrixA->at(i);
		double sum = 0.0;
		for (auto const& keyValue : *rowi) {
			int j = keyValue.first;
			double duij = keyValue.second;
			sum += duij * vectorc->at(j);
		}
		vectorc->at(i) = rightHandSideB->at(i) - sum;
	}
	rightHandSideB = vectorc;
}

void MbD::LDUSpMat::backSubstituteIntoDU()
{
	//"DU is upper triangular with nonzero diagonals."

	double sum, duij;
	for (int i = 0; i < m; i++)
	{
		rightHandSideB->at(i) = rightHandSideB->at(i) / matrixD->at(i);
	}
	answerX = std::make_shared<FullColumn<double>>(m);
	answerX->at(n - 1) = rightHandSideB->at(m - 1);
	for (int i = n - 2; i >= 0; i--)
	{
		auto rowi = matrixU->at(i);
		sum = 0.0;
		for (auto const& keyValue : *rowi) {
			auto j = keyValue.first;
			duij = keyValue.second;
			sum += answerX->at(j) * duij;
		}
		answerX->at(i) = rightHandSideB->at(i) - sum;
	}
}
