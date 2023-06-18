#include "LDUFullMatParPv.h"
#include "FullMatrix.h"
#include "SingularMatrixError.h"

using namespace MbD;

void MbD::LDUFullMatParPv::doPivoting(int p)
{
	//"Use scalings. Do row pivoting."

	//| app max rowPivot aip mag |
	auto app = matrixA->at(p)->at(p);
	auto max = app * rowScalings->at(p);
	if (max < 0.0) max = -max;
	auto rowPivot = p;
	for (int i = p + 1; i < m; i++)
	{
		auto aip = matrixA->at(i)->at(p);
		if (aip != 0.0) {
			auto mag = aip * rowScalings->at(i);
			if (mag < 0) mag = -mag;
			if (max < mag) {
				max = mag;
				rowPivot = i;
			}
		}
	}
	if (p != rowPivot) {
		matrixA->swapElems(p, rowPivot);
		rowScalings->swapElems(p, rowPivot);
		rowOrder->swapElems(p, rowPivot);
	}
	pivotValues->at(p) = max;
	if (max < singularPivotTolerance) throw SingularMatrixError("");
}
