#include "DifferenceOperator.h"
#include "CREATE.h"
#include "SingularMatrixError.h"
#include "LDUFullMatParPv.h"

void MbD::DifferenceOperator::calcOperatorMatrix()
{
	//Compute operatorMatrix such that 
	//value(time) : = (operatorMatrix at : 1) timesColumn : series.
	//valuedot(time) : = (operatorMatrix at : 2) timesColumn : series.
	//valueddot(time) : = (operatorMatrix at : 3) timesColumn : series.

	this->formTaylorMatrix();
	try {
		assert(false);
		//operatorMatrix = CREATE<LDUFullMatParPv>::With()->inversesaveOriginal(taylorMatrix, false);
	}
	catch (SingularMatrixError ex) {
	}
}

void MbD::DifferenceOperator::initialize()
{
	//OneOverFactorials: = StMFullRow new : 10.
	//1 to : OneOverFactorials size do : [:i | OneOverFactorials at : i put : 1.0d / i factorial]
}

void MbD::DifferenceOperator::setiStep(int i)
{
	iStep = i;
}

void MbD::DifferenceOperator::setorder(int o)
{
	order = o;
}

void MbD::DifferenceOperator::instanciateTaylorMatrix()
{
	if (taylorMatrix->empty() || (taylorMatrix->nrow() != (order + 1))) {
		taylorMatrix = std::make_shared<FullMatrix<double>>(order + 1, order + 1);
	}
}

void MbD::DifferenceOperator::formTaylorRowwithTimeNodederivative(int i, int ii, int k)
{
	//| rowi hi hipower aij |
	auto rowi = taylorMatrix->at(i);
	for (int j = 0; j < k; j++)
	{
		rowi->at(j) = 0.0;
	}
	rowi->at(k) = 1.0;
	auto hi = timeNodes->at(ii) - time;
	auto hipower = 1.0;
	for (int j = k + 1; j < order + 1; j++)
	{
		hipower = hipower * hi;
		assert(false);
		//aij = hipower * (OneOverFactorials at : j - k - 1);
		//rowi at : j put : aij
	}
}
