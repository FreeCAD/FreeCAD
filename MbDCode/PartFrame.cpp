#include<algorithm>

#include "Part.h"
#include "PartFrame.h"
#include "EulerConstraint.h"
#include "AbsConstraint.h"
#include "MarkerFrame.h"
#include "EulerParameters.h"
#include "EulerParametersDot.h"
#include "CREATE.h"

using namespace MbD;

PartFrame::PartFrame()
{
}
PartFrame::PartFrame(const char* str) : CartesianFrame(str)
{
}
void PartFrame::initialize()
{
	aGeu = CREATE<EulerConstraint>::With();
	aGeu->setOwner(this);
	aGabs = std::make_shared<std::vector<std::shared_ptr<AbsConstraint>>>();
	markerFrames = std::make_shared<std::vector<std::shared_ptr<MarkerFrame>>>();
}

void PartFrame::initializeLocally()
{
	std::for_each(markerFrames->begin(), markerFrames->end(), [](const auto& markerFrame) { markerFrame->initializeLocally(); });
	aGeu->initializeLocally();
	std::for_each(aGabs->begin(), aGabs->end(), [](const auto& aGab) { aGab->initializeLocally(); });
}

void PartFrame::initializeGlobally()
{
	std::for_each(markerFrames->begin(), markerFrames->end(), [](const auto& markerFrame) { markerFrame->initializeGlobally(); });
	aGeu->initializeGlobally();
	std::for_each(aGabs->begin(), aGabs->end(), [](const auto& aGab) { aGab->initializeGlobally(); });
}
void PartFrame::setqX(FColDsptr x) {
	qX->copyFrom(x);
}

FColDsptr PartFrame::getqX() {
	return qX;
}

void PartFrame::setqE(FColDsptr x) {
	qE->copyFrom(x);
}

FColDsptr PartFrame::getqE() {
	return qE;
}
void PartFrame::setqXdot(FColDsptr x) {
	//qXdot->copy(x);
}

FColDsptr PartFrame::getqXdot() {
	//return qXdot;
	return std::make_shared<FullColumn<double>>(3);
}

void PartFrame::setomeOpO(FColDsptr omeOpO) {
	//qEdot = EulerParametersDot<double>::FromqEOpAndOmegaOpO(qE, omeOpO);
}

FColDsptr PartFrame::getomeOpO() {
	return qE;
}

void PartFrame::setPart(Part* x) {
	part = x;
}

Part* PartFrame::getPart() {
	return part;
}

void PartFrame::addMarkerFrame(std::shared_ptr<MarkerFrame> markerFrame)
{
	markerFrame->setPartFrame(this);
	markerFrames->push_back(markerFrame);
}

EndFrmcptr PartFrame::endFrame(std::string name)
{
	auto match = std::find_if(markerFrames->begin(), markerFrames->end(), [&](auto& mkr) {return mkr->getName() == name; });
	return (*match)->endFrames->at(0);
}

void MbD::PartFrame::prePosIC()
{
//iqX = -1;
//iqE  = -1;
//super prePosIC.
//markerFrames do : [:mkr | mkr prePosIC] .
//aGeu prePosIC.
//aGabs do : [:con | con prePosIC]
}

FColDsptr MbD::PartFrame::rOpO()
{
	return qX;
}

FMatDsptr MbD::PartFrame::aAOp()
{
	return qE->aA;
}

FColFMatDsptr MbD::PartFrame::pAOppE()
{
	return qE->pApE;
}

void PartFrame::asFixed()
{
	for (size_t i = 0; i < 6; i++) {
		auto con = std::make_shared<AbsConstraint>(i);
		con->setOwner(this);
		aGabs->push_back(con);
	}
}

void MbD::PartFrame::postInput()
{
}

void MbD::PartFrame::calcPostDynCorrectorIteration()
{
	qE->calcABC();
	qE->calcpApE();
	//qEdot->calcAdotBdotCdot();
	//qEdot->calcpAdotpE();
}
