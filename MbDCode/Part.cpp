#include "Part.h"
#include "PartFrame.h"
#include "FullColumn.h"

using namespace MbD;

Part::Part() {
	initialize();
}

Part::Part(const char* str) : Item(str) {
	initialize();
}

void Part::initialize()
{
	partFrame = std::make_shared<PartFrame>();
	partFrame->setPart(this);
	pTpE = std::make_shared<FullColumn<double>>(4);
	ppTpEpE = std::make_shared<FullMatrix<double>>(4, 4);
	ppTpEpEdot = std::make_shared<FullMatrix<double>>(4, 4);
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

void Part::setSystem(System& sys)
{
	//May be needed in the future
}

void MbD::Part::asFixed()
{
	partFrame->asFixed();
}

void Part::initializeLocally()
{
	partFrame->initializeLocally();
}

void Part::initializeGlobally()
{
}
