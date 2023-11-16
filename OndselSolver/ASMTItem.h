/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once
#include "CREATE.h"
#include "APIExport.h"

namespace MbD {
	class ASMTAssembly;
	class Units;
	class ASMTSpatialContainer;
	class ASMTPart;

	class EXPORT ASMTItem
	{
		//
	public:
		virtual ~ASMTItem() {}
		virtual ASMTAssembly* root();
		virtual ASMTSpatialContainer* partOrAssembly();
		virtual ASMTPart* part();

		virtual void initialize();
		void noop();
		virtual std::string classname();
		void setName(std::string str);
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
		virtual void outputResults(AnalysisType type);
		std::shared_ptr<Units> mbdUnits();
		std::shared_ptr<Constant> sptrConstant(double value);
		virtual void storeOnLevel(std::ofstream& os, int level);
		virtual void storeOnLevelTabs(std::ofstream& os, int level);
		virtual void storeOnLevelString(std::ofstream& os, int level, std::string str);
		virtual void storeOnLevelDouble(std::ofstream& os, int level, double value);
		virtual void storeOnLevelInt(std::ofstream& os, int level, int i);
		virtual void storeOnLevelBool(std::ofstream& os, int level, bool value);
		//template<typename T>
		//void storeOnLevelArray(std::ofstream& os, int level, std::vector<T> array);
		void storeOnLevelArray(std::ofstream& os, int level, std::vector<double> array);
		void storeOnLevelName(std::ofstream& os, int level);
		virtual void storeOnTimeSeries(std::ofstream& os);
		void logString(std::string& str);
		void logString(const char* chars);

		std::string name;
		ASMTItem* owner = nullptr;
		std::shared_ptr<Item> mbdObject;
	};
	//template<typename T>
	//inline void ASMTItem::storeOnLevelArray(std::ofstream& os, int level, std::vector<T> array)
	//{
	//	storeOnLevelTabs(os, level);
	//	for (int i = 0; i < array.size(); i++)
	//	{
	//		os << array[i] << '\t';
	//	}
	//	//os << std::endl;
	//}
}

