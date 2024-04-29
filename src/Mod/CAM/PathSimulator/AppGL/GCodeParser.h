#ifndef __csgcodeparser_h__
#define __csgcodeparser_h__
#include "MillMotion.h"
#include <vector>

namespace MillSim
{
	struct GCToken
	{
		char letter;
		float fval;
		int ival;
	};

	class GCodeParser
	{
	public:
		GCodeParser() {}
		virtual ~GCodeParser();
		bool Parse(const char* filename);
		bool AddLine(const char* ptr);

	public:
		std::vector<MillMotion> Operations;
		MillMotion lastState = { eNop };

	protected:
		const char* GetNextToken(const char* ptr, GCToken* token);
		bool IsValidTok(char tok);
		const char* ParseFloat(const char* ptr, float* retFloat);
		bool ParseLine(const char* ptr);
		int lastTool = -1;
	};
}
#endif
