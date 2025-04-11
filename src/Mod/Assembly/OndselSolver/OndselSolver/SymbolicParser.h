/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <stack>
#include <memory>
#include <sstream>

#include "Symbolic.h"
#include "ASMTItem.h"
#include "ASMTItemIJ.h"

namespace MbD {
	class SymbolicParser
	{
		//
	public:
		SymbolicParser();
		void initialize();
		void parseUserFunction(Symsptr userFunc);
		void parseString(const std::string& expr);
		bool commaExpression();
		bool plusTerm();
		bool minusTerm();
		bool plainTerm();
		bool term();
		bool plainFunction();
		bool timesFunction();
		bool divideByFunction();
		bool peekForTypeNoPush(const std::string& c);
		std::string scanToken();
		void xLetter();
		void xDigit();
		void xDoubleQuote();
		bool symfunction();
		bool expression();
		bool expressionInParentheses();
		bool constant();
		bool namedFunction();
		bool intrinsic();
		bool variable();
		bool raisedTo();
		bool expected(const std::string& msg);
		bool signedNumber();
		bool peekForTypevalue(const std::string& type, std::string symbol);
		void notify(const std::string& msg) const;
		void notifyat(const std::string& msg, int mrk) const;
		void combineStackTo(size_t pos) const;
		bool isNextLineTag(char c) const;

		ASMTItem* owner = nullptr;
		std::shared_ptr<std::map<std::string, Symsptr>> variables;
		std::shared_ptr<std::vector<ASMTItemIJ>> geoIJs;
		std::shared_ptr<Units> units;
		std::streampos mark = 0, prevEnd = 0;
		char hereChar = '\0';
		std::string token, tokenType;
		double tokenNum = -1.0e100;
		std::shared_ptr<std::istringstream> source;
		std::shared_ptr<std::stringstream> buffer;
		std::shared_ptr<std::stack<Symsptr>> stack;
	};
}

