#include "Part.h"
#include "PartFrame.h"
#include "System.h"
#include "CREATE.h"
#include "DiagonalMatrix.h"
#include "EulerParameters.h"
#include "DataPosVelAcc.h"


using namespace MbD;

Part::Part() {
}

Part::Part(const char* str) : Item(str) {
}

System* MbD::Part::root()
{
	return system;
}

void Part::initialize()
{
	partFrame = CREATE<PartFrame>::With();
	partFrame->setPart(this);
	pTpE = std::make_shared<FullColumn<double>>(4);
	ppTpEpE = std::make_shared<FullMatrix<double>>(4, 4);
	ppTpEpEdot = std::make_shared<FullMatrix<double>>(4, 4);
}

void Part::initializeLocally()
{
	partFrame->initializeLocally();
	if (m > 0) {
		mX = std::make_shared<DiagonalMatrix<double>>(3, m);
	}
	else {
		mX = std::make_shared<DiagonalMatrix<double>>(3, 0.0);
	}
}

void Part::initializeGlobally()
{
	partFrame->initializeGlobally();
}

void Part::setqX(FColDsptr x) {
	partFrame->setqX(x);
}

FColDsptr Part::getqX() {
	return partFrame->getqX();
}

void Part::setqE(FColDsptr x) {
	partFrame->setqE(x);
}

FColDsptr Part::getqE() {
	return partFrame->getqE();
}

void Part::setqXdot(FColDsptr x) {
	partFrame->setqXdot(x);
}

FColDsptr Part::getqXdot() {
	return partFrame->getqXdot();
}

void Part::setomeOpO(FColDsptr x) {
	partFrame->setomeOpO(x);
}

FColDsptr Part::getomeOpO() {
	return partFrame->getomeOpO();
}

void Part::setqXddot(FColDsptr x)
{
	partFrame->setqXddot(x);
}

FColDsptr Part::getqXddot()
{
	return partFrame->getqXddot();
}

void Part::setqEddot(FColDsptr x)
{
	partFrame->setqEddot(x);
}

FColDsptr Part::getqEddot()
{
	return partFrame->getqEddot();
}

void Part::qX(FColDsptr x)
{
	partFrame->qX = x;
}

FColDsptr Part::qX()
{
	return partFrame->qX;
}

void Part::qE(std::shared_ptr<EulerParameters<double>> x)
{
	partFrame->qE = x;
}

std::shared_ptr<EulerParameters<double>> Part::qE()
{
	return partFrame->qE;
}

void Part::qXdot(FColDsptr x)
{
	partFrame->qXdot = x;
}

FColDsptr Part::qXdot()
{
	return partFrame->qXdot;
}

void Part::omeOpO(FColDsptr x)
{
	partFrame->setomeOpO(x);
}

FColDsptr Part::omeOpO()
{
	return partFrame->omeOpO();
}

void Part::qXddot(FColDsptr x)
{
	partFrame->qXddot = x;
}

FColDsptr Part::qXddot()
{
	return partFrame->qXddot;
}

void Part::qEddot(FColDsptr x)
{
	//ToDo: Should store EulerParametersDDot
	//ToDo: Need alpOpO too
	partFrame->qXddot = x;
}

FColDsptr Part::qEddot()
{
	return partFrame->qEddot;
}

FMatDsptr Part::aAOp()
{
	return partFrame->aAOp();
}

FColDsptr Part::alpOpO()
{
	return partFrame->alpOpO();
}

void Part::setSystem(System* sys)
{
	system = sys;
}

void Part::asFixed()
{
	partFrame->asFixed();
}

void Part::postInput()
{
	partFrame->postInput();
	Item::postInput();
}

void Part::calcPostDynCorrectorIteration()
{
	this->calcmE();
	this->calcmEdot();
	this->calcpTpE();
	this->calcppTpEpE();
	this->calcppTpEpEdot();
}

