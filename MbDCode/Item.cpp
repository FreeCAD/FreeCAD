#include "Item.h"

using namespace MbD;

Item::Item() {
	initialize();
}

Item::Item(const char* str) : name(str) 
{
	initialize();
}

void Item::initialize()
{
}

void Item::setName(std::string& str)
{
	name = str;
}

const std::string& Item::getName() const
{
	return name;
}

void Item::initializeLocally()
{
}

void Item::initializeGlobally()
{
}
