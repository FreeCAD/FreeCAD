#include <memory>

#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "Variable.h"
#include "MarkerFrame.h"

using namespace MbD;

std::shared_ptr<EndFrameqc> MbD::EndFrameqc::Create(const char* name)
{
	auto item = std::make_shared<EndFrameqc>(name);
	item->initialize();
	return item;
}

EndFrameqc::EndFrameqc() {
}

EndFrameqc::EndFrameqc(const char* str) : EndFramec(str) {
}

void EndFrameqc::initialize()
{
	prOeOpE = std::make_shared<FullMatrix<double>>(3, 4);
	pprOeOpEpE = std::make_shared<FullMatrix<std::shared_ptr<FullColumn<double>>>>(4, 4);
	pAOepE = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
	ppAOepEpE = std::make_shared<FullMatrix<std::shared_ptr<FullMatrix<double>>>>(4, 4);
}

void EndFrameqc::initializeLocally()
{
	if (endFrameqct) {
		endFrameqct->initializeLocally();
	}
}

void EndFrameqc::initializeGlobally()
{
	if (endFrameqct) {
		endFrameqct->initializeGlobally();
	}
	else {
		pprOeOpEpE = markerFrame->pprOmOpEpE;
		ppAOepEpE = markerFrame->ppAOmpEpE;
	}
}

void EndFrameqc::initEndFrameqct()
{
	endFrameqct = EndFrameqct::Create(this->getName().data());
}

void EndFrameqc::setrmemBlks(std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> xyzBlks)
{
	std::static_pointer_cast<EndFrameqct>(endFrameqct)->rmemBlks = xyzBlks;
}

void EndFrameqc::setphiThePsiBlks(std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> xyzRotBlks)
{
	std::static_pointer_cast<EndFrameqct>(endFrameqct)->phiThePsiBlks = xyzRotBlks;
}

FMatFColDsptr MbD::EndFrameqc::ppAjOepEpE(int jj)
{
	auto answer = std::make_shared<FullMatrix<std::shared_ptr<FullColumn<double>>>>(4, 4);
	for (int i = 0; i < 4; i++) {
		auto answeri = answer->at(i);
		auto ppAOepEipE = ppAOepEpE->at(i);
		for (int j = i; j < 4; j++) {
			answeri->at(j) = ppAOepEipE->at(j)->column(jj);
		}
	}
	answer->symLowerWithUpper();
	return answer;
}

void MbD::EndFrameqc::calcPostDynCorrectorIteration()
{
}