void Part::prePosIC()
{
	partFrame->prePosIC();
}

void Part::prePosKine()
{
	partFrame->prePosKine();
}

void Part::iqX(int eqnNo)
{
	partFrame->iqX = eqnNo;
}

void Part::iqE(int eqnNo)
{
	partFrame->iqE = eqnNo;

}

void Part::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	partFrame->fillEssenConstraints(essenConstraints);
}

void Part::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void Part::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	partFrame->fillConstraints(allConstraints);
}

void Part::fillqsu(FColDsptr col)
{
	partFrame->fillqsu(col);
}

void Part::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	//"Map wqX and wqE according to inertias. (0 to maximum inertia) map to (minw to maxw)"
	//"When the inertias are zero, they are set to a small number for positive definiteness."
	//"They are not set to zero because inertialess part may be underconstrained."
	//"Avoid having two kinds of singularities to confuse redundant constraint removal."
	//"Redundant constraint removal likes equal weights."
	//"wqE(4) = 0.0d is ok because there is always the euler parameter constraint."

	auto mMax = this->root()->maximumMass();
	auto aJiMax = this->root()->maximumMomentOfInertia();
	auto minw = 1.0e3;
	auto maxw = 1.0e6;
	auto wqX = std::make_shared<DiagonalMatrix<double>>(3);
	auto wqE = std::make_shared<DiagonalMatrix<double>>(4);
	if (mMax == 0) { mMax = 1.0; }
	for (int i = 0; i < 3; i++)
	{
		wqX->at(i) = (maxw * m / mMax) + minw;
	}
	if (aJiMax == 0) { aJiMax = 1.0; }
	for (int i = 0; i < 3; i++)
	{
		auto aJi = aJ->at(i);
		wqE->at(i) = (maxw * aJi / aJiMax) + minw;
	}
	wqE->at(3) = minw;
	diagMat->atiputDiagonalMatrix(partFrame->iqX, wqX);
	diagMat->atiputDiagonalMatrix(partFrame->iqE, wqE);
	partFrame->fillqsuWeights(diagMat);
}

void MbD::Part::fillqsuddotlam(FColDsptr col)
{
	partFrame->fillqsuddotlam(col);
}

void Part::fillqsulam(FColDsptr col)
{
	partFrame->fillqsulam(col);
}

void Part::fillqsudot(FColDsptr col)
{
	partFrame->fillqsudot(col);
}

void Part::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	//"wqXdot and wqEdot are set to their respective inertias."
	//"When the inertias are zero, they are set to a small number for positive definiteness."
	//"They are not set to zero because inertialess part may be underconstrained."
	//"wqEdot(4) = 0.0d is ok because there is always the euler parameter constraint."

		//| mMax aJiMax maxInertia minw maxw aJi wqXdot wqEdot |
	auto mMax = this->root()->maximumMass();
	auto aJiMax = this->root()->maximumMomentOfInertia();
	auto maxInertia = std::max(mMax, aJiMax);
	if (maxInertia == 0) maxInertia = 1.0;
	auto minw = 1.0e-12 * maxInertia;
	auto maxw = maxInertia;
	auto wqXdot = std::make_shared<DiagonalMatrix<double>>(3);
	auto wqEdot = std::make_shared<DiagonalMatrix<double>>(4);
	for (int i = 0; i < 3; i++)
	{
		wqXdot->at(i) = (maxw * m / maxInertia) + minw;
		auto aJi = aJ->at(i);
		wqEdot->at(i) = (maxw * aJi / maxInertia) + minw;
	}
	wqEdot->at(3) = minw;
	diagMat->atiputDiagonalMatrix(partFrame->iqX, wqXdot);
	diagMat->atiputDiagonalMatrix(partFrame->iqE, wqEdot);
	partFrame->fillqsudotWeights(diagMat);
}

void Part::useEquationNumbers()
{
	partFrame->useEquationNumbers();
}

