#include "TranslationConstraintIqcJc.h"
#include "DispCompIeqcJecKeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqcJc::TranslationConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	TranslationConstraintIJ(frmi, frmj, axisi)
{
}

void MbD::TranslationConstraintIqcJc::initriIeJeIe()
{
    riIeJeIe = CREATE<DispCompIeqcJecKeqc>::With(frmI, frmJ, frmI, axisI);
}

void MbD::TranslationConstraintIqcJc::calcPostDynCorrectorIteration()
{
	TranslationConstraintIJ::calcPostDynCorrectorIteration();
	auto riIeqJeIeq = std::static_pointer_cast<DispCompIeqcJecKeqc>(riIeJeIe);
	pGpXI = riIeqJeIeq->pvaluepXI();
	pGpEI = (riIeqJeIeq->pvaluepEI())->plusFullRow(riIeqJeIeq->pvaluepEK());
	ppGpXIpEI = riIeqJeIeq->ppvaluepXIpEK();
	ppGpEIpEI = riIeqJeIeq->ppvaluepEIpEI()->plusFullMatrix(riIeqJeIeq->ppvaluepEIpEK())
		->plusFullMatrix((riIeqJeIeq->ppvaluepEIpEK()->transpose()->plusFullMatrix(riIeqJeIeq->ppvaluepEKpEK())));
}

void MbD::TranslationConstraintIqcJc::useEquationNumbers()
{
	iqXI = std::static_pointer_cast<EndFrameqc>(frmI)->iqX();
	iqEI = std::static_pointer_cast<EndFrameqc>(frmI)->iqE();
}

void MbD::TranslationConstraintIqcJc::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::TranslationConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	auto ppGpXIpEIlam = ppGpXIpEI->times(lam);
	mat->atijplusFullMatrix(iqXI, iqEI, ppGpXIpEIlam);
	mat->atijplusTransposeFullMatrix(iqEI, iqXI, ppGpXIpEIlam);
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}
