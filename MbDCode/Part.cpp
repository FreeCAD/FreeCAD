#include "Part.h"
#include "PartFrame.h"
#include "CREATE.h"

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
}

void MbD::Part::calcPostDynCorrectorIteration()
{
}

void MbD::Part::prePosIC()
{
	partFrame->prePosIC();
}

void MbD::Part::iqX(int eqnNo)
{
	partFrame->iqX = eqnNo;
}

void MbD::Part::iqE(int eqnNo)
{
	partFrame->iqE = eqnNo;

}
