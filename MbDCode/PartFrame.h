#pragma once
#include <memory>
#include <vector>

#include "CartesianFrame.h"
#include "EndFramec.h"
#include "FullColumn.h"
#include "EulerParameters.h"
#include "EulerParametersDot.h"
#include "CREATE.h"

namespace MbD {
	class Part;
	class MarkerFrame;
	class EulerConstraint;
	class AbsConstraint;
	//class EulerParameters;
	//class EulerParametersDot;

	class PartFrame : public CartesianFrame
	{
		//ToDo: part iqX iqE qX qE qXdot qEdot qXddot qEddot aGeu aGabs markerFrames 
	public:
		PartFrame();
		PartFrame(const char* str);
		void initialize();
		void initializeLocally() override;
		void initializeGlobally() override;
		void asFixed();
		void postInput() override;
		void calcPostDynCorrectorIteration() override;

		void setqX(FColDsptr x);
		FColDsptr getqX();
		void setqE(FColDsptr x);
		FColDsptr getqE();
		void setqXdot(FColDsptr x);
		FColDsptr getqXdot();
		void setomeOpO(FColDsptr x);
		FColDsptr getomeOpO();
		void setPart(Part* x);
		Part* getPart();
		void addMarkerFrame(std::shared_ptr<MarkerFrame> x);
		EndFrmcptr endFrame(std::string name);

		void prePosIC() override;
		FColDsptr rOpO();
		FMatDsptr aAOp();
		FColFMatDsptr pAOppE();

		Part* part = nullptr;
		int iqX = -1;
		int iqE = -1;	//Position index of frame variables qX and qE in system list of variables
		FColDsptr qX = std::make_shared<FullColumn<double>>(3);
		std::shared_ptr<EulerParameters<double>> qE = CREATE<EulerParameters<double>>::With(4);
		//FColDsptr qXdot = std::make_shared<FullColumn<double>>(3);
		//std::shared_ptr<EulerParametersDot<double>> qEdot = std::make_shared<EulerParametersDot<double>>(4);
		//FColDsptr qXddot = std::make_shared<FullColumn<double>>(3);
		//FColDsptr qEddot = std::make_shared<FullColumn<double>>(4);
		std::shared_ptr<EulerConstraint> aGeu;
		std::shared_ptr<std::vector<std::shared_ptr<AbsConstraint>>> aGabs;
		std::shared_ptr<std::vector<std::shared_ptr<MarkerFrame>>> markerFrames;
	};
}

