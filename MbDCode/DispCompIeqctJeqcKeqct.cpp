#include "DispCompIeqctJeqcKeqct.h"
#include "EndFrameqct.h"

using namespace MbD;

MbD::DispCompIeqctJeqcKeqct::DispCompIeqctJeqcKeqct()
{
}

MbD::DispCompIeqctJeqcKeqct::DispCompIeqctJeqcKeqct(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk) : DispCompIeqcJeqcKeqct(frmi, frmj, frmk, axisk)
{
}

void MbD::DispCompIeqctJeqcKeqct::preVelIC()
{
	DispCompIeqcJeqcKeqct::preVelIC();
	auto mprIeJeOpt = std::static_pointer_cast<EndFrameqct>(frmI)->prOeOpt;
	priIeJeKept -= aAjOKe->dot(mprIeJeOpt);
}
