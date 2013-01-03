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
namespace fs = boost::filesystem;

#   ifdef BOOST_WINDOWS_API
    typedef wchar_t										char_type;
	#define tcout 										std::wcout
	#define tcerr 										std::wcerr
	#define _T(x)										L ##x
#   else
    typedef char_t										char_type;
    #define tcout 										std::cout
	#define tcerr 										std::cerr
	#define _T(x) 										x
#   endif

    typedef std::basic_string<char_type>               tstring;


bool isField(tstring field);

////////////////////////////////////////////////////////
struct Exc : public std::exception
{
   std::string s;
   Exc(std::string ss);
   virtual ~Exc() throw();
   const char* what() const throw();
};


#endif /* COMMON_H_ */
