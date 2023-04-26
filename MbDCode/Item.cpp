#include "Item.h"

void MbD::Item::setName(std::string& str)
{
	name = str;
}

const std::string& MbD::Item::getName() const
{
	return name;
}
//
//void MbD::Item::setMyInt(int val)
//{
//	myInt = val;
//}
//
//int MbD::Item::getMyInt()
//{
//	return myInt;
//}
