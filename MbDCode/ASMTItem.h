#pragma once
#include "CREATE.h"

namespace MbD {
	class ASMTAssembly;
	class Units;
	class ASMTSpatialContainer;

	class ASMTItem
	{
		//
	public:
		virtual ASMTAssembly* root();
		virtual std::shared_ptr<ASMTSpatialContainer> part();

		virtual void initialize();
		virtual void parseASMT(std::vector<std::string>& lines);
		FRowDsptr readRowOfDoubles(std::string& line);
		FColDsptr readColumnOfDoubles(std::string& line);
		double readDouble(std::string& line);
		int readInt(std::string& line);
		bool readBool(std::string& line);
		std::string readString(std::string& line);
		void readName(std::vector<std::string>& lines);
		virtual std::string fullName(std::string partialName);
		void readDoublesInto(std::string& str, std::string label, FRowDsptr& row);
		virtual void deleteMbD();
		virtual void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits);
		virtual void updateFromMbD();
		virtual void compareResults(AnalysisType type);
		std::shared_ptr<Units> mbdUnits();
		std::shared_ptr<Constant> sptrConstant(double value);

		std::string name;
		ASMTItem* owner;
		std::shared_ptr<Item> mbdObject;
	};
}

