#include "MatrixDecomposition.h"

using namespace MbD;

void MatrixDecomposition::applyRowOrderOnRightHandSideB()
{
	FColDsptr answer = std::make_shared<FullColumn<double>>(m);
	for (int i = 0; i < m; i++)
	{
		answer->at(i) = rightHandSideB->at(rowOrder->at(i));
	}
	rightHandSideB = answer;
}
