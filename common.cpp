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

bool isField(tstring field)
{
	if(field == _T("<Artist>")
	|| field == _T("<Title>")
	|| field == _T("<Album>")
	|| field == _T("<Genre>")
	|| field == _T("<Comment>")
	|| field == _T("<Track#>")
	|| field == _T("<Year>")
	|| field == _T("<Ignore>")
	)
		return true;
	return false;
}

