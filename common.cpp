/*
 * common.cpp
 *
 *  Created on: Dec 21, 2012
 *      Author: akavo
 */

#include "common.h"

Exc::Exc(std::string ss)
: s(ss)
{
}
Exc::~Exc() throw()
{
}
const char* Exc::what() const throw()
{
	return s.c_str();
}

/////////////////////////////////////////////

bool isField(std::string field)
{
	if(field == "<Artist>"
	|| field == "<Title>"
	|| field == "<Album>")
		return true;
	return false;
}

