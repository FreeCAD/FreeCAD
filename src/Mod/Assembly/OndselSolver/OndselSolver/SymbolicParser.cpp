/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"
#include "Sum.h"
#include "Product.h"
#include "CREATE.h"
#include "Power.h"
#include "Abs.h"
#include "ArcTan.h"
#include "Sine.h"
#include "Cosine.h"
#include "Negative.h"
#include "Reciprocal.h"
#include "GeneralSpline.h"
#include "ArcSine.h"
#include "Integral.h"
#include "RampStepFunction.h"
#include "Arguments.h"
#include "Functions.h"
#include "Transitions.h"

MbD::SymbolicParser::SymbolicParser()
{
	variables = std::make_shared<std::map<std::string, Symsptr>>();
	stack = std::make_shared<std::stack<Symsptr>>();
	buffer = std::make_shared<std::stringstream>();
}

void MbD::SymbolicParser::initialize()
{
	variables = std::make_shared<std::map<std::string, Symsptr>>();
	stack = std::make_shared<std::stack<Symsptr>>();
	buffer = std::make_shared<std::stringstream>();

}

void MbD::SymbolicParser::parseUserFunction(Symsptr userFunc)
{
	auto usrFunc = std::static_pointer_cast<BasicUserFunction>(userFunc);
	units = usrFunc->units;
	this->parseString(usrFunc->funcText);
	Symsptr func = stack->top();
	stack->pop();
	stack->push(Symbolic::times(func, std::make_shared<Constant>(usrFunc->myUnit)));
}

void MbD::SymbolicParser::parseString(const std::string& expr)
{
	buffer->clear();
	while (!stack->empty()) {
		stack->pop();
	}
	source = std::make_shared<std::istringstream>(expr);
	hereChar = source->get();
	prevEnd = -1;
	scanToken();
	expression();
	if (tokenType != "end") expected("Nothing more");
	if (stack->size() != 1) notify("Stack size error, compiler bug!");
}

bool MbD::SymbolicParser::commaExpression()
{
	if (peekForTypeNoPush(",")) {
		if (expression()) return true;
		expected("expression");
	}
	return false;
}

bool MbD::SymbolicParser::plusTerm()
{
	if (peekForTypeNoPush("+")) {
		if (plainTerm()) return true;
		expected("plainTerm");
	}
	return false;
}

bool MbD::SymbolicParser::minusTerm()
{
	if (peekForTypeNoPush("-")) {
		if (term()) {
			Symsptr trm = stack->top();
			stack->pop();
			auto negativeTrm = std::make_shared<Negative>(trm);
			Symsptr sum = stack->top();
			sum->addTerm(negativeTrm);
			return true;
		}
		expected("term");
	}
	return false;
}

