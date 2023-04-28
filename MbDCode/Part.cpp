#include "Part.h"
#include "PartFrame.h"
#include "FullColumn.h"

using namespace MbD;

Part::Part() {
	partFrame = std::make_shared<PartFrame>();
}

void Part::setqX(FullColumn<double>* x) {
	partFrame.get()->setqX(x);
}

FullColumn<double>* Part::getqX() {
	return partFrame.get()->getqX();
}

void Part::setqE(FullColumn<double>* x) {
	partFrame.get()->setqE(x);
}

FullColumn<double>* Part::getqE() {
	return partFrame.get()->getqE();
}

void Part::setSystem(System& sys)
{
	//May be needed in the future
}
