#include "Part.h"
#include "PartFrame.h"
#include "System.h"
#include "CREATE.h"
#include "DiagonalMatrix.h"
#include "EulerParameters.h"


using namespace MbD;

Part::Part() {
}

Part::Part(const char* str) : Item(str) {
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

void MbD::Part::setqXddot(FColDsptr x)
{
	partFrame->setqXddot(x);
}

FColDsptr MbD::Part::getqXddot()
{
	return partFrame->getqXddot();
}

void MbD::Part::setqEddot(FColDsptr x)
{
	partFrame->setqEddot(x);
}

FColDsptr MbD::Part::getqEddot()
{
	return partFrame->getqEddot();
}

void MbD::Part::qX(FColDsptr x)
{
	partFrame->qX = x;
}

FColDsptr MbD::Part::qX()
{
	return partFrame->qX;
}

void MbD::Part::qE(std::shared_ptr<EulerParameters<double>> x)
{
	partFrame->qE = x;
}

std::shared_ptr<EulerParameters<double>> MbD::Part::qE()
{
	return partFrame->qE;
}

void MbD::Part::qXdot(FColDsptr x)
{
	partFrame->qXdot = x;
}

FColDsptr MbD::Part::qXdot()
{
	return partFrame->qXdot;
}

void MbD::Part::omeOpO(FColDsptr x)
{
	partFrame->setomeOpO(x);
}

FColDsptr MbD::Part::omeOpO()
{
	assert(false);
	return FColDsptr();
}

void MbD::Part::qXddot(FColDsptr x)
{
	partFrame->qXddot = x;
}

FColDsptr MbD::Part::qXddot()
{
	return partFrame->qXddot;
}

void MbD::Part::qEddot(FColDsptr x)
{
	//ToDo: Should store EulerParametersDDot
	//ToDo: Need alpOpO too
	partFrame->qXddot = x;
}

FColDsptr MbD::Part::qEddot()
{
	return partFrame->qEddot;
}

void Part::setSystem(System& sys)
{
	//May be needed in the future
}

void Part::asFixed()
{
	partFrame->asFixed();
}

void MbD::Part::postInput()
{
	partFrame->postInput();
	Item::postInput();
}

void MbD::Part::calcPostDynCorrectorIteration()
{
	this->calcmE();
	this->calcmEdot();
	this->calcpTpE();
	this->calcppTpEpE();
	this->calcppTpEpEdot();
}

void MbD::Part::prePosIC()
{
	partFrame->prePosIC();
}

void MbD::Part::prePosKine()
{
	partFrame->prePosKine();
}

void MbD::Part::iqX(int eqnNo)
{
	partFrame->iqX = eqnNo;
}

void MbD::Part::iqE(int eqnNo)
{
	partFrame->iqE = eqnNo;

}

void MbD::Part::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	partFrame->fillEssenConstraints(essenConstraints);
}

void MbD::Part::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void MbD::Part::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	partFrame->fillConstraints(allConstraints);
}

void MbD::Part::fillqsu(FColDsptr col)
{
	partFrame->fillqsu(col);
}

void MbD::Part::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	//"Map wqX and wqE according to inertias. (0 to maximum inertia) map to (minw to maxw)"
	//"When the inertias are zero, they are set to a small number for positive definiteness."
	//"They are not set to zero because inertialess part may be underconstrained."
	//"Avoid having two kinds of singularities to confuse redundant constraint removal."
	//"Redundant constraint removal likes equal weights."
	//"wqE(4) = 0.0d is ok because there is always the euler parameter constraint."

	auto mMax = System::getInstance().maximumMass();
	auto aJiMax = System::getInstance().maximumMomentOfInertia();
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

void MbD::Part::fillqsulam(FColDsptr col)
{
	partFrame->fillqsulam(col);
}

void MbD::Part::fillqsudot(FColDsptr col)
{
	partFrame->fillqsudot(col);
}

void MbD::Part::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	//"wqXdot and wqEdot are set to their respective inertias."
	//"When the inertias are zero, they are set to a small number for positive definiteness."
	//"They are not set to zero because inertialess part may be underconstrained."
	//"wqEdot(4) = 0.0d is ok because there is always the euler parameter constraint."

		//| mMax aJiMax maxInertia minw maxw aJi wqXdot wqEdot |
	auto mMax = System::getInstance().maximumMass();
	auto aJiMax = System::getInstance().maximumMomentOfInertia();
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

void MbD::Part::useEquationNumbers()
{
	partFrame->useEquationNumbers();
}

void MbD::Part::setqsu(FColDsptr col)
{
	partFrame->setqsu(col);
}

void MbD::Part::setqsulam(FColDsptr col)
{
	partFrame->setqsulam(col);
}

void MbD::Part::setqsudotlam(FColDsptr col)
{
	partFrame->setqsudotlam(col);
}

void MbD::Part::postPosICIteration()
{
	partFrame->postPosICIteration();
}

