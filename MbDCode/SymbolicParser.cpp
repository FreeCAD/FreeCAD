#include <corecrt_math_defines.h>
#include <sstream>
#include <iomanip>

#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"
#include "Sum.h"
#include "Product.h"
#include "CREATE.h"
#include "Power.h"
#include "Abs.h"
#include "ArcTan.h"

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

void MbD::SymbolicParser::parseString(std::string expr)
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
		if (plainTerm()) return true;
		expected("plainTerm");
	}
	return false;
}

bool MbD::SymbolicParser::plainTerm()
{
	if (term()) {
		auto trm = stack->top();
		stack->pop();
		auto sum = stack->top();
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
		auto term = stack->top();
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
		auto trm = stack->top();
		stack->pop();
		auto product = stack->top();
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
		if (plainFunction()) return true;
		expected("plainFunction");
	}
	return false;
}

bool MbD::SymbolicParser::peekForTypeNoPush(std::string c)
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
	prevEnd = (int)source->tellg();
	prevEnd--;
	while (std::isspace(hereChar)) {
		hereChar = source->get();
	}
	if (hereChar == EOF) {
		mark = prevEnd + 1;
		tokenType = "end";
		return token = "";
	}
	mark = (int)source->tellg();
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
		if (!std::isalnum(hereChar)) break;
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
	auto sum = std::make_shared<Sum>();
	stack->push(sum);
	if (plusTerm() || minusTerm() || plainTerm()) {
		while (plusTerm() || minusTerm()) {}
		auto term = stack->top();
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
	else if (peekForTypevalue("word", "arctan")) {
		symfunc = std::make_shared<ArcTan>();
	}
	if (symfunc != nullptr) {
		stack->push(symfunc);
		if (peekForTypeNoPush("(")) {
			auto startsize = stack->size();
			while (expression() && peekForTypeNoPush(",")) {}
			if (stack->size() > startsize) {
				combineStackTo((int)startsize);
				if (peekForTypeNoPush(")")) {
					auto args = stack->top();
					stack->pop();
					auto func = stack->top();
					func->arguments(args);
					stack->pop();
					return true;
				}
				expected(")");
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
			auto exp = stack->top();
			stack->pop();
			auto base = stack->top();
			stack->pop();
			auto pow = std::make_shared<Power>(base, exp);
			stack->push(pow);
			return true;
		}
		expected("function");
	}
	return false;
}

bool MbD::SymbolicParser::expected(std::string msg)
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

bool MbD::SymbolicParser::peekForTypevalue(std::string type, std::string symbol)
{
	if ((tokenType == type) && (token == symbol)) {
		scanToken();
		return true;
	}
	return false;
}

void MbD::SymbolicParser::notify(std::string msg)
{
	notifyat(msg, mark);
}

void MbD::SymbolicParser::notifyat(std::string msg, int mrk)
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

void MbD::SymbolicParser::combineStackTo(int pos)
{
	auto args = std::make_shared<Sum>();
	while (stack->size() > pos) {
		auto arg = stack->top();
		stack->pop();
		args->addTerm(arg);
	}
	stack->push(args);
}

