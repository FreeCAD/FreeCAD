#pragma once
#include "CREATE.h"

namespace MbD {
	class ASMTItem
	{
		//
	public:
		virtual void initialize();
		virtual void parseASMT(std::vector<std::string>& lines);
		FRowDsptr readRowOfDoubles(std::string& line);
		FColDsptr readColumnOfDoubles(std::string& line);
		double readDouble(std::string& line);
		int readInt(std::string& line);
		bool readBool(std::string& line);
		std::string readString(std::string& line);
		void readName(std::vector<std::string>& lines);
		std::string fullName(std::string partialName);
		void readDoublesInto(std::string& str, std::string label, FRowDsptr& row);

		std::string name;
		ASMTItem* owner;
	};
}

