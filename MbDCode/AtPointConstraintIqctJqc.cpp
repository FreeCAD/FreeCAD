#include "AtPointConstraintIqctJqc.h"
#include "DispCompIeqctJeqcO.h"
#include "CREATE.h"

using namespace MbD;

AtPointConstraintIqctJqc::AtPointConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIqcJqc(frmi, frmj, axisi)
{
}

void MbD::AtPointConstraintIqctJqc::initializeGlobally()
{
	riIeJeO->initializeGlobally();
	ppGpEJpEJ = std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO)->ppriIeJeOpEJpEJ;
}

void AtPointConstraintIqctJqc::initriIeJeO()
{
	riIeJeO = CREATE<DispCompIeqctJeqcO>::With(frmI, frmJ, axis);
}

void MbD::AtPointConstraintIqctJqc::calcPostDynCorrectorIteration()
{
	//"ppGpEIpEI is no longer constant."

	ppGpEIpEI = std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO)->ppriIeJeOpEIpEI;
	AtPointConstraintIqcJqc::calcPostDynCorrectorIteration();
}

MbD::ConstraintType MbD::AtPointConstraintIqctJqc::type()
{
	return MbD::essential;
}

void MbD::AtPointConstraintIqctJqc::preVelIC()
{
	AtPointConstraintIJ::preVelIC();
	pGpt = std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO)->priIeJeOpt;
}

void MbD::AtPointConstraintIqctJqc::fillVelICError(FColDsptr col)
{
	col->atiminusNumber(iG, pGpt);
}

void MbD::AtPointConstraintIqctJqc::fillAccICIterError(FColDsptr col)
{
	AtPointConstraintIqcJqc::fillAccICIterError(col);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto qEdotI = efrmIqc->qEdot();
	double sum = (ppGpEIpt->timesFullColumn(qEdotI)) * 2.0;
	sum += ppGptpt;
	col->atiplusNumber(iG, sum);
}

void MbD::AtPointConstraintIqctJqc::preAccIC()
{
	AtPointConstraintIJ::preAccIC();
	ppGpEIpt = std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO)->ppriIeJeOpEIpt;
	ppGptpt = std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO)->ppriIeJeOptpt;
}
