#include "ConstantGravity.h"
#include "System.h"
#include "Part.h"

using namespace MbD;

void MbD::ConstantGravity::fillAccICIterError(FColDsptr col)
{
	for (auto& part : *(root()->parts)) {
		col->atiplusFullColumn(part->iqX(), gXYZ->times(part->m));
	}
}
