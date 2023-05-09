#include "EndFrameqct.h"

using namespace MbD;

EndFrameqct::EndFrameqct() {
	initialize();
}

EndFrameqct::EndFrameqct(const char* str) : EndFrameqc(str) {
	initialize();
}

void EndFrameqct::initialize()
{
	EndFrameqc::initialize();
	rmem = std::make_unique<FullColumn<double>>(3);	
	prmempt = std::make_unique<FullColumn<double>>(3);	
	pprmemptpt = std::make_unique<FullColumn<double>>(3);	
	aAme = std::make_unique<FullMatrix<double>>(3, 3);
	pAmept = std::make_unique<FullMatrix<double>>(3, 3);
	ppAmeptpt = std::make_unique<FullMatrix<double>>(3, 3);
	pprOeOpEpt = std::make_unique<FullMatrix<double>>(3, 4);
	pprOeOptpt = std::make_unique<FullColumn<double>>(3);	
	ppAOepEpt = std::make_unique<FullColumn<FullMatrix<double>>>(4);
	ppAOeptpt = std::make_unique<FullMatrix<double>>(3, 3);
}

void EndFrameqct::initializeLocally()
{
	//rmemBlks == nil
	//	ifTrue :
	//[rmem zeroSelf.
	//	prmempt zeroSelf.
	//	pprmemptpt zeroSelf] .
	//	phiThePsiBlks == nil
	//	ifTrue :
	//[aAme identity.
	//	pAmept zeroSelf.
	//	ppAmeptpt zeroSelf]
}

void EndFrameqct::initializeGlobally()
{

}
