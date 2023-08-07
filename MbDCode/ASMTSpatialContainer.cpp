#include "ASMTSpatialContainer.h"
#include "Units.h"
#include "Part.h"
#include "System.h"

using namespace MbD;

void MbD::ASMTSpatialContainer::initialize()
{
	xs = std::make_shared<FullRow<double>>();
	ys = std::make_shared<FullRow<double>>();
	zs = std::make_shared<FullRow<double>>();
	bryxs = std::make_shared<FullRow<double>>();
	bryys = std::make_shared<FullRow<double>>();
	bryzs = std::make_shared<FullRow<double>>();
	vxs = std::make_shared<FullRow<double>>();
	vys = std::make_shared<FullRow<double>>();
	vzs = std::make_shared<FullRow<double>>();
	omexs = std::make_shared<FullRow<double>>();
	omeys = std::make_shared<FullRow<double>>();
	omezs = std::make_shared<FullRow<double>>();
	axs = std::make_shared<FullRow<double>>();
	ays = std::make_shared<FullRow<double>>();
	azs = std::make_shared<FullRow<double>>();
	alpxs = std::make_shared<FullRow<double>>();
	alpys = std::make_shared<FullRow<double>>();
	alpzs = std::make_shared<FullRow<double>>();
}

void MbD::ASMTSpatialContainer::readRefPoints(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefPoints") != std::string::npos);
	lines.erase(lines.begin());
	refPoints = std::make_shared<std::vector<std::shared_ptr<ASMTRefPoint>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("RefCurves") != std::string::npos;
		});
	std::vector<std::string> refPointsLines(lines.begin(), it);
	while (!refPointsLines.empty()) {
		readRefPoint(refPointsLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTSpatialContainer::readRefPoint(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefPoint") != std::string::npos);
	lines.erase(lines.begin());
	auto refPoint = CREATE<ASMTRefPoint>::With();
	refPoint->parseASMT(lines);
	refPoints->push_back(refPoint);
	refPoint->owner = this;
}

void MbD::ASMTSpatialContainer::readRefCurves(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefCurves") != std::string::npos);
	lines.erase(lines.begin());
	refCurves = std::make_shared<std::vector<std::shared_ptr<ASMTRefCurve>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("RefSurfaces") != std::string::npos;
		});
	std::vector<std::string> refCurvesLines(lines.begin(), it);
	while (!refCurvesLines.empty()) {
		readRefCurve(refCurvesLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTSpatialContainer::readRefCurve(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::ASMTSpatialContainer::readRefSurfaces(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefSurfaces") != std::string::npos);
	lines.erase(lines.begin());
	refSurfaces = std::make_shared<std::vector<std::shared_ptr<ASMTRefSurface>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("Part") != std::string::npos;
		});
	std::vector<std::string> refSurfacesLines(lines.begin(), it);
	while (!refSurfacesLines.empty()) {
		readRefSurface(refSurfacesLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTSpatialContainer::readRefSurface(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::ASMTSpatialContainer::readXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "X", inxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Y", inys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Z", inzs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readBryantxs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Bryantx", inbryxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readBryantys(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Bryanty", inbryys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readBryantzs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Bryantz", inbryzs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readVXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "VX", invxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readVYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "VY", invys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readVZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "VZ", invzs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readOmegaXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "OmegaX", inomexs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readOmegaYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "OmegaY", inomeys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readOmegaZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "OmegaZ", inomezs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AX", inaxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AY", inays);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AZ", inazs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAlphaXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AlphaX", inalpxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAlphaYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AlphaY", inalpys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAlphaZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AlphaZ", inalpzs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	auto mbdPart = CREATE<Part>::With();
	mbdObject = mbdPart;
	mbdPart->name = fullName("");
	mbdPart->m = principalMassMarker->mass / mbdUnits->mass;
	mbdPart->aJ = principalMassMarker->momentOfInertias->times(1.0 / mbdUnits->aJ);
	mbdPart->qX(rOcmO()->times(1.0 / mbdUnits->length));
	mbdPart->qE(qEp());
	mbdPart->qXdot(vOcmO()->times(1.0 / mbdUnits->velocity));
	mbdPart->omeOpO(omeOpO()->times(1.0 / mbdUnits->omega));
	mbdPart->qXddot(std::make_shared<FullColumn<double>>(3, 0));
	mbdPart->qEddot(std::make_shared<FullColumn<double>>(4, 0));
	mbdSys->addPart(mbdPart);
	for (auto& refPoint : *refPoints) {
		refPoint->createMbD(mbdSys, mbdUnits);
	}
	for (auto& refCurve : *refCurves) {
		refCurve->createMbD(mbdSys, mbdUnits);
	}
	for (auto& refSurface : *refSurfaces) {
		refSurface->createMbD(mbdSys, mbdUnits);
	}
}

FColDsptr MbD::ASMTSpatialContainer::rOcmO()
{
	auto& rOPO = position3D;
	auto& aAOP = rotationMatrix;
	auto& rPcmP = principalMassMarker->position3D;
	auto rOcmO = rOPO->plusFullColumn(aAOP->timesFullColumn(rPcmP));
	return rOcmO;
}

std::shared_ptr<EulerParameters<double>> MbD::ASMTSpatialContainer::qEp()
{
	auto& aAOP = rotationMatrix;
	auto& aAPcm = principalMassMarker->rotationMatrix;
	auto aAOcm = aAOP->timesFullMatrix(aAPcm);
	return aAOcm->asEulerParameters();
}

FColDsptr MbD::ASMTSpatialContainer::vOcmO()
{
	assert(false);
	return FColDsptr();
}

FColDsptr MbD::ASMTSpatialContainer::omeOpO()
{
	assert(false);
	return FColDsptr();
}

std::shared_ptr<ASMTSpatialContainer> MbD::ASMTSpatialContainer::part()
{
	return std::make_shared<ASMTSpatialContainer>(*this);
}

void MbD::ASMTSpatialContainer::updateFromMbD()
{
	auto mbdUnts = mbdUnits();
	auto mbdPart = std::static_pointer_cast<Part>(mbdObject);
	auto rOcmO = mbdPart->qX()->times(mbdUnts->length);
	auto aAOp = mbdPart->aAOp();
	auto vOcmO = mbdPart->qXdot()->times(mbdUnts->velocity);
	auto omeOPO = mbdPart->omeOpO()->times(mbdUnts->omega);
	auto aOcmO = mbdPart->qXddot()->times(mbdUnts->acceleration);
	auto alpOPO = mbdPart->alpOpO()->times(mbdUnts->alpha);
	auto& rPcmP = principalMassMarker->position3D;
	auto& aAPp = principalMassMarker->rotationMatrix;
	auto aAOP = aAOp->timesTransposeFullMatrix(aAPp);
	auto rPcmO = aAOP->timesFullColumn(rPcmP);
	auto rOPO = rOcmO->minusFullColumn(rPcmO);
	auto vOPO = vOcmO->minusFullColumn(omeOPO->cross(rPcmO));
	auto aOPO = aOcmO->minusFullColumn(alpOPO->cross(rPcmO))->minusFullColumn(omeOPO->cross(omeOPO->cross(rPcmO)));
	xs->push_back(rOPO->at(0));
	ys->push_back(rOPO->at(1));
	zs->push_back(rOPO->at(2));
	auto bryantAngles = aAOP->bryantAngles();
	bryxs->push_back(bryantAngles->at(0));
	bryys->push_back(bryantAngles->at(1));
	bryzs->push_back(bryantAngles->at(2));
	vxs->push_back(vOPO->at(0));
	vys->push_back(vOPO->at(1));
	vzs->push_back(vOPO->at(2));
	omexs->push_back(omeOPO->at(0));
	omeys->push_back(omeOPO->at(1));
	omezs->push_back(omeOPO->at(2));
	axs->push_back(aOPO->at(0));
	ays->push_back(aOPO->at(1));
	azs->push_back(aOPO->at(2));
	alpxs->push_back(alpOPO->at(0));
	alpys->push_back(alpOPO->at(1));
	alpzs->push_back(alpOPO->at(2));
}

void MbD::ASMTSpatialContainer::compareResults(AnalysisType type)
{
	auto mbdUnts = mbdUnits();
	auto factor = 1.0e-6;
	auto lengthTol = mbdUnts->length * factor;
	auto angleTol = mbdUnts->angle * factor;
	auto velocityTol = mbdUnts->velocity * factor;
	auto omegaTol = mbdUnts->omega * factor;
	auto accelerationTol = mbdUnts->acceleration * factor;
	auto alphaTol = mbdUnts->alpha * factor;
	auto i = xs->size() - 1;
	//Pos
	if (!Numeric::equaltol(xs->at(i), inxs->at(i), lengthTol)) {
		std::cout << i << " xs " << xs->at(i) << ", " << i<< lengthTol << std::endl;
		std::cout << i << " xs " << xs->at(i) << ", " << inxs->at(i) << ", " << lengthTol << std::endl;
	}
	if (!Numeric::equaltol(ys->at(i), inys->at(i), lengthTol)) {
		std::cout << i << " ys " << ys->at(i) << ", " << inys->at(i) << ", " << lengthTol << std::endl;
	}
	if (!Numeric::equaltol(zs->at(i), inzs->at(i), lengthTol)) {
		std::cout << i << " zs " << zs->at(i) << ", " << inzs->at(i) << ", " << lengthTol << std::endl;
	}
	if (!Numeric::equaltol(bryxs->at(i), inbryxs->at(i), angleTol)) {
		std::cout << i << " bryxs " << bryxs->at(i) << ", " << inbryxs->at(i) << ", " << angleTol << std::endl;
	}
	if (!Numeric::equaltol(bryys->at(i), inbryys->at(i), angleTol)) {
		std::cout << i << " bryys " << bryys->at(i) << ", " << inbryys->at(i) << ", " << angleTol << std::endl;
	}
	if (!Numeric::equaltol(bryzs->at(i), inbryzs->at(i), angleTol)) {
		std::cout << i << " bryzs " << bryzs->at(i) << ", " << inbryzs->at(i) << ", " << angleTol << std::endl;
	}
	//Vel
	if (!Numeric::equaltol(vxs->at(i), invxs->at(i), velocityTol)) {
		std::cout << i << " vxs " << vxs->at(i) << ", " << invxs->at(i) << ", " << velocityTol << std::endl;
	}
	if (!Numeric::equaltol(vys->at(i), invys->at(i), velocityTol)) {
		std::cout << i << " vys " << vys->at(i) << ", " << invys->at(i) << ", " << velocityTol << std::endl;
	}
	if (!Numeric::equaltol(vzs->at(i), invzs->at(i), velocityTol)) {
		std::cout << i << " vzs " << vzs->at(i) << ", " << invzs->at(i) << ", " << velocityTol << std::endl;
	}
	if (!Numeric::equaltol(omexs->at(i), inomexs->at(i), omegaTol)) {
		std::cout << i << " omexs " << omexs->at(i) << ", " << inomexs->at(i) << ", " << omegaTol << std::endl;
	}
	if (!Numeric::equaltol(omeys->at(i), inomeys->at(i), omegaTol)) {
		std::cout << i << " omeys " << omeys->at(i) << ", " << inomeys->at(i) << ", " << omegaTol << std::endl;
	}
	if (!Numeric::equaltol(omezs->at(i), inomezs->at(i), omegaTol)) {
		std::cout << i << " omezs " << omezs->at(i) << ", " << inomezs->at(i) << ", " << omegaTol << std::endl;
	}
	//Acc
	if (!Numeric::equaltol(axs->at(i), inaxs->at(i), accelerationTol)) {
		std::cout << i << " axs " << axs->at(i) << ", " << inaxs->at(i) << ", " << accelerationTol << std::endl;
	}
	if (!Numeric::equaltol(ays->at(i), inays->at(i), accelerationTol)) {
		std::cout << i << " ays " << ays->at(i) << ", " << inays->at(i) << ", " << accelerationTol << std::endl;
	}
	if (!Numeric::equaltol(azs->at(i), inazs->at(i), accelerationTol)) {
		std::cout << i << " azs " << azs->at(i) << ", " << inazs->at(i) << ", " << accelerationTol << std::endl;
	}
	if (!Numeric::equaltol(alpxs->at(i), inalpxs->at(i), alphaTol)) {
		std::cout << i << " alpxs " << alpxs->at(i) << ", " << inalpxs->at(i) << ", " << alphaTol << std::endl;
	}
	if (!Numeric::equaltol(alpys->at(i), inalpys->at(i), alphaTol)) {
		std::cout << i << " alpys " << alpys->at(i) << ", " << inalpys->at(i) << ", " << alphaTol << std::endl;
	}
	if (!Numeric::equaltol(alpzs->at(i), inalpzs->at(i), alphaTol)) {
		std::cout << i << " alpzs " << alpzs->at(i) << ", " << inalpzs->at(i) << ", " << alphaTol << std::endl;
	}
}
