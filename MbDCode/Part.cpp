#include "Part.h"
#include "PartFrame.h"
#include "System.h"
#include "CREATE.h"
#include "DiagonalMatrix.h"

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
	//mX: = m > 0
	//ifTrue: [StMDiagonalMatrix new:3 withAll : m]
	//ifFalse : [StMDiagonalMatrix new:3 withAll : 0.0d]
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
	//this->calcmE();
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
