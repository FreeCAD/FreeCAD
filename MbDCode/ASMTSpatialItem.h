#pragma once

#include "ASMTItem.h"

namespace MbD {
	class ASMTSpatialItem : public ASMTItem
	{
		//
	public:
		void readPosition3D(std::vector<std::string>& lines);
		void readRotationMatrix(std::vector<std::string>& lines);
		void readVelocity3D(std::vector<std::string>& lines);
		void readOmega3D(std::vector<std::string>& lines);

		FColDsptr position3D, velocity3D, omega3D;
		FMatDsptr rotationMatrix;

	};
}