bool MbD::SymbolicParser::plainTerm()
{
	if (term()) {
		Symsptr trm = stack->top();
		stack->pop();
		Symsptr sum = stack->top();
		sum->addTerm(trm);
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::term()
{
	auto product = std::make_shared<Product>();
	stack->push(product);
	if (plainFunction()) {
		while (timesFunction() || divideByFunction()) {}
		Symsptr term = stack->top();
		if (term->isProduct()) {
			if (term->isOne()) {
				stack->pop();
			}
			else if (term->getTerms()->size() == 1) {
				stack->pop();
				stack->push(term->getTerms()->front());
			}
		}
		else {
			notify("SymbolicParser error");
		}
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::plainFunction()
{
	if (symfunction()) {
		Symsptr trm = stack->top();
		stack->pop();
		Symsptr product = stack->top();
		product->addTerm(trm);
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::timesFunction()
{
	if (peekForTypeNoPush("*")) {
		if (plainFunction()) return true;
		expected("plainFunction");
	}
	return false;
}

bool MbD::SymbolicParser::divideByFunction()
{
	if (peekForTypeNoPush("/")) {
		if (symfunction()) {
			Symsptr trm = stack->top();
			stack->pop();
			auto reciprocalTrm = std::make_shared<Reciprocal>(trm);
			Symsptr product = stack->top();
			product->addTerm(reciprocalTrm);
			return true;
		}
		expected("function");
	}
	return false;
}

bool MbD::SymbolicParser::peekForTypeNoPush(const std::string& c)
{
	//"Test to see if tokenType matches aType. If so, advance to the next token, leaving the stack unchanged"

	if (tokenType == c) {
		scanToken();
		return true;
	}
	return false;
}

std::string MbD::SymbolicParser::scanToken()
{
        prevEnd = source->tellg() - std::streamoff(1);
	while (std::isspace(hereChar) || isNextLineTag(hereChar)) {
		hereChar = source->get();
	}
	if (hereChar == EOF) {
		mark = prevEnd + std::streamoff(1);;
		tokenType = "end";
		return token = "";
	}
	mark = source->tellg();
	if (std::isalpha(hereChar)) {
		xLetter();
	}
	else if (std::isdigit(hereChar)) {
		xDigit();
	}
	else if (hereChar == '\"') {
		xDoubleQuote();
	}
	else {
		token = std::string(1, hereChar);
		tokenType = token;
		hereChar = source->get();
	}
	return token;
}

void MbD::SymbolicParser::xLetter()
{
	buffer->str("");
	buffer->clear();
	*buffer << hereChar;
	while (true) {
		hereChar = source->get();
		if (hereChar == EOF) break;
		if (!std::isalnum(hereChar) && hereChar != '_') break;
		*buffer << hereChar;
	}
	tokenType = "word";
	token = buffer->str();
}

void MbD::SymbolicParser::xDigit()
{
	tokenType = "number";
	if (hereChar != EOF) {
		auto pos = source->tellg();
		std::streamoff offset = -1;
		pos += offset;
		source->seekg(pos);
	}
	double mantissa = 0.0;
	int exponent = 0;
	*source >> mantissa;
	hereChar = source->peek();
	if ((hereChar == 'd') || (hereChar == 'e')) {
		hereChar = source->get();
		char peekChar = source->peek();
		if (std::isdigit(peekChar) || (peekChar == '+') || (peekChar == '-')) {
			*source >> exponent;
		}
	}
	token = "";
	tokenNum = mantissa * std::pow(10.0, exponent);
	hereChar = source->get();
}

void MbD::SymbolicParser::xDoubleQuote()
{
	tokenType = "comment";
	if (hereChar != EOF) {
		auto pos = source->tellg();
		std::streamoff offset = -1;
		pos += offset;
		source->seekg(pos);
	}
	*source >> std::quoted(token);
	hereChar = source->get();
}

bool MbD::SymbolicParser::symfunction()
{
	if (expressionInParentheses() || constant() || namedFunction() || variable()) {
		raisedTo();
		return true;
	}
	else {
		notify("Unrecognized symbol ->");
	}
	return false;
}

bool MbD::SymbolicParser::expression()
{
	if (token == "" && tokenType == "end") {
		auto symNum = std::make_shared<Constant>(0.0);
		stack->push(symNum);
		return true;
	}
	auto sum = std::make_shared<Sum>();
	stack->push(sum);
	if (plusTerm() || minusTerm() || plainTerm()) {
		while (plusTerm() || minusTerm()) {}
		Symsptr term = stack->top();
		if (term->isSum()) {
			auto sum1 = std::static_pointer_cast<Sum>(term);
			if (sum1->isZero()) {
				stack->pop();
			}
			else if (sum1->terms->size() == 1) {
				stack->pop();
				stack->push(sum1->terms->front());
			}
		}
		else {
			notify("Compiler error!");
		}
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::expressionInParentheses()
{
	if (peekForTypeNoPush("(")) {
		if (expression()) {
			if (peekForTypeNoPush(")")) {
				return true;
			}
			else {
				expected(")");
			}
		}
		else {
			expected("expression");
		}
	}
	return false;
}

bool MbD::SymbolicParser::constant()
{
	if (signedNumber()) {
		return true;
	}
	if (peekForTypevalue("word", "pi")) {
		auto symconst = std::make_shared<Constant>(M_PI);
		stack->push(symconst);
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::namedFunction()
{
	return intrinsic();
}

bool MbD::SymbolicParser::intrinsic()
{
	Symsptr symfunc = nullptr;
	if (peekForTypevalue("word", "abs")) {
		symfunc = std::make_shared<Abs>();
	}
	else if (peekForTypevalue("word", "asin") || peekForTypevalue("word", "arcsin")) {
		symfunc = std::make_shared<ArcSine>();
	}
	else if (peekForTypevalue("word", "arctan")) {
		symfunc = std::make_shared<ArcTan>();
	}
	else if (peekForTypevalue("word", "cos")) {
		symfunc = std::make_shared<Cosine>();
	}
	else if (peekForTypevalue("word", "sin")) {
		symfunc = std::make_shared<Sine>();
	}
	else if (peekForTypevalue("word", "spline")) {
		symfunc = std::make_shared<GeneralSpline>();
	}
	else if (peekForTypevalue("word", "integral")) {
		symfunc = std::make_shared<Integral>();
	}
	else if (peekForTypevalue("word", "rampstep")) {
		symfunc = std::make_shared<RampStepFunction>();
	}
	else if (peekForTypevalue("word", "piecewise")) {
		symfunc = std::make_shared<PiecewiseFunction>();
	}
	else if (peekForTypevalue("word", "functions")) {
		symfunc = std::make_shared<Functions>();
	}
	else if (peekForTypevalue("word", "transitions")) {
		symfunc = std::make_shared<Transitions>();
	}
	if (symfunc != nullptr) {
		stack->push(symfunc);
		if (peekForTypeNoPush("(")) {
			auto startsize = stack->size();
			if (expression()) {
				while (commaExpression()) {}
				if (stack->size() > startsize) {
					combineStackTo(startsize);
					if (peekForTypeNoPush(")")) {
						Symsptr args = stack->top();	//args is a Sum with "terms" containing the actual arguments
						stack->pop();
						auto func = std::static_pointer_cast<Function>(stack->top());
						func->arguments(args);
						return true;
					}
					expected(")");
				}
			}
			expected("expression");
		}
		expected("(");
	}
	return false;
}

bool MbD::SymbolicParser::variable()
{
	if ((tokenType == "word") && (variables->count(token) == 1)) {
		auto& var = variables->at(token);
		stack->push(var);
		scanToken();
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::raisedTo()
{
	if (peekForTypeNoPush("^")) {
		if (symfunction()) {
			Symsptr exp = stack->top();
			stack->pop();
			Symsptr base = stack->top();
			stack->pop();
			auto pow = std::make_shared<Power>(base, exp);
			stack->push(pow);
			return true;
		}
		expected("function");
	}
	return false;
}

bool MbD::SymbolicParser::expected(const std::string&)
{
	return false;
}

bool MbD::SymbolicParser::signedNumber()
{
	if (tokenType == "number") {
		auto symNum = std::make_shared<Constant>(tokenNum);
		stack->push(symNum);
		scanToken();
		return true;
	}
	if ((token == "+") && (hereChar != EOF) && (std::isdigit(hereChar) || (hereChar == '.'))) {
		//"no intervening delimiters"
		scanToken();
		auto symNum = std::make_shared<Constant>(tokenNum);
		stack->push(symNum);
		scanToken();
		return true;
	}
	if ((token == "-") && (hereChar != EOF) && (std::isdigit(hereChar) || (hereChar == '.'))) {
		//"no intervening delimiters"
		scanToken();
		auto symNum = std::make_shared<Constant>(-tokenNum);
		stack->push(symNum);
		scanToken();
		return true;
	}
	return false;
}

bool MbD::SymbolicParser::peekForTypevalue(const std::string& type, std::string symbol)
{
	if ((tokenType == type) && (token == symbol)) {
		scanToken();
		return true;
	}
	return false;
}

void MbD::SymbolicParser::notify(const std::string& msg) const
{
	notifyat(msg, mark);
}

void MbD::SymbolicParser::notifyat(const std::string&, int) const
{
	//"Temporarily reset source in order to get full contents"
	auto p = source->tellg();
	source->seekg(0);
	auto contents = source->str();
	source->seekg(p);
	assert(false);
	//SyntaxErrorException new
	//targetClass : class;
	//messageText: aString;
	//source: contents;
	//position: position;
	//raiseSignal
}

void MbD::SymbolicParser::combineStackTo(size_t pos) const
{
	auto args = std::make_shared<std::vector<Symsptr>>();
	while (stack->size() > pos) {
		Symsptr arg = stack->top();
		stack->pop();
		args->push_back(arg);
	}
	std::reverse(args->begin(), args->end());
	auto sum = std::make_shared<Arguments>();
	sum->terms = args;
	stack->push(sum);
}

bool MbD::SymbolicParser::isNextLineTag(char c) const
{
	//Skip <n> tag in asmt file
	auto pos = source->tellg();
	char ch = c;
	if (ch == '<') {
		ch = source->get();
		if (ch == 'n') {
			ch = source->get();
			if (ch == '>') {
				return true;
			}
			else {
				source->seekg(pos);
			}
		}
		else {
			source->seekg(pos);
		}
	}
	return false;
}