void Part::setqsu(FColDsptr col)
{
	partFrame->setqsu(col);
}

void Part::setqsulam(FColDsptr col)
{
	partFrame->setqsulam(col);
}

void Part::setqsudot(FColDsptr col)
{
	partFrame->setqsudot(col);
}

void Part::setqsudotlam(FColDsptr col)
{
	partFrame->setqsudotlam(col);
}

void Part::postPosICIteration()
{
	partFrame->postPosICIteration();
}

void Part::fillPosICError(FColDsptr col)
{
	partFrame->fillPosICError(col);
}

void Part::fillPosICJacob(SpMatDsptr mat)
{
	partFrame->fillPosICJacob(mat);
}

void Part::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
	partFrame->removeRedundantConstraints(redundantEqnNos);
}

void Part::reactivateRedundantConstraints()
{
	partFrame->reactivateRedundantConstraints();
}

void Part::constraintsReport()
{
	partFrame->constraintsReport();
}

void Part::postPosIC()
{
	partFrame->postPosIC();
	this->calcmE();
}

void Part::preDyn()
{
	partFrame->preDyn();
}

void Part::storeDynState()
{
	partFrame->storeDynState();
}

void Part::fillPosKineError(FColDsptr col)
{
	partFrame->fillPosKineError(col);
}

void Part::fillPosKineJacob(SpMatDsptr mat)
{
	partFrame->fillPosKineJacob(mat);
}

void Part::preVelIC()
{
	partFrame->preVelIC();
}

void Part::postVelIC()
{
	partFrame->postVelIC();
	this->calcp();
	this->calcmEdot();
	this->calcpTpE();
	this->calcppTpEpE();
	this->calcppTpEpEdot();
}

void Part::fillVelICError(FColDsptr col)
{
	partFrame->fillVelICError(col);
}

void Part::fillVelICJacob(SpMatDsptr mat)
{
	partFrame->fillVelICJacob(mat);
}

void Part::preAccIC()
{
	partFrame->preAccIC();
	Item::preAccIC();
}

void Part::calcp()
{
	pX = mX->timesFullColumn(partFrame->qXdot);
	pE = mE->timesFullColumn(partFrame->qEdot);
}

void Part::calcpdot()
{
	pXdot = mX->timesFullColumn(partFrame->qXddot);
	pEdot = mEdot->timesFullColumn(partFrame->qEdot)->plusFullColumn(mE->timesFullColumn(partFrame->qEddot));
}

void Part::calcmEdot()
{
	auto aC = partFrame->aC();
	auto aCdot = partFrame->aCdot();
	auto a4J = aJ->times(4.0);
	auto term1 = aC->transposeTimesFullMatrix(a4J->timesFullMatrix(aCdot));
	auto term2 = term1->transpose();
	mEdot = term1->plusFullMatrix(term2);
}

void Part::calcpTpE()
{
	//"pTpE is a column vector."
	auto& qEdot = partFrame->qEdot;
	auto aC = partFrame->aC();
	auto pCpEtimesqEdot = EulerParameters<double>::pCpEtimesColumn(qEdot);
	pTpE = (pCpEtimesqEdot->transposeTimesFullColumn(aJ->timesFullColumn(aC->timesFullColumn(qEdot))))->times(4.0);
}

void Part::calcppTpEpE()
{
	auto& qEdot = partFrame->qEdot;
	auto pCpEtimesqEdot = EulerParameters<double>::pCpEtimesColumn(qEdot);
	auto a4J = aJ->times(4.0);
	ppTpEpE = pCpEtimesqEdot->transposeTimesFullMatrix(a4J->timesFullMatrix(pCpEtimesqEdot));
}