void MbD::Part::fillPosICError(FColDsptr col)
{
	partFrame->fillPosICError(col);
}

void MbD::Part::fillPosICJacob(SpMatDsptr mat)
{
	partFrame->fillPosICJacob(mat);
}

void MbD::Part::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
	partFrame->removeRedundantConstraints(redundantEqnNos);
}

void MbD::Part::reactivateRedundantConstraints()
{
	partFrame->reactivateRedundantConstraints();
}

void MbD::Part::constraintsReport()
{
	partFrame->constraintsReport();
}

void MbD::Part::postPosIC()
{
	partFrame->postPosIC();
	this->calcmE();
}

void MbD::Part::outputStates()
{
	Item::outputStates();
	partFrame->outputStates();
}

void MbD::Part::preDyn()
{
	partFrame->preDyn();
}

void MbD::Part::storeDynState()
{
	partFrame->storeDynState();
}

void MbD::Part::fillPosKineError(FColDsptr col)
{
	partFrame->fillPosKineError(col);
}

void MbD::Part::fillPosKineJacob(SpMatDsptr mat)
{
	partFrame->fillPosKineJacob(mat);
}

void MbD::Part::preVelIC()
{
	partFrame->preVelIC();
}

void MbD::Part::postVelIC()
{
	partFrame->postVelIC();
	this->calcp();
	this->calcmEdot();
	this->calcpTpE();
	this->calcppTpEpE();
	this->calcppTpEpEdot();
}

void MbD::Part::fillVelICError(FColDsptr col)
{
	partFrame->fillVelICError(col);
}

void MbD::Part::fillVelICJacob(SpMatDsptr mat)
{
	partFrame->fillVelICJacob(mat);
}

void MbD::Part::preAccIC()
{
	partFrame->preAccIC();
	Item::preAccIC();
}

void MbD::Part::calcp()
{
	pX = mX->timesFullColumn(partFrame->qXdot);
	pE = mE->timesFullColumn(partFrame->qEdot);
}

void MbD::Part::calcpdot()
{
	pXdot = mX->timesFullColumn(partFrame->qXddot);
	pEdot = mEdot->timesFullColumn(partFrame->qEdot)->plusFullColumn(mE->timesFullColumn(partFrame->qEddot));
}

void MbD::Part::calcmEdot()
{
	auto aC = partFrame->aC();
	auto aCdot = partFrame->aCdot();
	auto a4J = aJ->times(4.0);
	auto term1 = aC->transposeTimesFullMatrix(a4J->timesFullMatrix(aCdot));
	auto term2 = term1->transpose();
	mEdot = term1->plusFullMatrix(term2);
}

void MbD::Part::calcpTpE()
{
	//"pTpE is a column vector."
	auto& qEdot = partFrame->qEdot;
	auto aC = partFrame->aC();
	auto pCpEtimesqEdot = EulerParameters<double>::pCpEtimesColumn(qEdot);
	pTpE = (pCpEtimesqEdot->transposeTimesFullColumn(aJ->timesFullColumn(aC->timesFullColumn(qEdot))))->times(4.0);
}

void MbD::Part::calcppTpEpE()
{
	auto& qEdot = partFrame->qEdot;
	auto pCpEtimesqEdot = EulerParameters<double>::pCpEtimesColumn(qEdot);
	auto a4J = aJ->times(4.0);
	ppTpEpE = pCpEtimesqEdot->transposeTimesFullMatrix(a4J->timesFullMatrix(pCpEtimesqEdot));
}

void MbD::Part::calcppTpEpEdot()
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

void MbD::Part::calcmE()
{
	auto aC = partFrame->aC();
	mE = aC->transposeTimesFullMatrix(aJ->timesFullMatrix(aC))->times(4.0);
}

void MbD::Part::fillAccICIterError(FColDsptr col)
{
	auto iqX = partFrame->iqX;
	auto iqE = partFrame->iqE;
	col->atiminusFullColumn(iqX, mX->timesFullColumn(partFrame->qXddot));
	col->atiminusFullColumn(iqE, mEdot->timesFullColumn(partFrame->qEdot));
	col->atiminusFullColumn(iqE, mE->timesFullColumn(partFrame->qEddot));
	col->atiplusFullColumn(iqE, pTpE);
	partFrame->fillAccICIterError(col);
}

void MbD::Part::fillAccICIterJacob(SpMatDsptr mat)
{
	auto iqX = partFrame->iqX;
	auto iqE = partFrame->iqE;
	mat->atijminusDiagonalMatrix(iqX, iqX, mX);
	mat->atijminusFullMatrix(iqE, iqE, mE);
	partFrame->fillAccICIterJacob(mat);
}

std::shared_ptr<EulerParametersDot<double>> MbD::Part::qEdot()
{
	return partFrame->qEdot;
}

void MbD::Part::setqsuddotlam(FColDsptr qsudotlam)
{
	partFrame->setqsuddotlam(qsudotlam);
}
