#include "EndFrameqct.h"
#include "System.h"

using namespace MbD;

EndFrameqct::EndFrameqct() {
	initialize();
}

EndFrameqct::EndFrameqct(const char* str) : EndFrameqc(str) {
	initialize();
}

void EndFrameqct::initialize()
{
	rmem = std::make_shared<FullColumn<double>>(3);	
	prmempt = std::make_shared<FullColumn<double>>(3);	
	pprmemptpt = std::make_shared<FullColumn<double>>(3);	
	aAme = std::make_shared<FullMatrix<double>>(3, 3);
	pAmept = std::make_shared<FullMatrix<double>>(3, 3);
	ppAmeptpt = std::make_shared<FullMatrix<double>>(3, 3);
	pprOeOpEpt = std::make_shared<FullMatrix<double>>(3, 4);
	pprOeOptpt = std::make_shared<FullColumn<double>>(3);	
	ppAOepEpt = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
	ppAOeptpt = std::make_shared<FullMatrix<double>>(3, 3);
}

void EndFrameqct::initializeLocally()
{
	if (!rmemBlks) {
		rmem->zeroSelf();
		prmempt->zeroSelf();
		pprmemptpt->zeroSelf();
	}
	if (!phiThePsiBlks) {
		aAme->identity();
		pAmept->zeroSelf();
		ppAmeptpt->zeroSelf();
	}
}

void EndFrameqct::initializeGlobally()
{
	if (!rmemBlks) {
		initprmemptBlks();
		initpprmemptptBlks();
	}
	if (!phiThePsiBlks) {
		initpPhiThePsiptBlks();
		initppPhiThePsiptptBlks();
	}
}

void MbD::EndFrameqct::initprmemptBlks()
{
	auto time = System::getInstance().time;
	prmemptBlks = std::make_shared< FullColumn<std::shared_ptr<Symbolic>>>(3);
	for (int i = 0; i < 3; i++) {
		auto disp = rmemBlks->at(i);
		auto vel = (disp->differentiateWRT(time))->simplified();
		prmemptBlks->at(i) = vel;
	}
}

void MbD::EndFrameqct::initpprmemptptBlks()
{
}

void MbD::EndFrameqct::initpPhiThePsiptBlks()
{
}

void MbD::EndFrameqct::initppPhiThePsiptptBlks()
{
}
