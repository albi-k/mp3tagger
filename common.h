/*
 * common.h
 *
 *  Created on: Dec 21, 2012
 *      Author: akavo
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <string>

//////////////////////////////////////////////////
#include <boost/filesystem.hpp>
#include "boost/program_options.hpp"
namespace fs = boost::filesystem;
namespace po = boost::program_options;

#   ifdef BOOST_WINDOWS_API
    typedef wchar_t										char_type;
	#define tcout 										std::wcout
	#define tcerr 										std::wcerr
	#define _T(x)										L ##x
	#define tvalue										wvalue
#   else
    typedef char										char_type;
    #define tcout 										std::cout
	#define tcerr 										std::cerr
	#define _T(x) 										x
	#define tvalue										value
#   endif

    typedef std::basic_string<char_type>				tstring;

bool isField(tstring field);

////////////////////////////////////////////////////////

struct Exc : public std::exception
{
   std::string s;
   Exc(std::string ss);
   virtual ~Exc() throw();
   const char* what() const throw();
};

////////////////////////////////////////////////////////
#define BOOST_THREAD_USE_LIB

#include <boost/thread/thread.hpp>
#include <sstream>
#include <ostream>
#include <iostream>

class atomic_message 
	: public std::basic_ostringstream<char_type>
{
public:
    ~atomic_message();
private:
	static boost::mutex s_mtx;
};

#define LogType atomic_message
#define Log atomic_message().flush()

/////////////////////////////////////////////////////////

void HardKill(boost::thread *thread);


#endif /* COMMON_H_ */