void Part::calcppTpEpEdot()
{
	//| qEdot aC a4J term1 pCpEtimesqEdot term2 |
	auto& qEdot = partFrame->qEdot;
	auto aC = partFrame->aC();
	auto a4J = aJ->times(4.0);
	auto term1 = EulerParameters<double>::pCTpEtimesColumn(a4J->timesFullColumn(aC->timesFullColumn(qEdot)));
	auto pCpEtimesqEdot = EulerParameters<double>::pCpEtimesColumn(qEdot);
	auto term2 = aC->transposeTimesFullMatrix(a4J->timesFullMatrix(pCpEtimesqEdot));
	ppTpEpEdot = term1->plusFullMatrix(term2)->transpose();
}

void Part::calcmE()
{
	auto aC = partFrame->aC();
	mE = aC->transposeTimesFullMatrix(aJ->timesFullMatrix(aC))->times(4.0);
}

void Part::fillAccICIterError(FColDsptr col)
{
	auto iqX = partFrame->iqX;
	auto iqE = partFrame->iqE;
	col->atiminusFullColumn(iqX, mX->timesFullColumn(partFrame->qXddot));
	col->atiminusFullColumn(iqE, mEdot->timesFullColumn(partFrame->qEdot));
	col->atiminusFullColumn(iqE, mE->timesFullColumn(partFrame->qEddot));
	col->atiplusFullColumn(iqE, pTpE);
	partFrame->fillAccICIterError(col);
}

void Part::fillAccICIterJacob(SpMatDsptr mat)
{
	auto iqX = partFrame->iqX;
	auto iqE = partFrame->iqE;
	mat->atijminusDiagonalMatrix(iqX, iqX, mX);
	mat->atijminusFullMatrix(iqE, iqE, mE);
	partFrame->fillAccICIterJacob(mat);
}

std::shared_ptr<EulerParametersDot<double>> Part::qEdot()
{
	return partFrame->qEdot;
}

void Part::setqsuddotlam(FColDsptr col)
{
	partFrame->setqsuddotlam(col);
}

std::shared_ptr<StateData> Part::stateData()
{
	//"
	//P : = part frame.
	//p : = principal frame at cm.
	//rOcmO : = rOPO + aAOP * rPcmP.
	//aAOp : = aAOP * aAPp.
	//vOcmO : = vOPO + aAdotOP * rPcmP
	//: = vOPO + (omeOPO cross : aAOP * rPcmP).
	//omeOpO : = omeOPO.
	//aOcmO : = aOPO + aAddotOP * rPcmP
	//: = aOPO + (alpOPO cross : aAOP * rPcmP) + (omeOPO cross : (omeOPO cross : aAOP * rPcmP)).
	//alpOpO : = alpOPO.

	//Therefore
	//aAOP : = aAOp * aAPpT
	//rOPO : = rOcmO - aAOP * rPcmP.
	//omeOPO : = omeOpO.
	//vOPO : = vOcmO - (omeOPO cross : aAOP * rPcmP).
	//alpOPO : = alpOpO.
	//aOPO : = aOcmO - (alpOPO cross : aAOP * rPcmP) - (omeOPO cross : (omeOPO cross :
	//aAOP * rPcmP)).
	//"

	auto rOpO = this->qX();
	auto aAOp = this->aAOp();
	auto vOpO = this->qXdot();
	auto omeOpO = this->omeOpO();
	auto aOpO = this->qXddot();
	auto alpOpO = this->alpOpO();
	auto answer = std::make_shared<DataPosVelAcc>();
	answer->rFfF = rOpO;
	answer->aAFf = aAOp;
	answer->vFfF = vOpO;
	answer->omeFfF = omeOpO;
	answer->aFfF = aOpO;
	answer->alpFfF = alpOpO;
	return answer;
}

double Part::suggestSmallerOrAcceptDynStepSize(double hnew)
{
	auto hnew2 = hnew;
	hnew2 = partFrame->suggestSmallerOrAcceptDynStepSize(hnew2);
	return hnew2;
}

void Part::postDynStep()
{
	partFrame->postDynStep();
}
