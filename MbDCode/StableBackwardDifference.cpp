#include "StableBackwardDifference.h"

void MbD::StableBackwardDifference::formTaylorMatrix()
{
	//This form is numerically more stable and is prefered over the full Taylor Matrix.
	//For method order 3:
	//| (t1 - t)	(t1 - t) ^ 2 / 2!	(t1 - t) ^ 3 / 3!|	|qd(t)  | = | q(t1) - q(t)	|
	//|	(t2 - t)	(t2 - t) ^ 2 / 2!	(t2 - t) ^ 3 / 3!|	|qdd(t)	|	|q(t2) - q(t)	|
	//|	(t3 - t)	(t3 - t) ^ 2 / 2!	(t3 - t) ^ 3 / 3!|	|qddd(t)|	|q(t3) - q(t)	|

	this->instanciateTaylorMatrix();
	for (int i = 0; i < order; i++)
	{
		this->formTaylorRowwithTimeNodederivative(i, i, 0);
	}
}

void MbD::StableBackwardDifference::instanciateTaylorMatrix()
{
	if (taylorMatrix->empty() || (taylorMatrix->nrow() != (order))) {
		taylorMatrix = std::make_shared<FullMatrix<double>>(order, order);
	}
}

void MbD::StableBackwardDifference::formTaylorRowwithTimeNodederivative(int i, int ii, int k)
{
	assert(false);
}
