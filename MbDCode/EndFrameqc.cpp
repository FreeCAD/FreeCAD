#include <memory>

#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "Variable.h"
#include "MarkerFrame.h"
#include "CREATE.h"
#include "EndFrameqct2.h"

using namespace MbD;

EndFrameqc::EndFrameqc() {
}

EndFrameqc::EndFrameqc(const char* str) : EndFramec(str) {
}

void EndFrameqc::initialize()
{
	prOeOpE = std::make_shared<FullMatrix<double>>(3, 4);
	pprOeOpEpE = std::make_shared<FullMatrix<FColDsptr>>(4, 4);
	pAOepE = std::make_shared<FullColumn<FMatDsptr>>(4);
	ppAOepEpE = std::make_shared<FullMatrix<FMatDsptr>>(4, 4);
}

void EndFrameqc::initializeGlobally()
{
	pprOeOpEpE = markerFrame->pprOmOpEpE;
	ppAOepEpE = markerFrame->ppAOmpEpE;
}

void EndFrameqc::initEndFrameqct()
{
	endFrameqct = CREATE<EndFrameqct>::With(this->name.data());
	endFrameqct->prOeOpE = prOeOpE;
	endFrameqct->pprOeOpEpE = pprOeOpEpE;
	endFrameqct->pAOepE = pAOepE;
	endFrameqct->ppAOepEpE = ppAOepEpE;
	endFrameqct->setMarkerFrame(markerFrame);
}

void MbD::EndFrameqc::initEndFrameqct2()
{
	endFrameqct = CREATE<EndFrameqct2>::With(this->name.data());
	endFrameqct->prOeOpE = prOeOpE;
	endFrameqct->pprOeOpEpE = pprOeOpEpE;
	endFrameqct->pAOepE = pAOepE;
	endFrameqct->ppAOepEpE = ppAOepEpE;
	endFrameqct->setMarkerFrame(markerFrame);
}

FMatFColDsptr EndFrameqc::ppAjOepEpE(int jj)
{
	auto answer = std::make_shared<FullMatrix<FColDsptr>>(4, 4);
	for (int i = 0; i < 4; i++) {
		auto& answeri = answer->at(i);
		auto& ppAOepEipE = ppAOepEpE->at(i);
		for (int j = i; j < 4; j++) {
			answeri->at(j) = ppAOepEipE->at(j)->column(jj);
		}
	}
	answer->symLowerWithUpper();
	return answer;
}

void EndFrameqc::calcPostDynCorrectorIteration()
{
	EndFramec::calcPostDynCorrectorIteration();
	prOeOpE = markerFrame->prOmOpE;
	pAOepE = markerFrame->pAOmpE;
}

FMatDsptr EndFrameqc::pAjOepET(int axis)
{
	auto answer = std::make_shared<FullMatrix<double>>(4, 3);
	for (int i = 0; i < 4; i++) {
		auto& answeri = answer->at(i);
		auto& pAOepEi = pAOepE->at(i);
		for (int j = 0; j < 3; j++) {
			auto& answerij = pAOepEi->at(j)->at(axis);
			answeri->at(j) = answerij;
		}
	}
	return answer;
}

FMatDsptr EndFrameqc::ppriOeOpEpE(int ii)
{
	auto answer = std::make_shared<FullMatrix<double>>(4, 4);
	for (int i = 0; i < 4; i++) {
		auto& answeri = answer->at(i);
		auto& pprOeOpEipE = pprOeOpEpE->at(i);
		for (int j = 0; j < 4; j++) {
			auto& answerij = pprOeOpEipE->at(j)->at(ii);
			answeri->at(j) = answerij;
		}
	}
	return answer;
}

int EndFrameqc::iqX()
{
	return markerFrame->iqX();
}

int EndFrameqc::iqE()
{
	return markerFrame->iqE();
}

FRowDsptr EndFrameqc::priOeOpE(int i)
{
	return prOeOpE->at(i);
}

FColDsptr EndFrameqc::qXdot()
{
	return markerFrame->qXdot();
}

std::shared_ptr<EulerParametersDot<double>> EndFrameqc::qEdot()
{
	return markerFrame->qEdot();
}

FColDsptr EndFrameqc::qXddot()
{
	return markerFrame->qXddot();
}

FColDsptr EndFrameqc::qEddot()
{
	return markerFrame->qEddot();
}

FColDsptr EndFrameqc::rpep()
{
	return markerFrame->rpmp;
}

FColFMatDsptr EndFrameqc::pAOppE()
{
	return markerFrame->pAOppE();
}

FMatDsptr EndFrameqc::aBOp()
{
	return markerFrame->aBOp();
}
