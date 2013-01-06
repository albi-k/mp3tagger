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

/////////////////////////////////////////////

boost::mutex atomic_message::s_mtx;

/////////////////////////////////////////////

#ifdef BOOST_WINDOWS_API
	#include <windows.h>
	void HardKill(boost::thread *thread)
	{
		TerminateThread(thread->native_handle(), EXIT_SUCCESS);
        return;
	}

#else
	#include <pthread.h>
	void HardKill(boost::thread *thread)
	{
		pthread_cancel(thread->native_handle())
        return;
	}
#endif