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
}

void Part::setqX(FullColDptr x) {
	partFrame->setqX(x);
}

FullColDptr Part::getqX() {
	return partFrame->getqX();
}

void Part::setqE(FullColDptr x) {
	partFrame->setqE(x);
}

FullColDptr Part::getqE() {
	return partFrame->getqE();
}

void Part::setSystem(System& sys)
{
	//May be needed in the future
}

void Part::initializeLocally()
{
}

void Part::initializeGlobally()
{
}
