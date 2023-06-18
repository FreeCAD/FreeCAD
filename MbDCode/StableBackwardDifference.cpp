#include "StableBackwardDifference.h"

using namespace MbD;

void MbD::StableBackwardDifference::formTaylorMatrix()
{
	//This form is numerically more stable and is prefered over the full Taylor Matrix.
	//For method order 3:
	//| (t1 - t)	(t1 - t) ^ 2 / 2!	(t1 - t) ^ 3 / 3!|	|qd(t)  | = | q(t1) - q(t)	|
	//|	(t2 - t)	(t2 - t) ^ 2 / 2!	(t2 - t) ^ 3 / 3!|	|qdd(t)	|	|q(t2) - q(t)	|
	//|	(t3 - t)	(t3 - t) ^ 2 / 2!	(t3 - t) ^ 3 / 3!|	|qddd(t)|	|q(t3) - q(t)	|

	this->instantiateTaylorMatrix();
	for (int i = 0; i < order; i++)
	{
		this->formTaylorRowwithTimeNodederivative(i, i, 0);
	}
}

void MbD::StableBackwardDifference::instantiateTaylorMatrix()
{
	if (taylorMatrix == nullptr || (taylorMatrix->nrow() != (order))) {
		taylorMatrix = std::make_shared<FullMatrix<double>>(order, order);
	}
}

void MbD::StableBackwardDifference::formTaylorRowwithTimeNodederivative(int i, int ii, int k)
{
	//| rowi hi hipower aij |
	auto& rowi = taylorMatrix->at(i);
	if (k > 0) {
		for (int j = 0; j < k - 1; j++)
		{
			rowi->at(j) = 0.0;
		}
		rowi->at(k) = 1.0;
	}

	auto hi = timeNodes->at(ii) - time;
	auto hipower = 1.0;
	for (int j = k; j < order; j++)
	{
		hipower *= hi;
		auto aij = hipower * OneOverFactorials->at((size_t)(j - k));
		rowi->at(j) = aij;
	}
}
